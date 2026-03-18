#pragma once

#include "render/Framebuffer.h"
#include "render/Vertex.h"

namespace sr::render {

class Renderer {
public:
    explicit Renderer(Framebuffer& framebuffer);

    void Clear(const Color& color);
    void DrawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2);

private:
    struct ScreenVertex {
        math::Vec2 position;
        Color color;
    };

    static float EdgeFunction(const math::Vec2& a, const math::Vec2& b,
        const math::Vec2& point);
    static Color InterpolateColor(const ScreenVertex& v0, const ScreenVertex& v1,
        const ScreenVertex& v2, float w0, float w1, float w2);

    ScreenVertex ToScreenVertex(const Vertex& vertex) const;
    void RasterizeTriangle(const ScreenVertex& v0, const ScreenVertex& v1,
        const ScreenVertex& v2);

    Framebuffer& m_Framebuffer;
};

} // namespace sr::render
