#pragma once

#include "base/Math.h"
#include "render/Framebuffer.h"
#include "render/Mesh.h"
#include "render/Vertex.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace sr::render {

template<typename vertex_t, typename uniforms_t, typename varyings_t>
class Pipeline {
public:
    Pipeline(Program<vertex_t, uniforms_t, varyings_t> program, uniforms_t uniforms)
        : m_Program(std::move(program)),
          m_Uniforms(std::move(uniforms))
    {
        static_assert(std::is_base_of_v<VertexBase, vertex_t>,
            "vertex_t must derive from VertexBase");
        static_assert(std::is_base_of_v<UniformsBase, uniforms_t>,
            "uniforms_t must derive from UniformsBase");
        static_assert(std::is_base_of_v<VaryingsBase, varyings_t>,
            "varyings_t must derive from VaryingsBase");
    }

    uniforms_t& Uniforms() { return m_Uniforms; }
    const uniforms_t& Uniforms() const { return m_Uniforms; }

    void Run(Framebuffer& framebuffer, const Mesh<vertex_t>& mesh) const
    {
        if (mesh.Empty() || m_Program.vertexShader == nullptr || m_Program.fragmentShader == nullptr) {
            return;
        }

        const std::vector<typename Mesh<vertex_t>::index_t>& indices = mesh.Indices();
        if (indices.size() < 3 || indices.size() % 3 != 0) {
            return;
        }

        const std::vector<vertex_t>& vertices = mesh.Vertices();
        const int width = framebuffer.Width();
        const int height = framebuffer.Height();
        StagedFrame stagedFrame = InitializeStagedFrame(framebuffer, width, height);

        for (std::size_t triangleIndex = 0; triangleIndex < indices.size(); triangleIndex += 3) {
            const typename Mesh<vertex_t>::index_t i0 = indices[triangleIndex + 0];
            const typename Mesh<vertex_t>::index_t i1 = indices[triangleIndex + 1];
            const typename Mesh<vertex_t>::index_t i2 = indices[triangleIndex + 2];
            if (!AreIndicesValid(vertices, i0, i1, i2)) {
                continue;
            }

            ScreenVertex sv0;
            ScreenVertex sv1;
            ScreenVertex sv2;

            m_Program.vertexShader(sv0.varyings, vertices[i0], m_Uniforms);
            m_Program.vertexShader(sv1.varyings, vertices[i1], m_Uniforms);
            m_Program.vertexShader(sv2.varyings, vertices[i2], m_Uniforms);

            if (!FinalizeVertex(framebuffer, sv0)
                || !FinalizeVertex(framebuffer, sv1)
                || !FinalizeVertex(framebuffer, sv2)) {
                continue;
            }

            const std::vector<Fragment> fragments = RasterizeTriangle(framebuffer, sv0, sv1, sv2);
            ShadeFragments(fragments, width, stagedFrame);
        }

        WriteInFramebuffer(framebuffer, width, height, stagedFrame);
    }

private:
    struct ScreenPoint {
        math::Vec2 position;
        float depth;
        float reciprocalW;
    };

    struct ScreenVertex {
        varyings_t varyings;
        ScreenPoint screen;
    };

    struct RasterBounds {
        int startX = 0;
        int endX = -1;
        int startY = 0;
        int endY = -1;
        float area = 0.0f;
        bool positiveArea = true;
    };

    struct Fragment {
        int x = 0;
        int y = 0;
        varyings_t varyings{};
    };

    struct StagedFrame {
        std::vector<float> depth;
        std::vector<Color> color;
        std::vector<bool> coverage;
    };

    static std::size_t PixelIndex(int width, int x, int y)
    {
        return static_cast<std::size_t>(y * width + x);
    }

    static bool AreIndicesValid(const std::vector<vertex_t>& vertices,
        typename Mesh<vertex_t>::index_t i0,
        typename Mesh<vertex_t>::index_t i1,
        typename Mesh<vertex_t>::index_t i2)
    {
        const std::size_t vertexCount = vertices.size();
        return static_cast<std::size_t>(i0) < vertexCount
            && static_cast<std::size_t>(i1) < vertexCount
            && static_cast<std::size_t>(i2) < vertexCount;
    }

    static float EdgeFunction(const math::Vec2& a,
        const math::Vec2& b,
        const math::Vec2& point)
    {
        return (point.x - a.x) * (b.y - a.y) - (point.y - a.y) * (b.x - a.x);
    }

    static bool ValidateClipPosition(const math::Vec4& clipPosition)
    {
        return clipPosition.w > 0.0f && std::isfinite(clipPosition.w);
    }

