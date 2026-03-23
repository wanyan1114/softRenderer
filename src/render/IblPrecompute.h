#pragma once

#include "base/Math.h"
#include "render/EnvironmentMap.h"
#include "render/Texture2D.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace sr::render::ibl {

enum class EnvironmentPreset {
    Daylight = 0,
    Sunset,
    Studio,
};

inline const char* EnvironmentPresetName(EnvironmentPreset preset)
{
    switch (preset) {
    case EnvironmentPreset::Daylight:
        return "Daylight";
    case EnvironmentPreset::Sunset:
        return "Sunset";
    case EnvironmentPreset::Studio:
        return "Studio";
    }

    return "Unknown";
}

inline std::array<EnvironmentPreset, 3> AllEnvironmentPresets()
{
    return {
        EnvironmentPreset::Daylight,
        EnvironmentPreset::Sunset,
        EnvironmentPreset::Studio,
    };
}

inline math::Vec3 SaturateVec3(const math::Vec3& value)
{
    return {
        math::Clamp(value.x, 0.0f, 1.0f),
        math::Clamp(value.y, 0.0f, 1.0f),
        math::Clamp(value.z, 0.0f, 1.0f),
    };
}

inline math::Vec3 Hadamard(const math::Vec3& lhs, const math::Vec3& rhs)
{
    return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
}

inline math::Vec3 Reflect(const math::Vec3& incident, const math::Vec3& normal)
{
    return incident - 2.0f * incident.Dot(normal) * normal;
}

inline float RadicalInverseVdC(std::uint32_t bits)
{
    bits = (bits << 16U) | (bits >> 16U);
    bits = ((bits & 0x55555555U) << 1U) | ((bits & 0xAAAAAAAAU) >> 1U);
    bits = ((bits & 0x33333333U) << 2U) | ((bits & 0xCCCCCCCCU) >> 2U);
    bits = ((bits & 0x0F0F0F0FU) << 4U) | ((bits & 0xF0F0F0F0U) >> 4U);
    bits = ((bits & 0x00FF00FFU) << 8U) | ((bits & 0xFF00FF00U) >> 8U);
    return static_cast<float>(bits) * 2.3283064365386963e-10f;
}

inline math::Vec2 Hammersley(std::uint32_t index, std::uint32_t sampleCount)
{
    return {
        static_cast<float>(index) / static_cast<float>(std::max(sampleCount, 1U)),
        RadicalInverseVdC(index),
    };
}

inline math::Vec3 ImportanceSampleGGX(const math::Vec2& xi,
    const math::Vec3& normal,
    float roughness)
{
    const float a = roughness * roughness;
    const float phi = 2.0f * math::kPi * xi.x;
    const float cosTheta = std::sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));

    const math::Vec3 halfVector{
        std::cos(phi) * sinTheta,
        cosTheta,
        std::sin(phi) * sinTheta,
    };

    const math::Vec3 up = std::abs(normal.y) < 0.999f
        ? math::Vec3{ 0.0f, 1.0f, 0.0f }
        : math::Vec3{ 1.0f, 0.0f, 0.0f };
    const math::Vec3 tangent = up.Cross(normal).Normalized();
    const math::Vec3 bitangent = normal.Cross(tangent).Normalized();

    return (tangent * halfVector.x + normal * halfVector.y + bitangent * halfVector.z).Normalized();
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

