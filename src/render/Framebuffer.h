#pragma once

#include "render/Color.h"

#include "base/Math.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sr::render {

class Framebuffer {
public:
    static constexpr std::size_t kSampleCount = 4;

    Framebuffer(int width, int height);

    int Width() const { return m_Width; }
    int Height() const { return m_Height; }

    void Clear(const Color& color);
    void ClearDepth(float depth = 1.0f);
    void SetPixel(int x, int y, const Color& color);
    void SetDepth(int x, int y, float depth);
    void SetSamplePixel(int x, int y, std::size_t sampleIndex, const Color& color);
    void SetSampleDepth(int x, int y, std::size_t sampleIndex, float depth);

    std::uint32_t GetPixel(int x, int y) const;
    float GetDepth(int x, int y) const;
    std::uint32_t GetSamplePixel(int x, int y, std::size_t sampleIndex) const;
    float GetSampleDepth(int x, int y, std::size_t sampleIndex) const;
    const std::uint32_t* Data() const { return m_Pixels.data(); }

    std::uint32_t* MutablePixelData() { return m_Pixels.data(); }
    float* MutableDepthData() { return m_DepthBuffer.data(); }
    const float* DepthData() const { return m_DepthBuffer.data(); }
    std::size_t PixelIndexUnchecked(int x, int y) const
    {
        return static_cast<std::size_t>(y * m_Width + x);
    }
    std::size_t SampleIndexUnchecked(int x, int y, std::size_t sampleIndex) const
    {
        return PixelIndexUnchecked(x, y) * kSampleCount + sampleIndex;
    }

    static math::Vec2 SampleOffset(std::size_t sampleIndex);
    void ResolveColor();

private:
    bool InBounds(int x, int y) const;
    bool SampleIndexInRange(std::size_t sampleIndex) const;

    int m_Width;
    int m_Height;
    std::vector<std::uint32_t> m_Pixels;
    std::vector<float> m_DepthBuffer;
    std::vector<std::uint32_t> m_SamplePixels;
    std::vector<float> m_SampleDepthBuffer;
};

} // namespace sr::render
