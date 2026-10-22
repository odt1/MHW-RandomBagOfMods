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
    // Monotonic.
    float last = 0.0f;
    for (int i = 0; i <= 1000; ++i) {
        const float length = stick::Length(stick::Radial({i / 1000.0f, 0.0f}, 0.01f, kCut));
        CHECK(length >= last);
        last = length;
    }
    // A deadzone above the game's cut still maps its edge to the cut.
    CHECK(stick::Length(stick::Radial({0.5f, 0.0f}, 0.5f, kCut)) == 0.0f);
    CHECK(Near(stick::Length(stick::Radial({0.75f, 0.0f}, 0.5f, kCut)), kCut + (1.0f - kCut) * 0.5f));
}

// Right stick: the camera turns only while the stick's length reaches 0.25 (the right helper's
// flag), then each axis of at least 0.15 turns it at the axis squared.
void TestCamera() {
    constexpr float kGate = 0.25f;
    constexpr float kAxisCut = 0.15f;
    // The slightest push along an axis opens the gate and turns the camera at the game's slowest
    // real speed, the gate squared.
    const stick::Vec2 small = stick::Radial({0.011f, 0.0f}, 0.01f, kGate);
    CHECK(stick::Length(small) >= kGate);
    CHECK(Near(GameAxis(small.x, kAxisCut), kGate, 1e-3f));
    // A mostly sideways push keeps its small vertical part under the axis cut, as in the game,
    // so noise on the other axis does not tilt the camera.
    const stick::Vec2 sideways = stick::Radial({0.03f, 0.003f}, 0.0f, kGate);
    CHECK(GameAxis(sideways.x, kAxisCut) != 0.0f);
    CHECK(GameAxis(sideways.y, kAxisCut) == 0.0f);
    // A small diagonal push turns both ways.
    const stick::Vec2 diagonal = stick::Radial({0.02f, 0.02f}, 0.01f, kGate);
    CHECK(GameAxis(diagonal.x, kAxisCut) != 0.0f);
    CHECK(GameAxis(diagonal.y, kAxisCut) != 0.0f);
}

}  // namespace

int main() {
    TestTravel();
    TestRadial();
    TestCamera();
    if (g_failures != 0) {
        std::fprintf(stderr, "stick_math: %d failure(s)\n", g_failures);
        return 1;
    }
    std::puts("stick_math: ok");
    return 0;
}
