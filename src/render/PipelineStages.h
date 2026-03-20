#pragma once

#include "base/Math.h"
#include "render/Framebuffer.h"
#include "render/PipelineMathHelper.h"
#include "render/Vertex.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace sr::render::detail {

struct ScreenPoint {
    math::Vec2 position;
    float depth = 0.0f;
    float reciprocalW = 0.0f;
};

template<typename varyings_t>
struct ScreenVertex {
    varyings_t varyings{};
    ScreenPoint screen{};
};

struct RasterBounds {
    int startX = 0;
    int endX = -1;
    int startY = 0;
    int endY = -1;
    float area = 0.0f;
    bool positiveArea = true;
};

template<typename vertex_t, typename uniforms_t, typename varyings_t>
struct VertexStage {
    using Triangle = std::array<varyings_t, 3>;

    static bool ProgramAvailable(const Program<vertex_t, uniforms_t, varyings_t>& program)
    {
        return program.vertexShader != nullptr && program.fragmentShader != nullptr;
    }

    static Triangle Execute(const Program<vertex_t, uniforms_t, varyings_t>& program,
        const uniforms_t& uniforms,
        const vertex_t& vertex0,
        const vertex_t& vertex1,
        const vertex_t& vertex2)
    {
        Triangle triangle{};
        program.vertexShader(triangle[0], vertex0, uniforms);
        program.vertexShader(triangle[1], vertex1, uniforms);
        program.vertexShader(triangle[2], vertex2, uniforms);
        return triangle;
    }
};

template<typename varyings_t>
struct ClipStage {
    using Math = PipelineMathHelper;
    using Triangle = std::array<varyings_t, 3>;

    enum class ClipPlane {
        Left,
        Right,
        Bottom,
        Top,
        Near,
        Far,
    };

    static std::vector<Triangle> Execute(const Triangle& inputTriangle)
    {
        const std::vector<varyings_t> clippedPolygon = ClipTriangleAgainstFrustum(
            inputTriangle[0],
            inputTriangle[1],
            inputTriangle[2]);
        return TriangulatePolygon(clippedPolygon);
    }

private:
    static float ClipDistance(const math::Vec4& clipPosition, ClipPlane plane)
    {
        switch (plane) {
        case ClipPlane::Left:
            return clipPosition.x + clipPosition.w;
        case ClipPlane::Right:
            return clipPosition.w - clipPosition.x;
        case ClipPlane::Bottom:
            return clipPosition.y + clipPosition.w;
        case ClipPlane::Top:
            return clipPosition.w - clipPosition.y;
        case ClipPlane::Near:
            return clipPosition.z + clipPosition.w;
        case ClipPlane::Far:
            return clipPosition.w - clipPosition.z;
        }

        return -1.0f;
    }

    static bool IsInsideClipPlane(const math::Vec4& clipPosition, ClipPlane plane)
    {
        return ClipDistance(clipPosition, plane) >= 0.0f;
    }

    static bool ValidateClipPosition(const math::Vec4& clipPosition)
    {
        return std::isfinite(clipPosition.x)
            && std::isfinite(clipPosition.y)
            && std::isfinite(clipPosition.z)
            && std::isfinite(clipPosition.w);
    }

    static varyings_t LerpWholeVaryings(const varyings_t& start,
        const varyings_t& end,
        float t)
    {
        varyings_t out{};
        float* outFloats = reinterpret_cast<float*>(&out);
        const float* startFloats = reinterpret_cast<const float*>(&start);
        const float* endFloats = reinterpret_cast<const float*>(&end);
        constexpr std::size_t kTotalFloatCount = sizeof(varyings_t) / sizeof(float);

        for (std::size_t i = 0; i < kTotalFloatCount; ++i) {
            outFloats[i] = math::Lerp(startFloats[i], endFloats[i], t);
        }

        return out;
    }

    static varyings_t IntersectClipEdge(const varyings_t& start,
        const varyings_t& end,
        float startDistance,
        float endDistance)
    {
        const float denominator = startDistance - endDistance;
        if (std::abs(denominator) <= Math::kClipEpsilon) {
            return start;
        }

        const float t = math::Clamp(startDistance / denominator, 0.0f, 1.0f);
        return LerpWholeVaryings(start, end, t);
    }

