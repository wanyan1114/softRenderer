#pragma once

#include "base/Math.h"
#include "render/Color.h"

namespace sr::render {

enum class FaceCullMode {
    None,
    Back,
};

struct VertexBase {
    math::Vec3 positionModel;

    constexpr VertexBase()
        : positionModel(0.0f, 0.0f, 0.0f)
    {
    }

    constexpr explicit VertexBase(const math::Vec3& position)
        : positionModel(position)
    {
    }
};

struct UniformsBase {
    math::Mat4 mvp = math::Mat4::Identity();
};

struct VaryingsBase {
    math::Vec4 clipPos{};
    math::Vec4 ndcPos{};
    math::Vec4 fragPos{};
};

template<typename vertex_t, typename uniforms_t, typename varyings_t>
struct Program {
    using vertex_shader_t = void (*)(varyings_t&, const vertex_t&, const uniforms_t&);
    using fragment_shader_t = Color (*)(const varyings_t&, const uniforms_t&);

    vertex_shader_t vertexShader = nullptr;
    fragment_shader_t fragmentShader = nullptr;
    FaceCullMode faceCullMode = FaceCullMode::Back;
};

struct FlatColorVertex : public VertexBase {
    using VertexBase::VertexBase;
};

struct FlatColorUniforms : public UniformsBase {
    Color color;
};

struct FlatColorVaryings : public VaryingsBase {
};

inline void FlatColorVertexShader(FlatColorVaryings& varyings,
    const FlatColorVertex& vertex,
    const FlatColorUniforms& uniforms)
{
    varyings.clipPos = uniforms.mvp * math::Vec4(vertex.positionModel, 1.0f);
}

inline Color FlatColorFragmentShader(const FlatColorVaryings&,
    const FlatColorUniforms& uniforms)
{
    return uniforms.color;
}

} // namespace sr::render
