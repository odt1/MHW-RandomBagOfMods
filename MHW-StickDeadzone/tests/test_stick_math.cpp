#include "game/stick_math.h"

#include <cmath>
#include <cstdio>

namespace {

namespace stick = stick_deadzone::game::stick;

int g_failures = 0;

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #condition); \
            ++g_failures;                                                    \
        }                                                                    \
    } while (false)

bool Near(float a, float b, float tolerance = 1e-4f) {
    return std::fabs(a - b) <= tolerance;
}

// The game's own cuts: a value under the cut is 0, at or over it is used as is.
float GameAxis(float v, float cut) {
    return std::fabs(v) >= cut ? v : 0.0f;
}

void TestTravel() {
    CHECK(stick::Travel(0.0f, 0.01f) == 0.0f);
    CHECK(stick::Travel(0.01f, 0.01f) == 0.0f);    // the deadzone's edge is still dead
    CHECK(Near(stick::Travel(0.505f, 0.01f), 0.5f));
    CHECK(stick::Travel(1.0f, 0.01f) == 1.0f);
    CHECK(stick::Travel(1.4f, 0.01f) == 1.0f);     // square gates reach past 1 on diagonals
    CHECK(Near(stick::Travel(0.5f, 0.0f), 0.5f));  // no deadzone: travel is the length
}

void TestAxis() {
    constexpr float kCut = 0.15f;
    CHECK(stick::Axis(0.0f, 0.01f, kCut) == 0.0f);
    CHECK(stick::Axis(0.005f, 0.01f, kCut) == 0.0f);
    CHECK(stick::Axis(-0.009f, 0.01f, kCut) == 0.0f);
    // Just past the deadzone: the camera's slowest turn, never cut by the game.
    const float small = stick::Axis(0.011f, 0.01f, kCut);
    CHECK(small >= kCut && small < kCut + 0.01f);
    CHECK(GameAxis(small, kCut) == small);
    CHECK(GameAxis(stick::Axis(-0.011f, 0.01f, kCut), kCut) < 0.0f);
    // Full tilt stays full tilt, both ways.
    CHECK(stick::Axis(1.0f, 0.01f, kCut) == 1.0f);
    CHECK(stick::Axis(-1.0f, 0.01f, kCut) == -1.0f);
    // Linear in between: halfway through the travel is halfway through the game's range.
    CHECK(Near(stick::Axis(0.505f, 0.01f, kCut), kCut + (1.0f - kCut) * 0.5f));
    // Monotonic.
    float last = 0.0f;
    for (int i = 0; i <= 1000; ++i) {
        const float v = stick::Axis(i / 1000.0f, 0.01f, kCut);
        CHECK(v >= last);
        last = v;
    }
    // Deadzone 0 and cut 0 leave the axis unchanged.
    CHECK(Near(stick::Axis(0.37f, 0.0f, 0.0f), 0.37f));
}

void TestRadial() {
    constexpr float kCut = 0.30f;
    CHECK(stick::Length(stick::Radial({0.0f, 0.0f}, 0.01f, kCut)) == 0.0f);
    CHECK(stick::Length(stick::Radial({0.005f, -0.005f}, 0.01f, kCut)) == 0.0f);
    // Any push past the deadzone reaches the game's cut, in every direction, so it is never ignored.
    for (int degrees = 0; degrees < 360; degrees += 5) {
        const float a = degrees * 3.14159265f / 180.0f;
        for (const float length : {0.0101f, 0.011f, 0.02f, 0.1f, 0.29f}) {
            const stick::Vec2 out = stick::Radial({length * std::cos(a), length * std::sin(a)}, 0.01f, kCut);
            CHECK(stick::Length(out) >= kCut);
        }
    }
    // Direction is kept.
    const stick::Vec2 out = stick::Radial({0.03f, 0.04f}, 0.01f, kCut);
    CHECK(Near(out.y / out.x, 0.04f / 0.03f));
    // Full tilt stays full tilt; past full tilt is clamped to it.
    CHECK(Near(stick::Length(stick::Radial({0.0f, 1.0f}, 0.01f, kCut)), 1.0f));
    CHECK(Near(stick::Length(stick::Radial({1.0f, 1.0f}, 0.01f, kCut)), 1.0f));
    // Linear in between.
    CHECK(Near(stick::Length(stick::Radial({0.505f, 0.0f}, 0.01f, kCut)), kCut + (1.0f - kCut) * 0.5f));
    // A deadzone above the game's cut still maps its edge to the cut.
    CHECK(stick::Length(stick::Radial({0.5f, 0.0f}, 0.5f, kCut)) == 0.0f);
    CHECK(Near(stick::Length(stick::Radial({0.75f, 0.0f}, 0.5f, kCut)), kCut + (1.0f - kCut) * 0.5f));
}

}  // namespace

int main() {
    TestTravel();
    TestAxis();
    TestRadial();
    if (g_failures != 0) {
        std::fprintf(stderr, "stick_math: %d failure(s)\n", g_failures);
        return 1;
    }
    std::puts("stick_math: ok");
    return 0;
}