    static std::vector<varyings_t> ClipPolygonAgainstPlane(const std::vector<varyings_t>& input,
        ClipPlane plane)
    {
        std::vector<varyings_t> output;
        if (input.empty()) {
            return output;
        }

        output.reserve(input.size() + 1);

        varyings_t previous = input.back();
        float previousDistance = ClipDistance(previous.clipPos, plane);
        bool previousInside = IsInsideClipPlane(previous.clipPos, plane);

        for (const varyings_t& current : input) {
            const float currentDistance = ClipDistance(current.clipPos, plane);
            const bool currentInside = IsInsideClipPlane(current.clipPos, plane);

            if (previousInside != currentInside) {
                output.push_back(IntersectClipEdge(previous, current, previousDistance, currentDistance));
            }

            if (currentInside) {
                output.push_back(current);
            }

            previous = current;
            previousDistance = currentDistance;
            previousInside = currentInside;
        }

        return output;
    }

    static std::vector<varyings_t> ClipTriangleAgainstFrustum(const varyings_t& v0,
        const varyings_t& v1,
        const varyings_t& v2)
    {
        if (!ValidateClipPosition(v0.clipPos)
            || !ValidateClipPosition(v1.clipPos)
            || !ValidateClipPosition(v2.clipPos)) {
            return {};
        }

        std::vector<varyings_t> polygon{ v0, v1, v2 };
        constexpr std::array<ClipPlane, 6> kClipPlanes{
            ClipPlane::Left,
            ClipPlane::Right,
            ClipPlane::Bottom,
            ClipPlane::Top,
            ClipPlane::Near,
            ClipPlane::Far,
        };

        for (const ClipPlane plane : kClipPlanes) {
            polygon = ClipPolygonAgainstPlane(polygon, plane);
            if (polygon.size() < 3) {
                return {};
            }
        }

        return polygon;
    }

    static std::vector<Triangle> TriangulatePolygon(const std::vector<varyings_t>& polygon)
    {
        std::vector<Triangle> triangles;
        if (polygon.size() < 3) {
            return triangles;
        }

        triangles.reserve(polygon.size() - 2);
        for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
            triangles.push_back(Triangle{ polygon[0], polygon[i], polygon[i + 1] });
        }

        return triangles;
    }
};

template<typename varyings_t>
struct RasterStage {
    using Math = PipelineMathHelper;
    using Triangle = std::array<varyings_t, 3>;
    using ScreenVertex = detail::ScreenVertex<varyings_t>;

    static constexpr std::size_t kBaseFloatCount = sizeof(VaryingsBase) / sizeof(float);
    static constexpr std::size_t kTotalFloatCount = sizeof(varyings_t) / sizeof(float);

    template<typename fragment_consumer_t>
    static void Execute(const Framebuffer& framebuffer,
        const Triangle& triangle,
        FaceCullMode faceCullMode,
        fragment_consumer_t&& consumeFragment)
    {
        ScreenVertex v0{ triangle[0], {} };
        ScreenVertex v1{ triangle[1], {} };
        ScreenVertex v2{ triangle[2], {} };

        if (!FinalizeVertex(framebuffer, v0)
            || !FinalizeVertex(framebuffer, v1)
            || !FinalizeVertex(framebuffer, v2)) {
            return;
        }

        RasterizeTriangle(
            framebuffer,
            v0,
            v1,
            v2,
            faceCullMode,
            std::forward<fragment_consumer_t>(consumeFragment));
    }

private:
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
            && std::isfinite(ndcPosition.z)
            && std::isfinite(ndcPosition.w);
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

