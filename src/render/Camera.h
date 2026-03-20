#pragma once

#include "base/Math.h"

namespace sr::render {

class Camera {
public:
    math::Vec3 Pos{ 0.0f, 0.0f, 2.6f };
    math::Vec3 Right{ 1.0f, 0.0f, 0.0f };
    math::Vec3 Up{ 0.0f, 1.0f, 0.0f };
    math::Vec3 Dir{ 0.0f, 0.0f, -1.0f };
    float Aspect = 1.0f;
    float FovYRadians = math::kPi / 3.0f;
    float NearPlane = 0.1f;
    float FarPlane = 10.0f;

    math::Mat4 ViewMat4() const;
    math::Mat4 ProjectionMat4() const;
};

} // namespace sr::render
