#include "game/deadzone.h"

#include "game/stick_math.h"
#include "runtime/log.h"
#include "runtime/memory.h"

#include <windows.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <format>
#include <fstream>
#include <utility>

namespace stick_deadzone::game::deadzone {
namespace {

// RVAs in the unpacked image of profile::kKnownPackedExecutableSha256 (re/FINDINGS.md).
// The pad singleton (class sMhSteamController) reads the selected pad, converts its sticks to floats
// with no deadzone, and copies them into the two contexts the game reads. Its per-frame update is
// reached only through this vtable slot (vtable 0x142F87138, index 6).
constexpr std::uintptr_t kPadUpdateRva = 0x1B168C0;
constexpr std::uintptr_t kPadUpdateSlotRva = 0x2F87168;
constexpr std::uintptr_t kPlayerParamRva = 0x5011F58;  // rPlayerParam*, null until loaded

constexpr std::array<std::uint8_t, 16> kPadUpdatePrologue{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x48,
};

// Stick blocks in the pad singleton: the converted floats, then their copies for context 0 (the one
// the hunter's controller reads) and context 1. Keyboard and mouse input overwrite context 0's
// sticks after the copy.
constexpr std::array<std::ptrdiff_t, 3> kStickBlocks{0xC28, 0xCE0, 0xD98};
constexpr std::ptrdiff_t kLeftX = 0x18;  // then left Y
constexpr std::ptrdiff_t kRightX = 0x20; // then right Y
constexpr std::ptrdiff_t kLeftLength = 0x28;
constexpr std::ptrdiff_t kRightLength = 0x2C;

// rPlayerParam fields the game compares the sticks with; each one is a plain cut, values at or
// above it are used unchanged.
constexpr std::ptrdiff_t kLeftHelperCut = 0x124;   // left stick "active" flag of the hunter's controller
constexpr std::ptrdiff_t kLeftNeutralCut = 0x134;  // movement's neutral check
constexpr std::ptrdiff_t kCameraCutY = 0x12C;      // main camera, per axis (copied into the camera
constexpr std::ptrdiff_t kCameraCutX = 0x130;      // every frame; axis order inferred)
// Values seen in the game's own pl_param, used while rPlayerParam is not loaded.
constexpr float kDefaultLeftCut = 0.30f;
constexpr float kDefaultCameraCut = 0.15f;
constexpr float kMaxCut = 0.95f;

constexpr ULONGLONG kConfigCheckMs = 1000;
constexpr ULONGLONG kStickLogMs = 1000;
constexpr float kDefaultDeadzone = 0.01f;
constexpr float kMaxDeadzone = 0.9f;

constexpr char kDefaultConfig[] =
    "; MHW Stick Deadzone\n"
    ";\n"
    "; Deadzone is how far a stick must move from the center before the game sees it, from 0.0 to\n"
    "; 0.9 (0.01 = 1% of the stick's travel). The game itself ignores 30% of the left stick and 15%\n"
    "; of each right stick axis. Past your deadzone the stick reaches the game's slowest walk or\n"
    "; camera turn right away and full tilt stays full tilt, so the game's own response is kept,\n"
    "; spread over more of the stick.\n"
    "; Raise a value if that stick moves the hunter or the camera when you let go of it.\n"
    "; Enabled=0 leaves that stick to the game. Changes apply within a second in game.\n"
    "[LeftStick]\n"
    "Enabled=1\n"
    "Deadzone=0.01\n"
    "\n"
    "[RightStick]\n"
    "Enabled=1\n"
    "Deadzone=0.01\n"
    "\n"
    "; LogSticks=1 writes the left and right stick before and after once a second while one is held.\n"
    "[Debug]\n"
    "LogSticks=0\n";

using PadUpdateFn = std::uint64_t(__fastcall*)(std::uint8_t* pad);

struct StickConfig {
    bool enabled = true;
    float deadzone = kDefaultDeadzone;
};

struct Cuts {
    float left = kDefaultLeftCut;
    float camera_x = kDefaultCameraCut;
    float camera_y = kDefaultCameraCut;

