#include "render/Framebuffer.h"

#include <algorithm>

namespace sr::render {

Framebuffer::Framebuffer(int width, int height)
    : m_Width(width), m_Height(height),
      m_Pixels(static_cast<std::size_t>(width * height), 0U)
{
}

void Framebuffer::Clear(const Color& color)
{
    std::fill(m_Pixels.begin(), m_Pixels.end(), color.ToBgra8());
}

void Framebuffer::SetPixel(int x, int y, const Color& color)
{
    if (!InBounds(x, y)) {
        return;
    }

    m_Pixels[static_cast<std::size_t>(y * m_Width + x)] = color.ToBgra8();
}

std::uint32_t Framebuffer::GetPixel(int x, int y) const
{
    if (!InBounds(x, y)) {
        return 0U;
    }

    return m_Pixels[static_cast<std::size_t>(y * m_Width + x)];
}

bool Framebuffer::InBounds(int x, int y) const
{
    return x >= 0 && x < m_Width && y >= 0 && y < m_Height;
}

} // namespace sr::render
