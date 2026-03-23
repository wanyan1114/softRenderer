#pragma once

#include <array>
#include <cstddef>

namespace sr::platform {

enum class Key {
    W = 0,
    A,
    S,
    D,
    Q,
    E,
    F,
    G,
    R,
    T,
    Space,
    LeftShift,
    Count,
};

struct InputState {
    std::array<bool, static_cast<std::size_t>(Key::Count)> keyStates{};

    bool IsKeyDown(Key key) const
    {
        return keyStates[static_cast<std::size_t>(key)];
    }
};

} // namespace sr::platform
