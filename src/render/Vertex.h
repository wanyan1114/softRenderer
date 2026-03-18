#pragma once

#include "base/Math.h"
#include "render/Color.h"

namespace sr::render {

struct Vertex {
    math::Vec2 positionNdc;
    Color color;
};

} // namespace sr::render
