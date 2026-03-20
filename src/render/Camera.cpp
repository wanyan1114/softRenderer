#include "render/Camera.h"

#include <cmath>

namespace sr::render {

math::Mat4 Camera::ViewMat4() const
{
    const math::Vec3 normalizedDir = Dir.Length() > 0.0f ? Dir.Normalized() : math::Vec3{ 0.0f, 0.0f, -1.0f };
    const math::Vec3 upVector = std::abs(normalizedDir.y) >= 0.999f
        ? math::Vec3{ 1.0f, 0.0f, 0.0f }
        : Up;
    return math::LookAt(Pos, Pos + normalizedDir, upVector);
}

math::Mat4 Camera::ProjectionMat4() const
{
    return math::Perspective(FovYRadians, Aspect, NearPlane, FarPlane);
}

} // namespace sr::render
