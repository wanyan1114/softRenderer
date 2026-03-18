#pragma once

#include <algorithm>
#include <cstdint>

namespace sr::render {

struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;

    constexpr Color()
        : r(0), g(0), b(0), a(255)
    {
    }

    constexpr Color(std::uint8_t red, std::uint8_t green, std::uint8_t blue,
        std::uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha)
    {
    }

    constexpr std::uint32_t ToBgra8() const
    {
        return static_cast<std::uint32_t>(b)
            | (static_cast<std::uint32_t>(g) << 8U)
            | (static_cast<std::uint32_t>(r) << 16U)
            | (static_cast<std::uint32_t>(a) << 24U);
    }

    float RedFloat() const { return static_cast<float>(r) / 255.0f; }
    float GreenFloat() const { return static_cast<float>(g) / 255.0f; }
    float BlueFloat() const { return static_cast<float>(b) / 255.0f; }
    float AlphaFloat() const { return static_cast<float>(a) / 255.0f; }

    static Color FromFloats(float red, float green, float blue, float alpha = 1.0f)
    {
        const auto toByte = [](float value) -> std::uint8_t {
            const float clamped = std::clamp(value, 0.0f, 1.0f);
            return static_cast<std::uint8_t>(clamped * 255.0f + 0.5f);
        };

        return Color{ toByte(red), toByte(green), toByte(blue), toByte(alpha) };
    }
};

} // namespace sr::render
