#pragma once

#include "render/Camera.h"
#include "render/Color.h"
#include "render/EnvironmentMap.h"
#include "render/Framebuffer.h"

#include <algorithm>
#include <cmath>

namespace sr::render {

enum class SkyboxDisplayMode {
    Skybox = 0,
    Irradiance,
    Prefilter,
};

inline const char* SkyboxDisplayModeName(SkyboxDisplayMode mode)
{
    switch (mode) {
    case SkyboxDisplayMode::Skybox:
        return "Skybox";
    case SkyboxDisplayMode::Irradiance:
        return "Irradiance";
    case SkyboxDisplayMode::Prefilter:
        return "Prefilter";
    }

    return "Unknown";
}

inline math::Vec3 TonemapSkybox(const math::Vec3& value)
{
    return {
        value.x / (1.0f + value.x),
        value.y / (1.0f + value.y),
        value.z / (1.0f + value.z),
    };
}

inline math::Vec3 LinearToDisplay(const math::Vec3& value)
{
    constexpr float kInvGamma = 1.0f / 2.2f;
    return {
        std::pow(math::Clamp(value.x, 0.0f, 1.0f), kInvGamma),
        std::pow(math::Clamp(value.y, 0.0f, 1.0f), kInvGamma),
        std::pow(math::Clamp(value.z, 0.0f, 1.0f), kInvGamma),
    };
}

inline math::Vec3 CalculateSkyDirection(const Camera& camera,
    int pixelX,
    int pixelY,
    int width,
    int height)
{
    const float ndcX = (2.0f * (static_cast<float>(pixelX) + 0.5f) / static_cast<float>(std::max(width, 1))) - 1.0f;
    const float ndcY = 1.0f - (2.0f * (static_cast<float>(pixelY) + 0.5f) / static_cast<float>(std::max(height, 1)));
    const float tanHalfFov = std::tan(camera.FovYRadians * 0.5f);

    const math::Vec3 forward = camera.Dir.Length() > 0.0f ? camera.Dir.Normalized() : math::Vec3{ 0.0f, 0.0f, -1.0f };
    const math::Vec3 right = camera.Right.Length() > 0.0f ? camera.Right.Normalized() : math::Vec3{ 1.0f, 0.0f, 0.0f };
    const math::Vec3 up = camera.Up.Length() > 0.0f ? camera.Up.Normalized() : math::Vec3{ 0.0f, 1.0f, 0.0f };

    return (forward
        + right * (ndcX * camera.Aspect * tanHalfFov)
        + up * (ndcY * tanHalfFov)).Normalized();
}

inline void RenderSkybox(Framebuffer& framebuffer,
    const Camera& camera,
    const EnvironmentMap& environment)
{
    if (environment.Empty()) {
        return;
    }

    for (int y = 0; y < framebuffer.Height(); ++y) {
        for (int x = 0; x < framebuffer.Width(); ++x) {
            const math::Vec3 direction = CalculateSkyDirection(
                camera,
                x,
                y,
                framebuffer.Width(),
                framebuffer.Height());
            const math::Vec3 sampled = environment.Sample(direction);
            const math::Vec3 display = LinearToDisplay(TonemapSkybox(sampled));
            framebuffer.SetPixel(x, y, Color::FromFloats(display.x, display.y, display.z));
        }
    }
}

inline void RenderSkybox(Framebuffer& framebuffer,
    const Camera& camera,
    const PrefilterEnvironmentMap& environment,
    float lod)
{
    if (environment.Empty()) {
        return;
    }

    for (int y = 0; y < framebuffer.Height(); ++y) {
        for (int x = 0; x < framebuffer.Width(); ++x) {
            const math::Vec3 direction = CalculateSkyDirection(
                camera,
                x,
                y,
                framebuffer.Width(),
                framebuffer.Height());
            const math::Vec3 sampled = environment.Sample(direction, lod);
            const math::Vec3 display = LinearToDisplay(TonemapSkybox(sampled));
            framebuffer.SetPixel(x, y, Color::FromFloats(display.x, display.y, display.z));
        }
    }
}

} // namespace sr::render
