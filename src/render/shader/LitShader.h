#pragma once

#include "render/VertexTypes.h"
#include "render/shader/Lighting.h"
#include "render/shader/ShaderTypes.h"

namespace sr::render {

inline void LitVertexShader(LitVaryings& varyings,
    const LitVertex& vertex,
    const LitUniforms& uniforms)
{
    varyings.clipPos = uniforms.mvp * math::Vec4(vertex.positionModel, 1.0f);
    varyings.worldPos = TransformPosition(uniforms.model, vertex.positionModel);
    varyings.worldNormal = TransformDirection(uniforms.model, vertex.normalModel).Normalized();
    varyings.uv = vertex.uv;
}

inline Color LitFragmentShader(const LitVaryings& varyings,
    const LitUniforms& uniforms)
{
    Color baseColor = uniforms.color;
    if (uniforms.textureEnabled > 0.5f && uniforms.baseColorTexture != nullptr) {
        baseColor = uniforms.baseColorTexture->Sample(varyings.uv);
    }

    return ApplyBlinnPhongLighting(
        baseColor,
        varyings.worldPos,
        varyings.worldNormal,
        uniforms.lightPosWorld,
        uniforms.cameraPosWorld,
        uniforms.skyAmbientColor,
        uniforms.groundAmbientColor,
        uniforms.ambientStrength,
        uniforms.specularStrength,
        uniforms.shininess);
}

} // namespace sr::render
