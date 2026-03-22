#include "render/Camera.h"

#include <cmath>

namespace sr::render {

math::Mat4 Camera::ViewMat4() const
{
    const math::Vec3 fallbackForward{ 0.0f, 0.0f, -1.0f };
    const math::Vec3 forward = Dir.Length() > 0.0f ? Dir.Normalized() : fallbackForward;
    const math::Vec3 normalizedUp = Up.Length() > 0.0f
        ? Up.Normalized()
        : math::Vec3{ 0.0f, 1.0f, 0.0f };
    const bool forwardIsNearlyVertical = std::abs(forward.y) >= 0.999f;
    const math::Vec3 lookAtUp = forwardIsNearlyVertical
        ? math::Vec3{ 1.0f, 0.0f, 0.0f }
        : normalizedUp;
    const math::Vec3 target = Pos + forward;

    return math::LookAt(Pos, target, lookAtUp);
}

math::Mat4 Camera::ProjectionMat4() const
{
    return math::Perspective(FovYRadians, Aspect, NearPlane, FarPlane);
}

} // namespace sr::render