    bool operator==(const Cuts&) const = default;
};

std::uint8_t* g_exe = nullptr;
std::filesystem::path g_config_path;
PadUpdateFn g_pad_update = nullptr;

// Guarded by g_tick_lock: the pad update is not guaranteed to stay on one thread.
SRWLOCK g_tick_lock = SRWLOCK_INIT;
bool g_reported_thread = false;
StickConfig g_left;
StickConfig g_right;
bool g_log_sticks = false;
FILETIME g_config_time{};
ULONGLONG g_next_config_check = 0;
ULONGLONG g_next_stick_log = 0;
Cuts g_cuts{};
bool g_cuts_logged = false;

template <typename... Args>
void SafeLog(std::format_string<Args...> format, Args&&... args) noexcept {
    try {
        log::Log(format, std::forward<Args>(args)...);
    } catch (...) {
        // Logging must never unwind through game code.
    }
}

float ReadFloat(const wchar_t* section, const wchar_t* key, float fallback) noexcept {
    wchar_t text[32]{};
    GetPrivateProfileStringW(section, key, L"", text, static_cast<DWORD>(std::size(text)), g_config_path.c_str());
    wchar_t* end = nullptr;
    const float value = std::wcstof(text, &end);
    if (end == text || !std::isfinite(value)) {
        return fallback;
    }
    return value;
}

StickConfig ReadStick(const wchar_t* section, const char* name) noexcept {
    StickConfig stick;
    stick.enabled = GetPrivateProfileIntW(section, L"Enabled", 1, g_config_path.c_str()) != 0;
    const float deadzone = ReadFloat(section, L"Deadzone", kDefaultDeadzone);
    if (deadzone < 0.0f || deadzone > kMaxDeadzone) {
        SafeLog("Config: {} Deadzone={} is outside 0.0 to {}; using {}.", name, deadzone, kMaxDeadzone,
                kDefaultDeadzone);
    } else {
        stick.deadzone = deadzone;
    }
    return stick;
}

void ReloadConfig() noexcept {
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    FILETIME time{};
    if (GetFileAttributesExW(g_config_path.c_str(), GetFileExInfoStandard, &attributes)) {
        time = attributes.ftLastWriteTime;
    }
    if (CompareFileTime(&time, &g_config_time) == 0) {
        return;
    }
    g_config_time = time;
    g_left = ReadStick(L"LeftStick", "LeftStick");
    g_right = ReadStick(L"RightStick", "RightStick");
    g_log_sticks = GetPrivateProfileIntW(L"Debug", L"LogSticks", 0, g_config_path.c_str()) != 0;
    SafeLog("Config: left stick {} deadzone {:.3f}, right stick {} deadzone {:.3f}, LogSticks={}.",
            g_left.enabled ? "on" : "off", g_left.deadzone, g_right.enabled ? "on" : "off", g_right.deadzone,
            g_log_sticks ? 1 : 0);
}

float ReadCut(std::uint8_t* param, std::ptrdiff_t offset, float fallback) noexcept {
    const auto value = mem::Read<float>(param + offset);
    if (!value || !(*value >= 0.0f && *value <= kMaxCut)) {
        return fallback;
    }
    return *value;
}

void RefreshCuts() noexcept {
    Cuts cuts;
    auto* param = mem::Read<std::uint8_t*>(g_exe + kPlayerParamRva).value_or(nullptr);
    if (param != nullptr) {
        cuts.left = std::max(ReadCut(param, kLeftHelperCut, kDefaultLeftCut),
                             ReadCut(param, kLeftNeutralCut, kDefaultLeftCut));
        cuts.camera_x = ReadCut(param, kCameraCutX, kDefaultCameraCut);
        cuts.camera_y = ReadCut(param, kCameraCutY, kDefaultCameraCut);
    }
    if (!g_cuts_logged || !(cuts == g_cuts)) {
        g_cuts_logged = true;
        SafeLog("Game deadzones ({}): left stick {:.3f}, right stick X {:.3f} Y {:.3f}.",
                param != nullptr ? "rPlayerParam" : "defaults, rPlayerParam not loaded", cuts.left,
                cuts.camera_x, cuts.camera_y);
    }
    g_cuts = cuts;
}

stick::Vec2 ReadVec(std::uint8_t* block, std::ptrdiff_t offset) {
    return {*reinterpret_cast<float*>(block + offset), *reinterpret_cast<float*>(block + offset + 4)};
}

void WriteStick(std::uint8_t* block, std::ptrdiff_t offset, std::ptrdiff_t length_offset, stick::Vec2 v) {
    *reinterpret_cast<float*>(block + offset) = v.x;
    *reinterpret_cast<float*>(block + offset + 4) = v.y;
    *reinterpret_cast<float*>(block + length_offset) = stick::Length(v);
}

bool Same(stick::Vec2 a, stick::Vec2 b) {
    return a.x == b.x && a.y == b.y;
}

// Rewrites one stick in the converted block and in each copy that still holds the converted value;
// a copy that keyboard or mouse input replaced is left alone.
void ApplyStick(std::uint8_t* pad, std::ptrdiff_t offset, std::ptrdiff_t length_offset, stick::Vec2 converted,
                stick::Vec2 result) {
    for (const std::ptrdiff_t block_offset : kStickBlocks) {
        std::uint8_t* block = pad + block_offset;
        if (block_offset == kStickBlocks[0] || Same(ReadVec(block, offset), converted)) {
            WriteStick(block, offset, length_offset, result);
        }
    }
}

void Tick(std::uint8_t* pad) {
    const ULONGLONG now = GetTickCount64();
    if (now >= g_next_config_check) {
        g_next_config_check = now + kConfigCheckMs;
        ReloadConfig();
        RefreshCuts();
    }
    std::uint8_t* converted = pad + kStickBlocks[0];
    const stick::Vec2 left = ReadVec(converted, kLeftX);
    const stick::Vec2 right = ReadVec(converted, kRightX);
    stick::Vec2 new_left = left;
    stick::Vec2 new_right = right;
    if (g_left.enabled) {
        new_left = stick::Radial(left, g_left.deadzone, g_cuts.left);
        ApplyStick(pad, kLeftX, kLeftLength, left, new_left);
    }
    if (g_right.enabled) {
        new_right = {stick::Axis(right.x, g_right.deadzone, g_cuts.camera_x),
                     stick::Axis(right.y, g_right.deadzone, g_cuts.camera_y)};
        ApplyStick(pad, kRightX, kRightLength, right, new_right);
    }
    if (g_log_sticks && now >= g_next_stick_log && (stick::Length(left) > 0.0f || stick::Length(right) > 0.0f)) {
        g_next_stick_log = now + kStickLogMs;
        SafeLog("Sticks: left ({:.3f}, {:.3f}) -> ({:.3f}, {:.3f}), right ({:.3f}, {:.3f}) -> ({:.3f}, {:.3f}).",
                left.x, left.y, new_left.x, new_left.y, right.x, right.y, new_right.x, new_right.y);
    }
}

std::uint64_t __fastcall PadUpdateDetour(std::uint8_t* pad) {
    // After the original, which converts the sticks and fills both contexts; the hunter's controller
    // reads context 0 later in the frame.
    const std::uint64_t result = g_pad_update(pad);
    if (TryAcquireSRWLockExclusive(&g_tick_lock)) {
        if (!g_reported_thread) {
            g_reported_thread = true;
            SafeLog("Stick deadzone: active (first pad update on thread {}).", GetCurrentThreadId());
        }
        Tick(pad);
        ReleaseSRWLockExclusive(&g_tick_lock);
    }
    return result;
}

bool WriteDefaultConfig() {
    std::ofstream file(g_config_path, std::ios::binary);
    file << kDefaultConfig;
    return static_cast<bool>(file);
}

}  // namespace

bool Install(const std::filesystem::path& config_path) {
    g_exe = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    g_config_path = config_path;

    if (!mem::Matches(g_exe + kPadUpdateRva, kPadUpdatePrologue)) {
        log::Log("Stick deadzone: game code at +0x{:X} does not match the known build; not installed.",
                 kPadUpdateRva);
        return false;
    }

    std::error_code error;
    if (!std::filesystem::exists(g_config_path, error) && !WriteDefaultConfig()) {
        log::Write("Stick deadzone: could not create the default config; using deadzone 0.01 for both sticks.");
    }
    ReloadConfig();

    // A vtable slot swap chains with code hooks in any order and with other slot hooks, including one
    // installed at the same moment. The detour can run as soon as the slot points at it, so the
    // function it calls is stored first.
    const auto patch = mem::HookPointer(reinterpret_cast<void**>(g_exe + kPadUpdateSlotRva),
                                        reinterpret_cast<void*>(&PadUpdateDetour),
                                        reinterpret_cast<void**>(&g_pad_update));
    if (!patch.installed) {
        log::Write("Stick deadzone: could not hook the pad update vtable slot; not installed.");
        return false;
    }
    if (!patch.protection_restored) {
        log::Write("Stick deadzone: the pad vtable page kept write access after the hook.");
    }
    const bool chained = reinterpret_cast<std::uint8_t*>(g_pad_update) != g_exe + kPadUpdateRva;
    log::Log("Stick deadzone: installed (pad update slot +0x{:X}{}).", kPadUpdateSlotRva,
             chained ? ", chained after another plugin's hook" : "");
    return true;
}

}  // namespace stick_deadzone::game::deadzone
