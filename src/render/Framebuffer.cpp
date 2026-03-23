#include "render/Framebuffer.h"

#include <algorithm>
#include <array>

namespace sr::render {
namespace {

constexpr std::array<math::Vec2, Framebuffer::kSampleCount> kSampleOffsets{
    math::Vec2{ 0.375f, 0.125f },
    math::Vec2{ 0.875f, 0.375f },
    math::Vec2{ 0.125f, 0.625f },
    math::Vec2{ 0.625f, 0.875f },
};

float ResolveDepthValue(const std::vector<float>& sampleDepthBuffer,
    std::size_t sampleBaseIndex)
{
    float resolvedDepth = 1.0f;
    for (std::size_t sampleIndex = 0; sampleIndex < Framebuffer::kSampleCount; ++sampleIndex) {
        resolvedDepth = std::min(resolvedDepth, sampleDepthBuffer[sampleBaseIndex + sampleIndex]);
    }
    return resolvedDepth;
}

} // namespace

Framebuffer::Framebuffer(int width, int height)
    : m_Width(width),
      m_Height(height),
      m_Pixels(static_cast<std::size_t>(width * height), 0U),
      m_DepthBuffer(static_cast<std::size_t>(width * height), 1.0f),
      m_SamplePixels(static_cast<std::size_t>(width * height) * kSampleCount, 0U),
      m_SampleDepthBuffer(static_cast<std::size_t>(width * height) * kSampleCount, 1.0f)
{
}

void Framebuffer::Clear(const Color& color)
{
    const std::uint32_t packedColor = color.ToBgra8();
    std::fill(m_Pixels.begin(), m_Pixels.end(), packedColor);
    std::fill(m_SamplePixels.begin(), m_SamplePixels.end(), packedColor);
}

void Framebuffer::ClearDepth(float depth)
{
    std::fill(m_DepthBuffer.begin(), m_DepthBuffer.end(), depth);
    std::fill(m_SampleDepthBuffer.begin(), m_SampleDepthBuffer.end(), depth);
}

void Framebuffer::SetPixel(int x, int y, const Color& color)
{
    if (!InBounds(x, y)) {
        return;
    }

    const std::size_t pixelIndex = PixelIndexUnchecked(x, y);
    const std::uint32_t packedColor = color.ToBgra8();
    m_Pixels[pixelIndex] = packedColor;

    const std::size_t sampleBaseIndex = pixelIndex * kSampleCount;
    for (std::size_t sampleIndex = 0; sampleIndex < kSampleCount; ++sampleIndex) {
        m_SamplePixels[sampleBaseIndex + sampleIndex] = packedColor;
    }
}

void Framebuffer::SetDepth(int x, int y, float depth)
{
    if (!InBounds(x, y)) {
        return;
    }

    const std::size_t pixelIndex = PixelIndexUnchecked(x, y);
    m_DepthBuffer[pixelIndex] = depth;

    const std::size_t sampleBaseIndex = pixelIndex * kSampleCount;
    for (std::size_t sampleIndex = 0; sampleIndex < kSampleCount; ++sampleIndex) {
        m_SampleDepthBuffer[sampleBaseIndex + sampleIndex] = depth;
    }
}

void Framebuffer::SetSamplePixel(int x, int y, std::size_t sampleIndex, const Color& color)
{
    if (!InBounds(x, y) || !SampleIndexInRange(sampleIndex)) {
        return;
    }

    m_SamplePixels[SampleIndexUnchecked(x, y, sampleIndex)] = color.ToBgra8();
}

void Framebuffer::SetSampleDepth(int x, int y, std::size_t sampleIndex, float depth)
{
    if (!InBounds(x, y) || !SampleIndexInRange(sampleIndex)) {
        return;
    }

    const std::size_t pixelIndex = PixelIndexUnchecked(x, y);
    m_SampleDepthBuffer[SampleIndexUnchecked(x, y, sampleIndex)] = depth;
    m_DepthBuffer[pixelIndex] = ResolveDepthValue(m_SampleDepthBuffer, pixelIndex * kSampleCount);
}

std::uint32_t Framebuffer::GetPixel(int x, int y) const
{
    if (!InBounds(x, y)) {
        return 0U;
    }

    return m_Pixels[PixelIndexUnchecked(x, y)];
}

float Framebuffer::GetDepth(int x, int y) const
{
    if (!InBounds(x, y)) {
        return 1.0f;
    }

    return m_DepthBuffer[PixelIndexUnchecked(x, y)];
}

std::uint32_t Framebuffer::GetSamplePixel(int x, int y, std::size_t sampleIndex) const
{
    if (!InBounds(x, y) || !SampleIndexInRange(sampleIndex)) {
        return 0U;
    }

    return m_SamplePixels[SampleIndexUnchecked(x, y, sampleIndex)];
}

float Framebuffer::GetSampleDepth(int x, int y, std::size_t sampleIndex) const
{
    if (!InBounds(x, y) || !SampleIndexInRange(sampleIndex)) {
        return 1.0f;
    }

    return m_SampleDepthBuffer[SampleIndexUnchecked(x, y, sampleIndex)];
}

math::Vec2 Framebuffer::SampleOffset(std::size_t sampleIndex)
{
    return kSampleOffsets[sampleIndex < kSampleOffsets.size() ? sampleIndex : 0];
}

void Framebuffer::ResolveColor()
{
    for (int y = 0; y < m_Height; ++y) {
        for (int x = 0; x < m_Width; ++x) {
            const std::size_t pixelIndex = PixelIndexUnchecked(x, y);
            const std::size_t sampleBaseIndex = pixelIndex * kSampleCount;

            float red = 0.0f;
            float green = 0.0f;
            float blue = 0.0f;
            float alpha = 0.0f;
            for (std::size_t sampleIndex = 0; sampleIndex < kSampleCount; ++sampleIndex) {
                const std::uint32_t packed = m_SamplePixels[sampleBaseIndex + sampleIndex];
                blue += static_cast<float>(packed & 0xFFU) / 255.0f;
                green += static_cast<float>((packed >> 8U) & 0xFFU) / 255.0f;
                red += static_cast<float>((packed >> 16U) & 0xFFU) / 255.0f;
                alpha += static_cast<float>((packed >> 24U) & 0xFFU) / 255.0f;
            }

            const float scale = 1.0f / static_cast<float>(kSampleCount);
            m_Pixels[pixelIndex] = Color::FromFloats(
                red * scale,
                green * scale,
                blue * scale,
                alpha * scale)
                                     .ToBgra8();
            m_DepthBuffer[pixelIndex] = ResolveDepthValue(m_SampleDepthBuffer, sampleBaseIndex);
        }
    }
}

bool Framebuffer::InBounds(int x, int y) const
{
    return x >= 0 && x < m_Width && y >= 0 && y < m_Height;
}

bool Framebuffer::SampleIndexInRange(std::size_t sampleIndex) const
{
    return sampleIndex < kSampleCount;
}

} // namespace sr::render
