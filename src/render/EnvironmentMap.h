#pragma once

#include "base/Math.h"
#include "render/Color.h"
#include "render/Texture2D.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace sr::render {

class EnvironmentMap {
public:
    EnvironmentMap() = default;

    explicit EnvironmentMap(Texture2D texture)
        : m_Texture(std::move(texture))
    {
    }

    bool Empty() const { return m_Texture.Empty(); }
    int Width() const { return m_Texture.Width(); }
    int Height() const { return m_Texture.Height(); }
    const Texture2D& Texture() const { return m_Texture; }

    math::Vec3 Sample(const math::Vec3& direction) const
    {
        if (Empty()) {
            return {};
        }

        const Color color = m_Texture.SampleWrapped(DirectionToUv(direction));
        return { color.RedFloat(), color.GreenFloat(), color.BlueFloat() };
    }

    static math::Vec2 DirectionToUv(const math::Vec3& direction)
    {
        const math::Vec3 dir = direction.Length() > 0.0f
            ? direction.Normalized()
            : math::Vec3{ 0.0f, 1.0f, 0.0f };
        const float phi = std::atan2(dir.z, dir.x);
        const float theta = std::acos(math::Clamp(dir.y, -1.0f, 1.0f));
        const float u = phi / (2.0f * math::kPi) + 0.5f;
        const float v = theta / math::kPi;
        return { u, v };
    }

    static math::Vec3 UvToDirection(const math::Vec2& uv)
    {
        const float phi = (uv.x - 0.5f) * 2.0f * math::kPi;
        const float theta = uv.y * math::kPi;
        const float sinTheta = std::sin(theta);
        return math::Vec3{
            std::cos(phi) * sinTheta,
            std::cos(theta),
            std::sin(phi) * sinTheta,
        }.Normalized();
    }

private:
    Texture2D m_Texture;
};

class PrefilterEnvironmentMap {
public:
    PrefilterEnvironmentMap() = default;

    explicit PrefilterEnvironmentMap(std::vector<EnvironmentMap> levels)
        : m_Levels(std::move(levels))
    {
    }

    bool Empty() const { return m_Levels.empty(); }
    int LevelCount() const { return static_cast<int>(m_Levels.size()); }

    const EnvironmentMap* Level(int index) const
    {
        if (index < 0 || index >= LevelCount()) {
            return nullptr;
        }
        return &m_Levels[static_cast<std::size_t>(index)];
    }

    math::Vec3 Sample(const math::Vec3& direction, float lod) const
    {
        if (m_Levels.empty()) {
            return {};
        }

        const float clampedLod = math::Clamp(lod, 0.0f, static_cast<float>(std::max(LevelCount() - 1, 0)));
        const int lowIndex = static_cast<int>(std::floor(clampedLod));
        const int highIndex = std::min(lowIndex + 1, LevelCount() - 1);
        const float t = clampedLod - static_cast<float>(lowIndex);

        const math::Vec3 low = m_Levels[static_cast<std::size_t>(lowIndex)].Sample(direction);
        const math::Vec3 high = m_Levels[static_cast<std::size_t>(highIndex)].Sample(direction);
        return math::Vec3::Lerp(low, high, t);
    }

private:
    std::vector<EnvironmentMap> m_Levels;
};

} // namespace sr::render
