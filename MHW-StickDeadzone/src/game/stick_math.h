#pragma once

#include <algorithm>
#include <cmath>

namespace stick_deadzone::game::stick {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

// Part of the stick's travel past `deadzone`: 0 inside it, 1 at full tilt.
inline float Travel(float length, float deadzone) {
    if (!(length > deadzone)) {
        return 0.0f;
    }
    return std::min(1.0f, (std::min(length, 1.0f) - deadzone) / (1.0f - deadzone));
}

// The game ignores a stick shorter than `game_cut` and uses the length as is above it. Travel past
// `deadzone` is spread over game_cut..1, so the smallest push gives the smallest response the game
// had and full tilt stays full tilt. Direction is kept.
inline Vec2 Radial(Vec2 v, float deadzone, float game_cut) {
    const float length = std::sqrt(v.x * v.x + v.y * v.y);
    const float travel = Travel(length, deadzone);
    if (travel == 0.0f) {
        return {};
    }
    // Rounding must not leave the result just under the cut, which the game compares with >=.
    constexpr float kOverCut = 1.0f + 1e-6f;
    const float scale = (game_cut + (1.0f - game_cut) * travel) * kOverCut / length;
    return {v.x * scale, v.y * scale};
}

// Same for a stick the game cuts axis by axis.
inline float Axis(float v, float deadzone, float game_cut) {
    const float travel = Travel(std::fabs(v), deadzone);
    if (travel == 0.0f) {
        return 0.0f;
    }
    return std::copysign(game_cut + (1.0f - game_cut) * travel, v);
}

// The main camera squares each axis that passes its cut. This picks the axis value whose square, the
// camera's speed, grows evenly with the travel: from its slowest turn (cut squared) to full speed.
inline float AxisLinear(float v, float deadzone, float game_cut) {
    const float travel = Travel(std::fabs(v), deadzone);
    if (travel == 0.0f) {
        return 0.0f;
    }
    const float slowest = game_cut * game_cut;
    return std::copysign(std::max(game_cut, std::sqrt(slowest + (1.0f - slowest) * travel)), v);
}

inline float Length(Vec2 v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

}  // namespace stick_deadzone::game::stick
