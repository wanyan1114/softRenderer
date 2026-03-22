#pragma once

#include "base/Math.h"
#include "render/Color.h"
#include "render/Texture2D.h"

namespace sr::render {

struct UniformsBase {
    math::Mat4 mvp = math::Mat4::Identity();
};

struct VaryingsBase {
    math::Vec4 clipPos{};
    math::Vec4 ndcPos{};
    math::Vec4 fragPos{};
};

struct LitUniforms : public UniformsBase {
    math::Mat4 model = math::Mat4::Identity();
    Color color{ 255, 255, 255 };
    math::Vec3 lightPosWorld{ 1.6f, 2.0f, 2.8f };
    math::Vec3 cameraPosWorld{ 0.0f, 0.0f, 2.6f };
    Color skyAmbientColor{ 196, 210, 232 };
    Color groundAmbientColor{ 156, 140, 148 };
    float ambientStrength = 0.42f;
    float specularStrength = 0.18f;
    float shininess = 28.0f;
    const Texture2D* baseColorTexture = nullptr;
    float textureEnabled = 0.0f;
};

struct LitVaryings : public VaryingsBase {
    math::Vec3 worldPos{};
    math::Vec3 worldNormal{};
    math::Vec2 uv{};
};

inline math::Vec3 TransformPosition(const math::Mat4& matrix, const math::Vec3& position)
{
    const math::Vec4 transformed = matrix * math::Vec4(position, 1.0f);
    return { transformed.x, transformed.y, transformed.z };
}

inline math::Vec3 TransformDirection(const math::Mat4& matrix, const math::Vec3& direction)
{
    const math::Vec4 transformed = matrix * math::Vec4(direction, 0.0f);
    return { transformed.x, transformed.y, transformed.z };
}

} // namespace sr::render
