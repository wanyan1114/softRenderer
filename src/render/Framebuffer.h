#pragma once

#include "render/Color.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sr::render {

class Framebuffer {
public:
    Framebuffer(int width, int height);

    int Width() const { return m_Width; }
    int Height() const { return m_Height; }

    void Clear(const Color& color);
    void ClearDepth(float depth = 1.0f);
    void SetPixel(int x, int y, const Color& color);
    void SetDepth(int x, int y, float depth);

    std::uint32_t GetPixel(int x, int y) const;
    float GetDepth(int x, int y) const;
    const std::uint32_t* Data() const { return m_Pixels.data(); }

    std::uint32_t* MutablePixelData() { return m_Pixels.data(); }
    float* MutableDepthData() { return m_DepthBuffer.data(); }
    const float* DepthData() const { return m_DepthBuffer.data(); }
    std::size_t PixelIndexUnchecked(int x, int y) const
    {
        return static_cast<std::size_t>(y * m_Width + x);
    }

private:
    bool InBounds(int x, int y) const;

    int m_Width;
    int m_Height;
    std::vector<std::uint32_t> m_Pixels;
    std::vector<float> m_DepthBuffer;
};

} // namespace sr::render
