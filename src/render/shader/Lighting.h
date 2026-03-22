#pragma once

#include "base/Math.h"
#include "render/Color.h"

#include <cmath>

namespace sr::render {

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

    const float diffuse = math::Clamp(normal.Dot(lightDir), 0.0f, 1.0f);
    float specular = 0.0f;
    if (diffuse > 0.0f) {
        specular = std::pow(math::Clamp(normal.Dot(halfDir), 0.0f, 1.0f), shininess);
    }

    const float hemiFactor = math::Clamp(normal.y * 0.5f + 0.5f, 0.0f, 1.0f);
    const float ambientRed = ambientStrength * math::Lerp(
        groundAmbientColor.RedFloat(),
        skyAmbientColor.RedFloat(),
        hemiFactor);
    const float ambientGreen = ambientStrength * math::Lerp(
        groundAmbientColor.GreenFloat(),
        skyAmbientColor.GreenFloat(),
        hemiFactor);
    const float ambientBlue = ambientStrength * math::Lerp(
        groundAmbientColor.BlueFloat(),
        skyAmbientColor.BlueFloat(),
        hemiFactor);

    const float red = baseColor.RedFloat() * (ambientRed + diffuse)
        + specularStrength * specular;
    const float green = baseColor.GreenFloat() * (ambientGreen + diffuse)
        + specularStrength * specular;
    const float blue = baseColor.BlueFloat() * (ambientBlue + diffuse)
        + specularStrength * specular;

    return Color::FromFloats(red, green, blue, baseColor.AlphaFloat());
}

} // namespace sr::render