    static bool FinalizeVertex(const Framebuffer& framebuffer, ScreenVertex& vertex)
    {
        const math::Vec4& clipPosition = vertex.varyings.clipPos;
        if (!std::isfinite(clipPosition.x)
            || !std::isfinite(clipPosition.y)
            || !std::isfinite(clipPosition.z)
            || !std::isfinite(clipPosition.w)
            || std::abs(clipPosition.w) <= Math::kClipEpsilon) {
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

    static varyings_t InterpolateVaryings(const Framebuffer& framebuffer,
        const ScreenVertex& v0,
        const ScreenVertex& v1,
        const ScreenVertex& v2,
        float w0,
        float w1,
        float w2)
    {
        const float correctedW0 = w0 * v0.screen.reciprocalW;
        const float correctedW1 = w1 * v1.screen.reciprocalW;
        const float correctedW2 = w2 * v2.screen.reciprocalW;
        const float sum = correctedW0 + correctedW1 + correctedW2;

        const float normalizedW0 = std::abs(sum) <= Math::kAreaEpsilon ? w0 : correctedW0 / sum;
        const float normalizedW1 = std::abs(sum) <= Math::kAreaEpsilon ? w1 : correctedW1 / sum;
        const float normalizedW2 = std::abs(sum) <= Math::kAreaEpsilon ? w2 : correctedW2 / sum;

        varyings_t out{};
        out.clipPos = normalizedW0 * v0.varyings.clipPos
            + normalizedW1 * v1.varyings.clipPos
            + normalizedW2 * v2.varyings.clipPos;

        const float clipW = out.clipPos.w;
        if (std::abs(clipW) > Math::kClipEpsilon && std::isfinite(clipW)) {
            out.ndcPos = CalculateNdcPosition(out.clipPos);
            out.fragPos = CalculateFragPosition(framebuffer, out.ndcPos);
        }

        if constexpr (kTotalFloatCount > kBaseFloatCount) {
            float* outFloats = reinterpret_cast<float*>(&out);
            const float* v0Floats = reinterpret_cast<const float*>(&v0.varyings);
            const float* v1Floats = reinterpret_cast<const float*>(&v1.varyings);
            const float* v2Floats = reinterpret_cast<const float*>(&v2.varyings);

            for (std::size_t i = kBaseFloatCount; i < kTotalFloatCount; ++i) {
                outFloats[i] = normalizedW0 * v0Floats[i]
                    + normalizedW1 * v1Floats[i]
                    + normalizedW2 * v2Floats[i];
            }
        }

        return out;
    }

    static bool ShouldCullTriangle(float area, FaceCullMode faceCullMode)
    {
        if (faceCullMode == FaceCullMode::None) {
            return false;
        }

        return area <= Math::kAreaEpsilon;
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

    template<typename fragment_consumer_t>
    static void RasterizeTriangle(const Framebuffer& framebuffer,
        const ScreenVertex& v0,
        const ScreenVertex& v1,
        const ScreenVertex& v2,
        FaceCullMode faceCullMode,
        fragment_consumer_t&& consumeFragment)
    {
        const float area = Math::CalculateTriangleArea(v0.screen.position, v1.screen.position, v2.screen.position);
        if (Math::IsDegenerateTriangle(area) || ShouldCullTriangle(area, faceCullMode)) {
            return;
        }

        const RasterBounds bounds = CalculateRasterBounds(framebuffer, v0, v1, v2, area);
        for (int y = bounds.startY; y <= bounds.endY; ++y) {
            const std::size_t rowStartIndex = framebuffer.PixelIndexUnchecked(bounds.startX, y);
            for (int x = bounds.startX; x <= bounds.endX; ++x) {
                const math::Vec2 samplePoint = Math::CalculateSamplePoint(x, y);
                const float e0 = Math::EdgeFunction(v1.screen.position, v2.screen.position, samplePoint);
                const float e1 = Math::EdgeFunction(v2.screen.position, v0.screen.position, samplePoint);
                const float e2 = Math::EdgeFunction(v0.screen.position, v1.screen.position, samplePoint);
                if (!Math::IsSampleCovered(e0, e1, e2, bounds.positiveArea)) {
                    continue;
                }

                const float w0 = e0 / bounds.area;
                const float w1 = e1 / bounds.area;
                const float w2 = e2 / bounds.area;
                consumeFragment(
                    rowStartIndex + static_cast<std::size_t>(x - bounds.startX),
                    InterpolateVaryings(framebuffer, v0, v1, v2, w0, w1, w2));
            }
        }
    }
};

template<typename vertex_t, typename uniforms_t, typename varyings_t>
struct FragmentStage {
    static void Execute(std::size_t pixelIndex,
        const varyings_t& varyings,
        const Program<vertex_t, uniforms_t, varyings_t>& program,
        const uniforms_t& uniforms,
        Framebuffer& framebuffer)
    {
        float* const depthBuffer = framebuffer.MutableDepthData();
        std::uint32_t* const pixelBuffer = framebuffer.MutablePixelData();

        const float depth = varyings.fragPos.z;
        if (depth >= depthBuffer[pixelIndex]) {
            return;
        }

        depthBuffer[pixelIndex] = depth;
        pixelBuffer[pixelIndex] = program.fragmentShader(varyings, uniforms).ToBgra8();
    }
};

} // namespace sr::render::detail

