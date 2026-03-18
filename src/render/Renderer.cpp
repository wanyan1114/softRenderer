#include "render/Renderer.h"

#include <algorithm>
#include <cmath>

namespace sr::render {

Renderer::Renderer(Framebuffer& framebuffer)
    : m_Framebuffer(framebuffer)
{
}

void Renderer::Clear(const Color& color)
{
    m_Framebuffer.Clear(color);
}

void Renderer::DrawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2)
{
    RasterizeTriangle(
        ToScreenVertex(v0),
        ToScreenVertex(v1),
        ToScreenVertex(v2));
}

float Renderer::EdgeFunction(const math::Vec2& a, const math::Vec2& b,
    const math::Vec2& point)
{
    return (point.x - a.x) * (b.y - a.y) - (point.y - a.y) * (b.x - a.x);
}

Color Renderer::InterpolateColor(const ScreenVertex& v0, const ScreenVertex& v1,
    const ScreenVertex& v2, float w0, float w1, float w2)
{
    return Color::FromFloats(
        w0 * v0.color.RedFloat() + w1 * v1.color.RedFloat() + w2 * v2.color.RedFloat(),
        w0 * v0.color.GreenFloat() + w1 * v1.color.GreenFloat() + w2 * v2.color.GreenFloat(),
        w0 * v0.color.BlueFloat() + w1 * v1.color.BlueFloat() + w2 * v2.color.BlueFloat(),
        w0 * v0.color.AlphaFloat() + w1 * v1.color.AlphaFloat() + w2 * v2.color.AlphaFloat());
}

Renderer::ScreenVertex Renderer::ToScreenVertex(const Vertex& vertex) const
{
    const float width = static_cast<float>(m_Framebuffer.Width() - 1);
    const float height = static_cast<float>(m_Framebuffer.Height() - 1);

    return {
        math::Vec2{
            (vertex.positionNdc.x * 0.5f + 0.5f) * width,
            (0.5f - vertex.positionNdc.y * 0.5f) * height,
        },
        vertex.color,
    };
}

void Renderer::RasterizeTriangle(const ScreenVertex& v0, const ScreenVertex& v1,
    const ScreenVertex& v2)
{
    const float area = EdgeFunction(v0.position, v1.position, v2.position);
    if (area == 0.0f) {
        return;
    }

    const float minX = std::min({ v0.position.x, v1.position.x, v2.position.x });
    const float maxX = std::max({ v0.position.x, v1.position.x, v2.position.x });
    const float minY = std::min({ v0.position.y, v1.position.y, v2.position.y });
    const float maxY = std::max({ v0.position.y, v1.position.y, v2.position.y });

    const int startX = std::max(0, static_cast<int>(std::floor(minX)));
    const int endX = std::min(m_Framebuffer.Width() - 1, static_cast<int>(std::ceil(maxX)));
    const int startY = std::max(0, static_cast<int>(std::floor(minY)));
    const int endY = std::min(m_Framebuffer.Height() - 1, static_cast<int>(std::ceil(maxY)));
    const float inverseArea = 1.0f / area;

    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            const math::Vec2 samplePoint{
                static_cast<float>(x) + 0.5f,
                static_cast<float>(y) + 0.5f,
            };

            const float w0 = EdgeFunction(v1.position, v2.position, samplePoint) * inverseArea;
            const float w1 = EdgeFunction(v2.position, v0.position, samplePoint) * inverseArea;
            const float w2 = EdgeFunction(v0.position, v1.position, samplePoint) * inverseArea;

            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) {
                continue;
            }

            m_Framebuffer.SetPixel(x, y, InterpolateColor(v0, v1, v2, w0, w1, w2));
        }
    }
}

} // namespace sr::render
