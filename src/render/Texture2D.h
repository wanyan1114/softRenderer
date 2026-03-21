#pragma once

#include "base/Math.h"
#include "render/Color.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace sr::render {

class Texture2D {
public:
    Texture2D() = default;

    Texture2D(int width, int height, std::vector<Color> pixels)
        : m_Width(width),
          m_Height(height),
          m_Pixels(std::move(pixels))
    {
    }

    int Width() const { return m_Width; }
    int Height() const { return m_Height; }

    bool Empty() const
    {
        return m_Width <= 0 || m_Height <= 0 || m_Pixels.empty();
    }

    Color Sample(const math::Vec2& uv) const
    {
        if (Empty()) {
            return {};
        }

        const float u = math::Clamp(uv.x, 0.0f, 1.0f);
        const float v = math::Clamp(uv.y, 0.0f, 1.0f);

        const float texelX = u * static_cast<float>(m_Width - 1);
        const float texelY = (1.0f - v) * static_cast<float>(m_Height - 1);

        const int x0 = static_cast<int>(texelX);
        const int y0 = static_cast<int>(texelY);
        const int x1 = std::min(x0 + 1, m_Width - 1);
        const int y1 = std::min(y0 + 1, m_Height - 1);

        const float fracX = texelX - static_cast<float>(x0);
        const float fracY = texelY - static_cast<float>(y0);

        const Color c00 = Texel(x0, y0);
        const Color c10 = Texel(x1, y0);
        const Color c01 = Texel(x0, y1);
        const Color c11 = Texel(x1, y1);

        const float topR = math::Lerp(c00.RedFloat(), c10.RedFloat(), fracX);
        const float topG = math::Lerp(c00.GreenFloat(), c10.GreenFloat(), fracX);
        const float topB = math::Lerp(c00.BlueFloat(), c10.BlueFloat(), fracX);
        const float topA = math::Lerp(c00.AlphaFloat(), c10.AlphaFloat(), fracX);

        const float bottomR = math::Lerp(c01.RedFloat(), c11.RedFloat(), fracX);
        const float bottomG = math::Lerp(c01.GreenFloat(), c11.GreenFloat(), fracX);
        const float bottomB = math::Lerp(c01.BlueFloat(), c11.BlueFloat(), fracX);
        const float bottomA = math::Lerp(c01.AlphaFloat(), c11.AlphaFloat(), fracX);

        return Color::FromFloats(
            math::Lerp(topR, bottomR, fracY),
            math::Lerp(topG, bottomG, fracY),
            math::Lerp(topB, bottomB, fracY),
            math::Lerp(topA, bottomA, fracY));
    }

    static Texture2D MakeDebugUvTexture(int width, int height)
    {
        std::vector<Color> pixels;
        pixels.reserve(static_cast<std::size_t>(width * height));

        for (int y = 0; y < height; ++y) {
            const float v = 1.0f - static_cast<float>(y) / static_cast<float>(std::max(height - 1, 1));
            for (int x = 0; x < width; ++x) {
                const float u = static_cast<float>(x) / static_cast<float>(std::max(width - 1, 1));
                const int checkerX = static_cast<int>(u * 8.0f);
                const int checkerY = static_cast<int>(v * 8.0f);
                const bool evenChecker = ((checkerX + checkerY) % 2) == 0;

                Color base = evenChecker
                    ? Color{ 240, 240, 240 }
                    : Color{ 70, 70, 70 };

                if (u < 0.18f && v > 0.82f) {
                    base = Color{ 220, 64, 64 };
                } else if (u > 0.82f && v > 0.82f) {
                    base = Color{ 64, 190, 96 };
                } else if (u < 0.18f && v < 0.18f) {
                    base = Color{ 64, 96, 220 };
                } else if (u > 0.82f && v < 0.18f) {
                    base = Color{ 224, 196, 72 };
                }

                const bool border = u < 0.04f || u > 0.96f || v < 0.04f || v > 0.96f;
                const float gridX = std::abs(u * 8.0f - std::round(u * 8.0f));
                const float gridY = std::abs(v * 8.0f - std::round(v * 8.0f));
                const bool grid = gridX < 0.03f || gridY < 0.03f;
                if (border || grid) {
                    base = Color{ 16, 16, 16 };
                }

                pixels.push_back(base);
            }
        }

        return Texture2D(width, height, std::move(pixels));
    }

private:
    Color Texel(int x, int y) const
    {
        return m_Pixels[static_cast<std::size_t>(y * m_Width + x)];
    }

    int m_Width = 0;
    int m_Height = 0;
    std::vector<Color> m_Pixels;
};

} // namespace sr::render
