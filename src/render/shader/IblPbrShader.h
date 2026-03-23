#pragma once

#include "render/EnvironmentMap.h"
#include "render/VertexTypes.h"
#include "render/shader/Lighting.h"
#include "render/shader/ShaderTypes.h"

namespace sr::render {

inline void IblPbrVertexShader(IblPbrVaryings& varyings,
    const LitVertex& vertex,
    const IblPbrUniforms& uniforms)
{
    varyings.clipPos = uniforms.mvp * math::Vec4(vertex.positionModel, 1.0f);
    varyings.worldPos = TransformPosition(uniforms.model, vertex.positionModel);
    varyings.worldNormal = TransformDirection(uniforms.model, vertex.normalModel).Normalized();
    varyings.uv = vertex.uv;
}

inline Color IblPbrFragmentShader(const IblPbrVaryings& varyings,
    const IblPbrUniforms& uniforms)
{
    const math::Vec3 albedo = SampleAlbedoTexture(uniforms.albedoTexture, varyings.uv, uniforms.albedoColor);
    const float metallic = SampleScalarTexture(uniforms.metallicTexture, varyings.uv, uniforms.metallicFactor);
    const float roughness = math::Clamp(
        SampleScalarTexture(uniforms.roughnessTexture, varyings.uv, uniforms.roughnessFactor),
        0.045f,
        1.0f);
    const float ao = SampleScalarTexture(uniforms.aoTexture, varyings.uv, uniforms.aoFactor);

    const math::Vec3 normal = varyings.worldNormal.Normalized();
    const math::Vec3 viewDir = (uniforms.cameraPosWorld - varyings.worldPos).Normalized();
    const float noV = Saturate(normal.Dot(viewDir));
    const math::Vec3 reflection = Reflect(-viewDir, normal).Normalized();
    const math::Vec3 f0 = BuildF0(albedo, metallic);
    const math::Vec3 fresnel = FresnelSchlickRoughness(noV, f0, roughness);
    const math::Vec3 kS = fresnel;
    const math::Vec3 kD = (math::Vec3{ 1.0f, 1.0f, 1.0f } - kS) * (1.0f - Saturate(metallic));

    math::Vec3 ambient{};
    if (uniforms.irradianceMap != nullptr) {
        const math::Vec3 irradiance = uniforms.irradianceMap->Sample(normal);
        ambient = ambient + Hadamard(Hadamard(kD, albedo), irradiance) * (ao * uniforms.iblStrength);
    }

    if (uniforms.prefilterMap != nullptr && uniforms.brdfLut != nullptr) {
        const float maxLod = static_cast<float>(std::max(uniforms.prefilterMap->LevelCount() - 1, 0));
        const math::Vec3 prefiltered = uniforms.prefilterMap->Sample(reflection, roughness * maxLod);
        const Color brdfSample = uniforms.brdfLut->Sample({ noV, roughness });
        const math::Vec3 specularIbl = prefiltered * (ao * uniforms.iblStrength);
        ambient = ambient + Hadamard(specularIbl, fresnel * brdfSample.RedFloat() + math::Vec3{ brdfSample.GreenFloat(), brdfSample.GreenFloat(), brdfSample.GreenFloat() });
    }

    const math::Vec3 direct = EvaluateDirectPbr(
        albedo,
        metallic,
        roughness,
        varyings.worldPos,
        normal,
        uniforms.lightPosWorld,
        uniforms.cameraPosWorld,
        uniforms.lightColor,
        uniforms.lightIntensity);

    const math::Vec3 linearColor = ambient + direct;
    const math::Vec3 displayColor = LinearToSrgb(TonemapReinhard(linearColor));
    return Vec3ToColor(displayColor);
}

} // namespace sr::render
