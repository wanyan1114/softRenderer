#pragma once

#include "base/Math.h"
#include "render/Color.h"

#include <cmath>

namespace sr::render {

namespace detail {

inline bool PointInRect(const math::Vec2& point,
    float minX,
    float minY,
    float maxX,
    float maxY)
{
    return point.x >= minX && point.x <= maxX
        && point.y >= minY && point.y <= maxY;
}

inline bool DigitUsesSevenSegment(int digit, int segment)
{
    switch (digit) {
    case 1:
        return segment == 1 || segment == 2;
    case 2:
        return segment == 0 || segment == 1 || segment == 3 || segment == 4 || segment == 6;
    case 3:
        return segment == 0 || segment == 1 || segment == 2 || segment == 3 || segment == 6;
    case 4:
        return segment == 1 || segment == 2 || segment == 5 || segment == 6;
    case 5:
        return segment == 0 || segment == 2 || segment == 3 || segment == 5 || segment == 6;
    case 6:
        return segment == 0 || segment == 2 || segment == 3 || segment == 4 || segment == 5 || segment == 6;
    default:
        return false;
    }
}

inline bool IsDigitPixel(int digit, const math::Vec2& faceUv)
{
    constexpr float kMinX = 0.28f;
    constexpr float kMaxX = 0.72f;
    constexpr float kMinY = 0.18f;
    constexpr float kMaxY = 0.82f;
    if (!PointInRect(faceUv, kMinX, kMinY, kMaxX, kMaxY)) {
        return false;
    }

    const math::Vec2 digitUv{
        (faceUv.x - kMinX) / (kMaxX - kMinX),
        (faceUv.y - kMinY) / (kMaxY - kMinY),
    };

    constexpr float kLeftX0 = 0.14f;
    constexpr float kLeftX1 = 0.30f;
    constexpr float kRightX0 = 0.70f;
    constexpr float kRightX1 = 0.86f;
    constexpr float kMidX0 = 0.24f;
    constexpr float kMidX1 = 0.76f;

    constexpr float kTopY0 = 0.84f;
    constexpr float kTopY1 = 1.00f;
    constexpr float kUpperY0 = 0.52f;
    constexpr float kUpperY1 = 0.84f;
    constexpr float kMiddleY0 = 0.42f;
    constexpr float kMiddleY1 = 0.58f;
    constexpr float kLowerY0 = 0.16f;
    constexpr float kLowerY1 = 0.48f;
    constexpr float kBottomY0 = 0.00f;
    constexpr float kBottomY1 = 0.16f;

    const bool segments[7] = {
        PointInRect(digitUv, kMidX0, kTopY0, kMidX1, kTopY1),
        PointInRect(digitUv, kRightX0, kUpperY0, kRightX1, kUpperY1),
        PointInRect(digitUv, kRightX0, kLowerY0, kRightX1, kLowerY1),
        PointInRect(digitUv, kMidX0, kBottomY0, kMidX1, kBottomY1),
        PointInRect(digitUv, kLeftX0, kLowerY0, kLeftX1, kLowerY1),
        PointInRect(digitUv, kLeftX0, kUpperY0, kLeftX1, kUpperY1),
        PointInRect(digitUv, kMidX0, kMiddleY0, kMidX1, kMiddleY1),
    };

    for (int segment = 0; segment < 7; ++segment) {
        if (segments[segment] && DigitUsesSevenSegment(digit, segment)) {
            return true;
        }
    }

    return false;
}

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

} // namespace detail

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

struct LitVertex : public VertexBase {
    math::Vec3 normalModel{};

    constexpr LitVertex() = default;

    constexpr LitVertex(const math::Vec3& position, const math::Vec3& normal)
        : VertexBase(position),
          normalModel(normal)
    {
    }
};

struct LitUniforms : public UniformsBase {
    math::Mat4 model = math::Mat4::Identity();
    Color color{ 255, 184, 77 };
    math::Vec3 lightPosWorld{ 1.6f, 2.0f, 2.8f };
    math::Vec3 cameraPosWorld{ 0.0f, 0.0f, 2.6f };
    Color skyAmbientColor{ 196, 210, 232 };
    Color groundAmbientColor{ 156, 140, 148 };
    float ambientStrength = 0.42f;
    float specularStrength = 0.18f;
    float shininess = 28.0f;
};

struct LitVaryings : public VaryingsBase {
    math::Vec3 worldPos{};
    math::Vec3 worldNormal{};
};

inline void LitVertexShader(LitVaryings& varyings,
    const LitVertex& vertex,
    const LitUniforms& uniforms)
{
    varyings.clipPos = uniforms.mvp * math::Vec4(vertex.positionModel, 1.0f);
    varyings.worldPos = detail::TransformPosition(uniforms.model, vertex.positionModel);
    varyings.worldNormal = detail::TransformDirection(uniforms.model, vertex.normalModel).Normalized();
}

inline Color LitFragmentShader(const LitVaryings& varyings,
    const LitUniforms& uniforms)
{
    return detail::ApplyBlinnPhongLighting(
        uniforms.color,
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

struct DebugFaceVertex : public LitVertex {
    math::Vec2 faceUv{};

    constexpr DebugFaceVertex() = default;

    constexpr DebugFaceVertex(const math::Vec3& position,
        const math::Vec3& normal,
        const math::Vec2& uv)
        : LitVertex(position, normal),
          faceUv(uv)
    {
    }
};

struct DebugFaceUniforms : public LitUniforms {
    Color digitColor{ 248, 244, 238 };
    int faceNumber = 1;
    float lightingEnabled = 1.0f;
};

struct DebugFaceVaryings : public LitVaryings {
    math::Vec2 faceUv{};
};

inline void DebugFaceVertexShader(DebugFaceVaryings& varyings,
    const DebugFaceVertex& vertex,
    const DebugFaceUniforms& uniforms)
{
    varyings.clipPos = uniforms.mvp * math::Vec4(vertex.positionModel, 1.0f);
    varyings.worldPos = detail::TransformPosition(uniforms.model, vertex.positionModel);
    varyings.worldNormal = detail::TransformDirection(uniforms.model, vertex.normalModel).Normalized();
    varyings.faceUv = vertex.faceUv;
}

inline Color DebugFaceFragmentShader(const DebugFaceVaryings& varyings,
    const DebugFaceUniforms& uniforms)
{
    if (detail::IsDigitPixel(uniforms.faceNumber, varyings.faceUv)) {
        return uniforms.digitColor;
    }

    if (uniforms.lightingEnabled <= 0.5f) {
        return uniforms.color;
    }

    return detail::ApplyBlinnPhongLighting(
        uniforms.color,
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

