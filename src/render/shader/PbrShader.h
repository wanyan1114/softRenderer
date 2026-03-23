#pragma once

#include "render/VertexTypes.h"
#include "render/shader/Lighting.h"
#include "render/shader/ShaderTypes.h"

namespace sr::render {

inline void PbrVertexShader(PbrVaryings& varyings,
    const LitVertex& vertex,
    const PbrUniforms& uniforms)
{
    varyings.clipPos = uniforms.mvp * math::Vec4(vertex.positionModel, 1.0f);
    varyings.worldPos = TransformPosition(uniforms.model, vertex.positionModel);
    varyings.worldNormal = TransformDirection(uniforms.model, vertex.normalModel).Normalized();
    varyings.uv = vertex.uv;
}

inline Color PbrFragmentShader(const PbrVaryings& varyings,
    const PbrUniforms& uniforms)
{
    const math::Vec3 albedo = SampleAlbedoTexture(uniforms.albedoTexture, varyings.uv, uniforms.albedoColor);
    const float metallic = SampleScalarTexture(uniforms.metallicTexture, varyings.uv, uniforms.metallicFactor);
    const float roughness = math::Clamp(
        SampleScalarTexture(uniforms.roughnessTexture, varyings.uv, uniforms.roughnessFactor),
        0.045f,
        1.0f);
    const float ao = SampleScalarTexture(uniforms.aoTexture, varyings.uv, uniforms.aoFactor);

    return ApplyDirectPbrLighting(
        albedo,
        metallic,
        roughness,
        ao,
        varyings.worldPos,
        varyings.worldNormal,
        uniforms.lightPosWorld,
        uniforms.cameraPosWorld,
        uniforms.lightColor,
        uniforms.lightIntensity,
        uniforms.skyAmbientColor,
        uniforms.groundAmbientColor,
        uniforms.ambientStrength);
}

} // namespace sr::render