inline math::Vec3 ProceduralEnvironmentColor(EnvironmentPreset preset, const math::Vec3& direction)
{
    const math::Vec3 dir = direction.Length() > 0.0f ? direction.Normalized() : math::Vec3{ 0.0f, 1.0f, 0.0f };

    math::Vec3 skyTop{};
    math::Vec3 skyHorizon{};
    math::Vec3 ground{};
    math::Vec3 sunColor{};
    math::Vec3 sunDirection{};
    float sunSharpness = 256.0f;
    float sunStrength = 1.0f;
    float hazeStrength = 0.0f;

    switch (preset) {
    case EnvironmentPreset::Daylight:
        skyTop = { 0.12f, 0.34f, 0.78f };
        skyHorizon = { 0.76f, 0.88f, 0.98f };
        ground = { 0.28f, 0.25f, 0.20f };
        sunColor = { 1.0f, 0.96f, 0.82f };
        sunDirection = math::Vec3{ 0.45f, 0.82f, 0.32f }.Normalized();
        sunSharpness = 768.0f;
        sunStrength = 2.8f;
        hazeStrength = 0.12f;
        break;
    case EnvironmentPreset::Sunset:
        skyTop = { 0.08f, 0.09f, 0.24f };
        skyHorizon = { 0.94f, 0.42f, 0.16f };
        ground = { 0.14f, 0.08f, 0.06f };
        sunColor = { 1.0f, 0.58f, 0.28f };
        sunDirection = math::Vec3{ -0.62f, 0.24f, 0.38f }.Normalized();
        sunSharpness = 448.0f;
        sunStrength = 3.4f;
        hazeStrength = 0.26f;
        break;
    case EnvironmentPreset::Studio:
        skyTop = { 0.16f, 0.16f, 0.18f };
        skyHorizon = { 0.56f, 0.56f, 0.58f };
        ground = { 0.05f, 0.05f, 0.05f };
        sunColor = { 1.0f, 1.0f, 1.0f };
        sunDirection = math::Vec3{ 0.18f, 0.94f, -0.24f }.Normalized();
        sunSharpness = 1024.0f;
        sunStrength = 2.2f;
        hazeStrength = 0.04f;
        break;
    }

    const float upFactor = math::Clamp(dir.y * 0.5f + 0.5f, 0.0f, 1.0f);
    const math::Vec3 skyGradient = math::Vec3::Lerp(skyHorizon, skyTop, std::pow(upFactor, 0.7f));
    const math::Vec3 groundGradient = math::Vec3::Lerp(ground * 1.2f, ground, std::pow(1.0f - upFactor, 0.6f));
    math::Vec3 color = dir.y >= 0.0f ? skyGradient : groundGradient;

    const float sunAmount = std::pow(std::max(dir.Dot(sunDirection), 0.0f), sunSharpness);
    color = color + sunColor * (sunStrength * sunAmount);

    const float horizonGlow = std::pow(1.0f - std::abs(dir.y), 6.0f) * hazeStrength;
    color = color + skyHorizon * horizonGlow;

    return SaturateVec3(color);
}

inline EnvironmentMap GenerateEnvironmentPreset(EnvironmentPreset preset,
    int width = 128,
    int height = 64)
{
    std::vector<Color> pixels;
    pixels.reserve(static_cast<std::size_t>(width * height));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(std::max(width - 1, 1));
            const float v = static_cast<float>(y) / static_cast<float>(std::max(height - 1, 1));
            const math::Vec3 direction = EnvironmentMap::UvToDirection({ u, v });
            const math::Vec3 color = ProceduralEnvironmentColor(preset, direction);
            pixels.push_back(Color::FromFloats(color.x, color.y, color.z));
        }
    }

    return EnvironmentMap(Texture2D(width, height, std::move(pixels)));
}

inline EnvironmentMap ConvoluteDiffuseIrradiance(const EnvironmentMap& source,
    int width = 32,
    int height = 16,
    int phiSteps = 24,
    int thetaSteps = 12)
{
    std::vector<Color> pixels;
    pixels.reserve(static_cast<std::size_t>(width * height));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(std::max(width - 1, 1));
            const float v = static_cast<float>(y) / static_cast<float>(std::max(height - 1, 1));
            const math::Vec3 normal = EnvironmentMap::UvToDirection({ u, v });

            const math::Vec3 up = std::abs(normal.y) < 0.999f
                ? math::Vec3{ 0.0f, 1.0f, 0.0f }
                : math::Vec3{ 1.0f, 0.0f, 0.0f };
            const math::Vec3 tangent = up.Cross(normal).Normalized();
            const math::Vec3 bitangent = normal.Cross(tangent).Normalized();

            math::Vec3 irradiance{};
            float totalWeight = 0.0f;

            for (int thetaIndex = 0; thetaIndex < thetaSteps; ++thetaIndex) {
                const float theta = (static_cast<float>(thetaIndex) + 0.5f)
                    / static_cast<float>(thetaSteps) * (math::kPi * 0.5f);
                const float sinTheta = std::sin(theta);
                const float cosTheta = std::cos(theta);

                for (int phiIndex = 0; phiIndex < phiSteps; ++phiIndex) {
                    const float phi = (static_cast<float>(phiIndex) + 0.5f)
                        / static_cast<float>(phiSteps) * (2.0f * math::kPi);
                    const math::Vec3 localSample{
                        std::cos(phi) * sinTheta,
                        cosTheta,
                        std::sin(phi) * sinTheta,
                    };
                    const math::Vec3 sampleDirection = (
                        tangent * localSample.x
                        + normal * localSample.y
                        + bitangent * localSample.z).Normalized();
                    const float weight = cosTheta * sinTheta;
                    irradiance = irradiance + source.Sample(sampleDirection) * weight;
                    totalWeight += weight;
                }
            }

            if (totalWeight > 0.0f) {
                irradiance = irradiance / totalWeight;
            }

            pixels.push_back(Color::FromFloats(irradiance.x, irradiance.y, irradiance.z));
        }
    }

    return EnvironmentMap(Texture2D(width, height, std::move(pixels)));
}