    static math::Vec4 CalculateNdcPosition(const math::Vec4& clipPosition)
    {
        const float reciprocalW = 1.0f / clipPosition.w;
        math::Vec4 ndcPosition = clipPosition * reciprocalW;
        ndcPosition.w = reciprocalW;
        return ndcPosition;
    }

    static bool ValidateNdcPosition(const math::Vec4& ndcPosition)
    {
        return std::isfinite(ndcPosition.x)
            && std::isfinite(ndcPosition.y)
            && std::isfinite(ndcPosition.z);
    }

    static math::Vec4 CalculateFragPosition(const Framebuffer& framebuffer,
        const math::Vec4& ndcPosition)
    {
        const float width = static_cast<float>(framebuffer.Width() - 1);
        const float height = static_cast<float>(framebuffer.Height() - 1);
        return math::Vec4{
            (ndcPosition.x * 0.5f + 0.5f) * width,
            (0.5f - ndcPosition.y * 0.5f) * height,
            math::Clamp(ndcPosition.z * 0.5f + 0.5f, 0.0f, 1.0f),
            ndcPosition.w,
        };
    }

    static ScreenPoint BuildScreenPoint(const math::Vec4& fragPosition)
    {
        return ScreenPoint{
            math::Vec2{ fragPosition.x, fragPosition.y },
            fragPosition.z,
            fragPosition.w,
        };
    }

