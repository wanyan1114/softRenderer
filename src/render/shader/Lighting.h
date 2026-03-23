#pragma once

#include "base/Math.h"
#include "render/Color.h"

#include <algorithm>
#include <cmath>

namespace sr::render {

inline float Saturate(float value)
{
    return math::Clamp(value, 0.0f, 1.0f);
}

inline math::Vec3 ColorToVec3(const Color& color)
{
    return { color.RedFloat(), color.GreenFloat(), color.BlueFloat() };
}

inline Color Vec3ToColor(const math::Vec3& value, float alpha = 1.0f)
{
    return Color::FromFloats(value.x, value.y, value.z, alpha);
}

inline math::Vec3 Hadamard(const math::Vec3& lhs, const math::Vec3& rhs)
{
    return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
}

inline math::Vec3 TonemapReinhard(const math::Vec3& value)
{
    return {
        value.x / (1.0f + value.x),
        value.y / (1.0f + value.y),
        value.z / (1.0f + value.z),
    };
}

inline math::Vec3 LinearToSrgb(const math::Vec3& value)
{
    constexpr float kInvGamma = 1.0f / 2.2f;
    return {
        std::pow(Saturate(value.x), kInvGamma),
        std::pow(Saturate(value.y), kInvGamma),
        std::pow(Saturate(value.z), kInvGamma),
    };
}

inline math::Vec3 Reflect(const math::Vec3& incident, const math::Vec3& normal)
{
    return incident - 2.0f * incident.Dot(normal) * normal;
}

inline math::Vec3 FresnelSchlick(float cosTheta, const math::Vec3& f0)
{
    const float factor = std::pow(1.0f - Saturate(cosTheta), 5.0f);
    return f0 + (math::Vec3{ 1.0f, 1.0f, 1.0f } - f0) * factor;
}

inline math::Vec3 FresnelSchlickRoughness(float cosTheta,
    const math::Vec3& f0,
    float roughness)
{
    const math::Vec3 maxValue{
        std::max(1.0f - roughness, f0.x),
        std::max(1.0f - roughness, f0.y),
        std::max(1.0f - roughness, f0.z),
    };
    return f0 + (maxValue - f0) * std::pow(1.0f - Saturate(cosTheta), 5.0f);
}

inline float DistributionGGX(const math::Vec3& normal, const math::Vec3& halfDir, float roughness)
{
    const float a = roughness * roughness;
    const float a2 = a * a;
    const float noH = Saturate(normal.Dot(halfDir));
    const float noH2 = noH * noH;
    const float denom = noH2 * (a2 - 1.0f) + 1.0f;
    return a2 / std::max(math::kPi * denom * denom, 0.0001f);
}

inline float GeometrySchlickGGX(float nDotV, float roughness)
{
    const float r = roughness + 1.0f;
    const float k = (r * r) / 8.0f;
    return nDotV / std::max(nDotV * (1.0f - k) + k, 0.0001f);
}

inline float GeometrySmith(float noV, float noL, float roughness)
{
    return GeometrySchlickGGX(noV, roughness) * GeometrySchlickGGX(noL, roughness);
}

inline math::Vec3 BuildF0(const math::Vec3& albedo, float metallic)
{
    return math::Vec3::Lerp(math::Vec3{ 0.04f, 0.04f, 0.04f }, albedo, Saturate(metallic));
}

inline math::Vec3 BuildHemiAmbientColor(const math::Vec3& normal,
    const Color& skyAmbientColor,
    const Color& groundAmbientColor,
    float ambientStrength)
{
    const float hemiFactor = Saturate(normal.y * 0.5f + 0.5f);
    return math::Vec3::Lerp(
        ColorToVec3(groundAmbientColor),
        ColorToVec3(skyAmbientColor),
        hemiFactor) * ambientStrength;
}

inline Color ApplyBlinnPhongLighting(const Color& baseColor,
    const math::Vec3& worldPos,
    const math::Vec3& worldNormal,
    const math::Vec3& lightPosWorld,
    const math::Vec3& cameraPosWorld,
    const Color& skyAmbientColor,
    const Color& groundAmbientColor,
    float ambientStrength,
    float specularStrength,
    float shininess)
{
    const math::Vec3 normal = worldNormal.Normalized();
    const math::Vec3 lightDir = (lightPosWorld - worldPos).Normalized();
    const math::Vec3 viewDir = (cameraPosWorld - worldPos).Normalized();
    const math::Vec3 halfDir = (lightDir + viewDir).Normalized();

    const float diffuse = Saturate(normal.Dot(lightDir));
    float specular = 0.0f;
    if (diffuse > 0.0f) {
        specular = std::pow(Saturate(normal.Dot(halfDir)), shininess);
    }

    const math::Vec3 ambient = BuildHemiAmbientColor(
        normal,
        skyAmbientColor,
        groundAmbientColor,
        ambientStrength);

    const float red = baseColor.RedFloat() * (ambient.x + diffuse)
        + specularStrength * specular;
    const float green = baseColor.GreenFloat() * (ambient.y + diffuse)
        + specularStrength * specular;
    const float blue = baseColor.BlueFloat() * (ambient.z + diffuse)
        + specularStrength * specular;

    return Color::FromFloats(red, green, blue, baseColor.AlphaFloat());
}

inline float SampleScalarTexture(const Texture2D* texture,
    const math::Vec2& uv,
    float factor)
{
    if (texture == nullptr) {
        return Saturate(factor);
    }

    const Color sample = texture->Sample(uv);
    const float scalar = (sample.RedFloat() + sample.GreenFloat() + sample.BlueFloat()) / 3.0f;
    return Saturate(scalar * factor);
}

inline math::Vec3 SampleAlbedoTexture(const Texture2D* texture,
    const math::Vec2& uv,
    const Color& factor)
{
    const math::Vec3 factorVec = ColorToVec3(factor);
    if (texture == nullptr) {
        return factorVec;
    }

    return Hadamard(factorVec, ColorToVec3(texture->Sample(uv)));
}

inline math::Vec3 EvaluateDirectPbr(const math::Vec3& albedo,
    float metallic,
    float roughness,
    const math::Vec3& worldPos,
    const math::Vec3& worldNormal,
    const math::Vec3& lightPosWorld,
    const math::Vec3& cameraPosWorld,
    const math::Vec3& lightColor,
    float lightIntensity)
{
    const math::Vec3 normal = worldNormal.Normalized();
    const math::Vec3 viewDir = (cameraPosWorld - worldPos).Normalized();
    const math::Vec3 lightVector = lightPosWorld - worldPos;
    const float lightDistance = std::max(lightVector.Length(), 0.001f);
    const math::Vec3 lightDir = lightVector / lightDistance;
    const math::Vec3 halfDir = (viewDir + lightDir).Normalized();

    const float noL = Saturate(normal.Dot(lightDir));
    const float noV = Saturate(normal.Dot(viewDir));
    const float voH = Saturate(viewDir.Dot(halfDir));
    if (noL <= 0.0f || noV <= 0.0f) {
        return {};
    }

    const float clampedMetallic = Saturate(metallic);
    const float clampedRoughness = math::Clamp(roughness, 0.045f, 1.0f);
    const float attenuation = lightIntensity / (lightDistance * lightDistance);
    const math::Vec3 radiance = lightColor * attenuation;
    const math::Vec3 f0 = BuildF0(albedo, clampedMetallic);

    const math::Vec3 fresnel = FresnelSchlick(voH, f0);
    const float distribution = DistributionGGX(normal, halfDir, clampedRoughness);
    const float geometry = GeometrySmith(noV, noL, clampedRoughness);
    const math::Vec3 specular = fresnel * (distribution * geometry / std::max(4.0f * noV * noL, 0.001f));

    const math::Vec3 kS = fresnel;
    const math::Vec3 kD = (math::Vec3{ 1.0f, 1.0f, 1.0f } - kS) * (1.0f - clampedMetallic);
    const math::Vec3 diffuse = Hadamard(kD, albedo) * (1.0f / math::kPi);
    return Hadamard(diffuse + specular, radiance) * noL;
}

inline Color ApplyDirectPbrLighting(const math::Vec3& albedo,
    float metallic,
    float roughness,
    float ao,
    const math::Vec3& worldPos,
    const math::Vec3& worldNormal,
    const math::Vec3& lightPosWorld,
    const math::Vec3& cameraPosWorld,
    const math::Vec3& lightColor,
    float lightIntensity,
    const Color& skyAmbientColor,
    const Color& groundAmbientColor,
    float ambientStrength,
    float alpha = 1.0f)
{
    const math::Vec3 normal = worldNormal.Normalized();
    const float clampedMetallic = Saturate(metallic);
    const float clampedRoughness = math::Clamp(roughness, 0.045f, 1.0f);
    const float clampedAo = Saturate(ao);

    const math::Vec3 direct = EvaluateDirectPbr(
        albedo,
        clampedMetallic,
        clampedRoughness,
        worldPos,
        normal,
        lightPosWorld,
        cameraPosWorld,
        lightColor,
        lightIntensity);

    const math::Vec3 viewDir = (cameraPosWorld - worldPos).Normalized();
    const math::Vec3 f0 = BuildF0(albedo, clampedMetallic);
    const math::Vec3 ambientColor = BuildHemiAmbientColor(
        normal,
        skyAmbientColor,
        groundAmbientColor,
        ambientStrength);
    const math::Vec3 fresnel = FresnelSchlickRoughness(Saturate(normal.Dot(viewDir)), f0, clampedRoughness);
    const math::Vec3 kD = (math::Vec3{ 1.0f, 1.0f, 1.0f } - fresnel) * (1.0f - clampedMetallic);
    const math::Vec3 ambientDiffuse = Hadamard(Hadamard(kD, albedo), ambientColor) * clampedAo;

    // Keep a small non-IBL specular floor so metallic areas remain readable in direct-PBR mode.
    const float ambientSpecularStrength = 0.08f + (1.0f - clampedRoughness) * 0.14f;
    const math::Vec3 ambientSpecular = Hadamard(ambientColor, fresnel) * (ambientSpecularStrength * clampedAo);

    const math::Vec3 linearColor = ambientDiffuse + ambientSpecular + direct;
    const math::Vec3 displayColor = LinearToSrgb(TonemapReinhard(linearColor));
    return Vec3ToColor(displayColor, alpha);
}

} // namespace sr::render