inline PrefilterEnvironmentMap PrefilterEnvironment(const EnvironmentMap& source,
    int baseWidth = 64,
    int baseHeight = 32,
    int mipCount = 5,
    std::uint32_t sampleCount = 64)
{
    std::vector<EnvironmentMap> levels;
    levels.reserve(static_cast<std::size_t>(mipCount));

    for (int levelIndex = 0; levelIndex < mipCount; ++levelIndex) {
        const float roughness = mipCount > 1
            ? static_cast<float>(levelIndex) / static_cast<float>(mipCount - 1)
            : 0.0f;
        const int width = std::max(1, baseWidth >> levelIndex);
        const int height = std::max(1, baseHeight >> levelIndex);
        std::vector<Color> pixels;
        pixels.reserve(static_cast<std::size_t>(width * height));

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const float u = static_cast<float>(x) / static_cast<float>(std::max(width - 1, 1));
                const float v = static_cast<float>(y) / static_cast<float>(std::max(height - 1, 1));
                const math::Vec3 reflection = EnvironmentMap::UvToDirection({ u, v });
                const math::Vec3 normal = reflection;
                const math::Vec3 view = reflection;

                math::Vec3 prefiltered{};
                float totalWeight = 0.0f;

                for (std::uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
                    const math::Vec2 xi = Hammersley(sampleIndex, sampleCount);
                    const math::Vec3 halfVector = ImportanceSampleGGX(xi, normal, std::max(roughness, 0.045f));
                    const math::Vec3 light = Reflect(-view, halfVector).Normalized();
                    const float noL = std::max(normal.Dot(light), 0.0f);
                    if (noL <= 0.0f) {
                        continue;
                    }

                    prefiltered = prefiltered + source.Sample(light) * noL;
                    totalWeight += noL;
                }

                if (totalWeight > 0.0f) {
                    prefiltered = prefiltered / totalWeight;
                }

                pixels.push_back(Color::FromFloats(prefiltered.x, prefiltered.y, prefiltered.z));
            }
        }

        levels.emplace_back(Texture2D(width, height, std::move(pixels)));
    }

    return PrefilterEnvironmentMap(std::move(levels));
}

inline math::Vec2 IntegrateBrdf(float noV, float roughness, std::uint32_t sampleCount)
{
    const math::Vec3 view{
        std::sqrt(std::max(0.0f, 1.0f - noV * noV)),
        noV,
        0.0f,
    };
    const math::Vec3 normal{ 0.0f, 1.0f, 0.0f };

    float resultA = 0.0f;
    float resultB = 0.0f;

    for (std::uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        const math::Vec2 xi = Hammersley(sampleIndex, sampleCount);
        const math::Vec3 halfVector = ImportanceSampleGGX(xi, normal, std::max(roughness, 0.045f));
        const math::Vec3 light = Reflect(-view, halfVector).Normalized();

        const float noL = std::max(light.y, 0.0f);
        const float noH = std::max(halfVector.y, 0.0f);
        const float voH = std::max(view.Dot(halfVector), 0.0f);
        if (noL <= 0.0f) {
            continue;
        }

        const float geometry = GeometrySmith(noV, noL, roughness);
        const float visibility = (geometry * voH) / std::max(noH * noV, 0.0001f);
        const float fresnel = std::pow(1.0f - voH, 5.0f);

        resultA += (1.0f - fresnel) * visibility;
        resultB += fresnel * visibility;
    }

    const float invSampleCount = 1.0f / static_cast<float>(std::max(sampleCount, 1U));
    return { resultA * invSampleCount, resultB * invSampleCount };
}

inline Texture2D IntegrateBrdfLut(int width = 64,
    int height = 64,
    std::uint32_t sampleCount = 64)
{
    std::vector<Color> pixels;
    pixels.reserve(static_cast<std::size_t>(width * height));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float noV = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
            const float roughness = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
            const math::Vec2 integrated = IntegrateBrdf(noV, roughness, sampleCount);
            pixels.push_back(Color::FromFloats(integrated.x, integrated.y, 0.0f));
        }
    }

    return Texture2D(width, height, std::move(pixels));
}

} // namespace sr::render::ibl