    static StagedFrame InitializeStagedFrame(Framebuffer& framebuffer, int width, int height)
    {
        StagedFrame stagedFrame{
            std::vector<float>(static_cast<std::size_t>(width * height), 1.0f),
            std::vector<Color>(static_cast<std::size_t>(width * height)),
            std::vector<bool>(static_cast<std::size_t>(width * height), false),
        };

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                stagedFrame.depth[PixelIndex(width, x, y)] = framebuffer.GetDepth(x, y);
            }
        }

        return stagedFrame;
    }

    bool FinalizeVertex(Framebuffer& framebuffer, ScreenVertex& vertex) const
    {
        const math::Vec4& clipPosition = vertex.varyings.clipPos;
        if (!ValidateClipPosition(clipPosition)) {
            return false;
        }

        const math::Vec4 ndcPosition = CalculateNdcPosition(clipPosition);
        if (!ValidateNdcPosition(ndcPosition)) {
            return false;
        }

        const math::Vec4 fragPosition = CalculateFragPosition(framebuffer, ndcPosition);
        vertex.varyings.ndcPos = ndcPosition;
        vertex.varyings.fragPos = fragPosition;
        vertex.screen = BuildScreenPoint(fragPosition);
        return true;
    }

    varyings_t InterpolateVaryings(Framebuffer& framebuffer,
        const ScreenVertex& v0,
        const ScreenVertex& v1,
        const ScreenVertex& v2,
        float w0,
        float w1,
        float w2) const
    {
        static_assert(std::is_standard_layout_v<varyings_t>,
            "varyings_t must be standard layout");
        static_assert(sizeof(varyings_t) % sizeof(float) == 0,
            "varyings_t must contain only float-compatible fields");

        const float correctedW0 = w0 * v0.screen.reciprocalW;
        const float correctedW1 = w1 * v1.screen.reciprocalW;
        const float correctedW2 = w2 * v2.screen.reciprocalW;
        const float sum = correctedW0 + correctedW1 + correctedW2;

        const float normalizedW0 = sum == 0.0f ? w0 : correctedW0 / sum;
        const float normalizedW1 = sum == 0.0f ? w1 : correctedW1 / sum;
        const float normalizedW2 = sum == 0.0f ? w2 : correctedW2 / sum;

        varyings_t out{};
        out.clipPos = normalizedW0 * v0.varyings.clipPos
            + normalizedW1 * v1.varyings.clipPos
            + normalizedW2 * v2.varyings.clipPos;

        const float clipW = out.clipPos.w;
        if (clipW > 0.0f && std::isfinite(clipW)) {
            out.ndcPos = CalculateNdcPosition(out.clipPos);
            out.fragPos = CalculateFragPosition(framebuffer, out.ndcPos);
        }

        constexpr std::size_t baseFloatCount = sizeof(VaryingsBase) / sizeof(float);
        constexpr std::size_t totalFloatCount = sizeof(varyings_t) / sizeof(float);

        if constexpr (totalFloatCount > baseFloatCount) {
            float* outFloats = reinterpret_cast<float*>(&out);
            const float* v0Floats = reinterpret_cast<const float*>(&v0.varyings);
            const float* v1Floats = reinterpret_cast<const float*>(&v1.varyings);
            const float* v2Floats = reinterpret_cast<const float*>(&v2.varyings);

            for (std::size_t i = baseFloatCount; i < totalFloatCount; ++i) {
                outFloats[i] = normalizedW0 * v0Floats[i]
                    + normalizedW1 * v1Floats[i]
                    + normalizedW2 * v2Floats[i];
            }
        }

        return out;
    }

    static float CalculateTriangleArea(const ScreenVertex& v0,
        const ScreenVertex& v1,
        const ScreenVertex& v2)
    {
        return EdgeFunction(v0.screen.position, v1.screen.position, v2.screen.position);
    }

    static bool IsDegenerateTriangle(float area)
    {
        return area == 0.0f;
    }

    static RasterBounds CalculateRasterBounds(const Framebuffer& framebuffer,
        const ScreenVertex& v0,
        const ScreenVertex& v1,
        const ScreenVertex& v2,
        float area)
    {
        const float minX = std::min({ v0.screen.position.x, v1.screen.position.x, v2.screen.position.x });
        const float maxX = std::max({ v0.screen.position.x, v1.screen.position.x, v2.screen.position.x });
        const float minY = std::min({ v0.screen.position.y, v1.screen.position.y, v2.screen.position.y });
        const float maxY = std::max({ v0.screen.position.y, v1.screen.position.y, v2.screen.position.y });

        return RasterBounds{
            std::max(0, static_cast<int>(std::floor(minX))),
            std::min(framebuffer.Width() - 1, static_cast<int>(std::ceil(maxX))),
            std::max(0, static_cast<int>(std::floor(minY))),
            std::min(framebuffer.Height() - 1, static_cast<int>(std::ceil(maxY))),
            area,
            area > 0.0f,
        };
    }

    static math::Vec2 CalculateSamplePoint(int x, int y)
    {
        return math::Vec2{
            static_cast<float>(x) + 0.5f,
            static_cast<float>(y) + 0.5f,
        };
    }

    static bool IsSampleCovered(float e0, float e1, float e2, bool positiveArea)
    {
        if (positiveArea) {
            return e0 >= 0.0f && e1 >= 0.0f && e2 >= 0.0f;
        }

        return e0 <= 0.0f && e1 <= 0.0f && e2 <= 0.0f;
    }

    std::vector<Fragment> RasterizeTriangle(Framebuffer& framebuffer,
        const ScreenVertex& v0,
        const ScreenVertex& v1,
        const ScreenVertex& v2) const
    {
        std::vector<Fragment> fragments;

        const float area = CalculateTriangleArea(v0, v1, v2);
        if (IsDegenerateTriangle(area)) {
            return fragments;
        }

        const RasterBounds bounds = CalculateRasterBounds(framebuffer, v0, v1, v2, area);
        for (int y = bounds.startY; y <= bounds.endY; ++y) {
            for (int x = bounds.startX; x <= bounds.endX; ++x) {
                const math::Vec2 samplePoint = CalculateSamplePoint(x, y);
                const float e0 = EdgeFunction(v1.screen.position, v2.screen.position, samplePoint);
                const float e1 = EdgeFunction(v2.screen.position, v0.screen.position, samplePoint);
                const float e2 = EdgeFunction(v0.screen.position, v1.screen.position, samplePoint);
                if (!IsSampleCovered(e0, e1, e2, bounds.positiveArea)) {
                    continue;
                }

                const float w0 = e0 / bounds.area;
                const float w1 = e1 / bounds.area;
                const float w2 = e2 / bounds.area;
                fragments.push_back(Fragment{
                    x,
                    y,
                    InterpolateVaryings(framebuffer, v0, v1, v2, w0, w1, w2),
                });
            }
        }

        return fragments;
    }

    void ShadeFragments(const std::vector<Fragment>& fragments,
        int width,
        StagedFrame& stagedFrame) const
    {
        for (const Fragment& fragment : fragments) {
            const std::size_t index = PixelIndex(width, fragment.x, fragment.y);
            const float depth = fragment.varyings.fragPos.z;
            if (depth >= stagedFrame.depth[index]) {
                continue;
            }

            const Color shadedColor = m_Program.fragmentShader(fragment.varyings, m_Uniforms);
            stagedFrame.depth[index] = depth;
            stagedFrame.color[index] = shadedColor;
            stagedFrame.coverage[index] = true;
        }
    }

    static void WriteInFramebuffer(Framebuffer& framebuffer,
        int width,
        int height,
        const StagedFrame& stagedFrame)
    {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::size_t index = PixelIndex(width, x, y);
                if (!stagedFrame.coverage[index]) {
                    continue;
                }

                framebuffer.SetDepth(x, y, stagedFrame.depth[index]);
                framebuffer.SetPixel(x, y, stagedFrame.color[index]);
            }
        }
    }

    Program<vertex_t, uniforms_t, varyings_t> m_Program;
    uniforms_t m_Uniforms;
};

} // namespace sr::render
