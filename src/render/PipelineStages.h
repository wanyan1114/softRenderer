#pragma once

#include "base/Math.h"
#include "base/StateMachine.h"
#include "render/Framebuffer.h"
#include "render/PipelineMathHelper.h"
#include "render/Program.h"
#include "render/shader/ShaderTypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace sr::render::detail {

enum class PipelineStageId {
    Vertex,
    Clip,
    Raster,
    Fragment,
    Completed,
    Invalid,
};

template<typename varyings_t>
struct VertexStageData {
    using Triangle = std::array<varyings_t, 3>;

    Triangle triangle{};
};

template<typename varyings_t>
struct ClipStageData {
    using Triangle = std::array<varyings_t, 3>;

    std::vector<Triangle> clippedTriangles;
    std::size_t currentTriangleIndex = 0;
};

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

template<typename varyings_t>
struct RasterStageData {
    using ScreenTriangle = std::array<ScreenVertex<varyings_t>, 3>;

    ScreenTriangle screenTriangle{};
    RasterBounds bounds{};
    bool hasTriangle = false;
};

template<typename vertex_t, typename uniforms_t, typename varyings_t>
bool ProgramAvailable(const Program<vertex_t, uniforms_t, varyings_t>& program)
{
    return program.vertexShader != nullptr && program.fragmentShader != nullptr;
}

template<typename vertex_t, typename uniforms_t, typename varyings_t>
class PipelineStageMachine;

template<typename machine_t>
class VertexStage final : public base::State<machine_t, PipelineStageId> {
public:
    PipelineStageId GetID() const override
    {
        return PipelineStageId::Vertex;
    }

    void OnExcute(machine_t& machine) override
    {
        using triangle_t = typename machine_t::vertex_triangle_t;

        triangle_t triangle{};
        const auto& input = machine.InputTriangle();
        const auto& program = machine.ProgramRef();
        const auto& uniforms = machine.UniformsRef();

        program.vertexShader(triangle[0], input[0], uniforms);
        program.vertexShader(triangle[1], input[1], uniforms);
        program.vertexShader(triangle[2], input[2], uniforms);

        machine.VertexData().triangle = std::move(triangle);
        machine.ChangeToClip();
    }
};

template<typename machine_t>
class ClipStage final : public base::State<machine_t, PipelineStageId> {
public:
    PipelineStageId GetID() const override
    {
        return PipelineStageId::Clip;
    }

    void OnExcute(machine_t& machine) override
    {
        auto& clipData = machine.ClipData();
        clipData.clippedTriangles = Execute(machine.VertexData().triangle);
        clipData.currentTriangleIndex = 0;

        if (clipData.clippedTriangles.empty()) {
            machine.Complete();
            return;
        }

        machine.ChangeToRaster();
    }

private:
    using varyings_t = typename machine_t::varyings_type;
    using Triangle = std::array<varyings_t, 3>;
    using Math = PipelineMathHelper;

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

template<typename machine_t>
class RasterStage final : public base::State<machine_t, PipelineStageId> {
public:
    PipelineStageId GetID() const override
    {
        return PipelineStageId::Raster;
    }

    void OnExcute(machine_t& machine) override
    {
        auto& clipData = machine.ClipData();
        auto& rasterData = machine.RasterData();
        rasterData.hasTriangle = false;

        while (clipData.currentTriangleIndex < clipData.clippedTriangles.size()) {
            if (PrepareTriangle(
                    machine.FramebufferRef(),
                    clipData.clippedTriangles[clipData.currentTriangleIndex],
                    machine.ProgramRef().faceCullMode,
                    rasterData)) {
                machine.ChangeToFragment();
                return;
            }

            ++clipData.currentTriangleIndex;
        }

        machine.Complete();
    }

private:
    using varyings_t = typename machine_t::varyings_type;
    using Triangle = std::array<varyings_t, 3>;
    using ScreenTriangle = typename RasterStageData<varyings_t>::ScreenTriangle;
    using Math = PipelineMathHelper;

