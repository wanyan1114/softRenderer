#pragma once

#include "base/Math.h"

#include <cmath>

namespace sr::render::detail {

struct PipelineMathHelper {
    static constexpr float kAreaEpsilon = 1e-6f;
    static constexpr float kClipEpsilon = 1e-6f;

    static float EdgeFunction(const math::Vec2& a,
        const math::Vec2& b,
        const math::Vec2& point)
    {
        return (point.x - a.x) * (b.y - a.y) - (point.y - a.y) * (b.x - a.x);
    }

    static float CalculateTriangleArea(const math::Vec2& a,
        const math::Vec2& b,
        const math::Vec2& c)
    {
        return EdgeFunction(a, b, c);
    }

    static bool IsDegenerateTriangle(float area)
    {
        return std::abs(area) <= kAreaEpsilon;
    }

    static math::Vec2 CalculateSamplePoint(int x, int y)
    {
        return math::Vec2{
            static_cast<float>(x) + 0.5f,
            static_cast<float>(y) + 0.5f,
        };
    }

    static bool IsSampleCovered(float e0, float e1, float e2, bool positiveArea)
    {
        if (positiveArea) {
            return e0 >= 0.0f && e1 >= 0.0f && e2 >= 0.0f;
        }

        return e0 <= 0.0f && e1 <= 0.0f && e2 <= 0.0f;
    }
};

} // namespace sr::render::detail

