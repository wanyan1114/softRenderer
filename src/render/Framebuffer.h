#pragma once

#include "render/Color.h"

#include <cstdint>
#include <vector>

namespace sr::render {

class Framebuffer {
public:
    Framebuffer(int width, int height);

    int Width() const { return m_Width; }
    int Height() const { return m_Height; }

    void Clear(const Color& color);
    void SetPixel(int x, int y, const Color& color);

    std::uint32_t GetPixel(int x, int y) const;
    const std::uint32_t* Data() const { return m_Pixels.data(); }

private:
    bool InBounds(int x, int y) const;

    int m_Width;
    int m_Height;
    std::vector<std::uint32_t> m_Pixels;
};

} // namespace sr::render