    static bool PrepareTriangle(const Framebuffer& framebuffer,
        const Triangle& triangle,
        FaceCullMode faceCullMode,
        RasterStageData<varyings_t>& rasterData)
    {
        ScreenVertex<varyings_t> v0{ triangle[0], {} };
        ScreenVertex<varyings_t> v1{ triangle[1], {} };
        ScreenVertex<varyings_t> v2{ triangle[2], {} };

        if (!FinalizeVertex(framebuffer, v0)
            || !FinalizeVertex(framebuffer, v1)
            || !FinalizeVertex(framebuffer, v2)) {
            return false;
        }

        const float area = Math::CalculateTriangleArea(v0.screen.position, v1.screen.position, v2.screen.position);
        if (Math::IsDegenerateTriangle(area) || ShouldCullTriangle(area, faceCullMode)) {
            return false;
        }

        rasterData.screenTriangle = ScreenTriangle{ v0, v1, v2 };
        rasterData.bounds = CalculateRasterBounds(framebuffer, v0, v1, v2, area);
        rasterData.hasTriangle = true;
        return true;
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

    static bool FinalizeVertex(const Framebuffer& framebuffer, ScreenVertex<varyings_t>& vertex)
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

    static bool ShouldCullTriangle(float area, FaceCullMode faceCullMode)
    {
        if (faceCullMode == FaceCullMode::None) {
            return false;
        }

        return area <= Math::kAreaEpsilon;
    }

    static RasterBounds CalculateRasterBounds(const Framebuffer& framebuffer,
        const ScreenVertex<varyings_t>& v0,
        const ScreenVertex<varyings_t>& v1,
        const ScreenVertex<varyings_t>& v2,
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
};

template<typename machine_t>
class FragmentStage final : public base::State<machine_t, PipelineStageId> {
public:
    PipelineStageId GetID() const override
    {
        return PipelineStageId::Fragment;
    }

    void OnExcute(machine_t& machine) override
    {
        auto& rasterData = machine.RasterData();
        if (rasterData.hasTriangle) {
            RasterizeTriangle(
                machine.ProgramRef(),
                machine.UniformsRef(),
                machine.FramebufferRef(),
                rasterData);
        }

        rasterData.hasTriangle = false;
        ++machine.ClipData().currentTriangleIndex;

        if (machine.ClipData().currentTriangleIndex >= machine.ClipData().clippedTriangles.size()) {
            machine.Complete();
            return;
        }

        machine.ChangeToRaster();
    }

private:
    using vertex_t = typename machine_t::vertex_type;
    using uniforms_t = typename machine_t::uniforms_type;
    using varyings_t = typename machine_t::varyings_type;
    using Math = PipelineMathHelper;

    static constexpr std::size_t kBaseFloatCount = sizeof(VaryingsBase) / sizeof(float);
    static constexpr std::size_t kTotalFloatCount = sizeof(varyings_t) / sizeof(float);

    static varyings_t InterpolateVaryings(const Framebuffer& framebuffer,
        const ScreenVertex<varyings_t>& v0,
        const ScreenVertex<varyings_t>& v1,
        const ScreenVertex<varyings_t>& v2,
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
            const float reciprocalW = 1.0f / clipW;
            out.ndcPos = out.clipPos * reciprocalW;
            out.ndcPos.w = reciprocalW;

            const float width = static_cast<float>(framebuffer.Width() - 1);
            const float height = static_cast<float>(framebuffer.Height() - 1);
            out.fragPos = math::Vec4{
                (out.ndcPos.x * 0.5f + 0.5f) * width,
                (0.5f - out.ndcPos.y * 0.5f) * height,
                math::Clamp(out.ndcPos.z * 0.5f + 0.5f, 0.0f, 1.0f),
                out.ndcPos.w,
            };
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

    static void ExecuteFragmentSamples(int x,
        int y,
        const varyings_t& varyings,
        const Program<vertex_t, uniforms_t, varyings_t>& program,
        const uniforms_t& uniforms,
        Framebuffer& framebuffer,
        const std::array<bool, Framebuffer::kSampleCount>& sampleCoverage)
    {
        const float depth = varyings.fragPos.z;
        bool hasVisibleSample = false;
        for (std::size_t sampleIndex = 0; sampleIndex < Framebuffer::kSampleCount; ++sampleIndex) {
            if (!sampleCoverage[sampleIndex]) {
                continue;
            }

            if (depth < framebuffer.GetSampleDepth(x, y, sampleIndex)) {
                hasVisibleSample = true;
                break;
            }
        }

        if (!hasVisibleSample) {
            return;
        }

        const Color fragmentColor = program.fragmentShader(varyings, uniforms);
        for (std::size_t sampleIndex = 0; sampleIndex < Framebuffer::kSampleCount; ++sampleIndex) {
            if (!sampleCoverage[sampleIndex]) {
                continue;
            }

            if (depth >= framebuffer.GetSampleDepth(x, y, sampleIndex)) {
                continue;
            }

            framebuffer.SetSampleDepth(x, y, sampleIndex, depth);
            framebuffer.SetSamplePixel(x, y, sampleIndex, fragmentColor);
        }
    }

    static void RasterizeTriangle(const Program<vertex_t, uniforms_t, varyings_t>& program,
        const uniforms_t& uniforms,
        Framebuffer& framebuffer,
        const RasterStageData<varyings_t>& rasterData)
    {
        const auto& v0 = rasterData.screenTriangle[0];
        const auto& v1 = rasterData.screenTriangle[1];
        const auto& v2 = rasterData.screenTriangle[2];
        const RasterBounds& bounds = rasterData.bounds;

        for (int y = bounds.startY; y <= bounds.endY; ++y) {
            for (int x = bounds.startX; x <= bounds.endX; ++x) {
                std::array<bool, Framebuffer::kSampleCount> sampleCoverage{};
                bool anyCovered = false;

                for (std::size_t sampleIndex = 0; sampleIndex < Framebuffer::kSampleCount; ++sampleIndex) {
                    const math::Vec2 samplePoint = math::Vec2{
                        static_cast<float>(x),
                        static_cast<float>(y)
                    } + Framebuffer::SampleOffset(sampleIndex);
                    const float e0 = Math::EdgeFunction(v1.screen.position, v2.screen.position, samplePoint);
                    const float e1 = Math::EdgeFunction(v2.screen.position, v0.screen.position, samplePoint);
                    const float e2 = Math::EdgeFunction(v0.screen.position, v1.screen.position, samplePoint);
                    const bool covered = Math::IsSampleCovered(e0, e1, e2, bounds.positiveArea);
                    sampleCoverage[sampleIndex] = covered;
                    anyCovered = anyCovered || covered;
                }

                if (!anyCovered) {
                    continue;
                }

                const math::Vec2 pixelCenter = Math::CalculateSamplePoint(x, y);
                const float e0 = Math::EdgeFunction(v1.screen.position, v2.screen.position, pixelCenter);
                const float e1 = Math::EdgeFunction(v2.screen.position, v0.screen.position, pixelCenter);
                const float e2 = Math::EdgeFunction(v0.screen.position, v1.screen.position, pixelCenter);
                const float w0 = e0 / bounds.area;
                const float w1 = e1 / bounds.area;
                const float w2 = e2 / bounds.area;
                const varyings_t varyings = InterpolateVaryings(framebuffer, v0, v1, v2, w0, w1, w2);
                ExecuteFragmentSamples(
                    x,
                    y,
                    varyings,
                    program,
                    uniforms,
                    framebuffer,
                    sampleCoverage);
            }
        }
    }
};

template<typename vertex_t, typename uniforms_t, typename varyings_t>
class PipelineStageMachine
    : public base::StateMachine<PipelineStageMachine<vertex_t, uniforms_t, varyings_t>, PipelineStageId> {
public:
    using vertex_type = vertex_t;
    using uniforms_type = uniforms_t;
    using varyings_type = varyings_t;
    using vertex_triangle_t = std::array<varyings_t, 3>;
    using input_triangle_t = std::array<vertex_t, 3>;
    using Base = base::StateMachine<PipelineStageMachine<vertex_t, uniforms_t, varyings_t>, PipelineStageId>;

    PipelineStageMachine(Framebuffer& framebuffer,
        const Program<vertex_t, uniforms_t, varyings_t>& program,
        const uniforms_t& uniforms,
        input_triangle_t inputTriangle)
        : m_Framebuffer(framebuffer),
          m_Program(program),
          m_Uniforms(uniforms),
          m_InputTriangle(std::move(inputTriangle))
    {
    }

    void Run()
    {
        m_Result = PipelineStageId::Invalid;
        Base::SetInitialState(*this, m_VertexState);
        while (Base::HasState()) {
            Base::UpdateCurrentState(*this);
        }
    }

    PipelineStageId Result() const
    {
        return m_Result;
    }

    const input_triangle_t& InputTriangle() const
    {
        return m_InputTriangle;
    }

    const Program<vertex_t, uniforms_t, varyings_t>& ProgramRef() const
    {
        return m_Program;
    }

    const uniforms_t& UniformsRef() const
    {
        return m_Uniforms;
    }

    Framebuffer& FramebufferRef() const
    {
        return m_Framebuffer;
    }

    VertexStageData<varyings_t>& VertexData()
    {
        return m_VertexData;
    }

    ClipStageData<varyings_t>& ClipData()
    {
        return m_ClipData;
    }

    RasterStageData<varyings_t>& RasterData()
    {
        return m_RasterData;
    }

    void ChangeToClip()
    {
        Base::ChangeState(*this, m_ClipState);
    }

    void ChangeToRaster()
    {
        Base::ChangeState(*this, m_RasterState);
    }

    void ChangeToFragment()
    {
        Base::ChangeState(*this, m_FragmentState);
    }

    void Complete()
    {
        m_Result = PipelineStageId::Completed;
        Base::ClearState(*this);
    }

    void Invalidate()
    {
        m_Result = PipelineStageId::Invalid;
        Base::ClearState(*this);
    }

private:
    Framebuffer& m_Framebuffer;
    const Program<vertex_t, uniforms_t, varyings_t>& m_Program;
    const uniforms_t& m_Uniforms;
    input_triangle_t m_InputTriangle;

    VertexStageData<varyings_t> m_VertexData{};
    ClipStageData<varyings_t> m_ClipData{};
    RasterStageData<varyings_t> m_RasterData{};
    PipelineStageId m_Result = PipelineStageId::Invalid;

    VertexStage<PipelineStageMachine> m_VertexState{};
    ClipStage<PipelineStageMachine> m_ClipState{};
    RasterStage<PipelineStageMachine> m_RasterState{};
    FragmentStage<PipelineStageMachine> m_FragmentState{};
};

} // namespace sr::render::detail
