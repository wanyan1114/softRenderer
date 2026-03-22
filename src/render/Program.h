#pragma once

#include "render/Color.h"

namespace sr::render {

enum class FaceCullMode {
    None,
    Back,
};

template<typename vertex_t, typename uniforms_t, typename varyings_t>
struct Program {
    using vertex_shader_t = void (*)(varyings_t&, const vertex_t&, const uniforms_t&);
    using fragment_shader_t = Color (*)(const varyings_t&, const uniforms_t&);

    vertex_shader_t vertexShader = nullptr;
    fragment_shader_t fragmentShader = nullptr;
    FaceCullMode faceCullMode = FaceCullMode::Back;
};

} // namespace sr::render
