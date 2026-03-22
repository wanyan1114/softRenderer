#pragma once

#include "base/Math.h"

namespace sr::render {

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

struct LitVertex : public VertexBase {
    math::Vec3 normalModel{};
    math::Vec2 uv{};

    constexpr LitVertex() = default;

    constexpr LitVertex(const math::Vec3& position,
        const math::Vec3& normal,
        const math::Vec2& uvValue = {})
        : VertexBase(position),
          normalModel(normal),
          uv(uvValue)
    {
    }
};

} // namespace sr::render
