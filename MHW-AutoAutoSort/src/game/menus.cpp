#include "game/menus.h"

#include "runtime/log.h"
#include "runtime/memory.h"

#include <windows.h>

#include <MinHook.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace auto_auto_sort::game::menus {
namespace {

// RVAs in the unpacked image of profile::kKnownPackedExecutableSha256 (re/FINDINGS.md).
// uGUIBase (DTI-registered GUI base class, 437 constructors call its constructor +0x1FB3D10):
// open sets bit 0 of the state word at +0x2298, close sets bit 1 and the bit 0 clears later.
constexpr std::uintptr_t kOpenRva = 0x1FB79B0;
constexpr std::uintptr_t kCloseRva = 0x1FB55E0;
constexpr std::uintptr_t kDestructorRva = 0x1FB4210;

constexpr std::array<std::uint8_t, 16> kOpenPrologue{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x6C, 0x24, 0x18, 0x48, 0x89, 0x74, 0x24, 0x20, 0x57,
};
constexpr std::array<std::uint8_t, 16> kClosePrologue{
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x40, 0x83, 0x89, 0x98, 0x22, 0x00, 0x00,
};
constexpr std::array<std::uint8_t, 16> kDestructorPrologue{
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57,
};

constexpr std::ptrdiff_t kGuiState = 0x2298;
constexpr std::uint32_t kGuiOpen = 0x1;
// MtObject vtable slot 4 is getDTI, `lea rax, [rip+dti]; ret`; the DTI's class name is at +8.
constexpr std::size_t kGetDtiSlot = 4;
constexpr std::ptrdiff_t kDtiName = 0x8;
constexpr ULONGLONG kCloseGraceMs = 300;

// Menus that list pouch or box slots by position, change the pouch, or edit the item bar. Sorting
// under them would move items behind the cursor, so the sort waits until they close.
constexpr std::array<std::string_view, 15> kPouchMenus{
    "uGUIItemPouch",           // start menu > Item Pouch (its own Auto-sort lives here)
    "uGUIItemWindowReplace",   // item bar editor (Sort by Item Pouch)
    "uGUIBoxItemReplace",      // item box: move items between pouch and box
    "uGUIBoxItemMySet",        // item loadouts
    "uGUIBoxItemSell",
    "uGUIItemShop",
    "uGUISupplyBox",
    "uGUINumSelectPouch",
    "uGUIItemReplace",
    "uGUIItemReplaceSub",
    "uGUIItemMakeList",        // crafting list
    "uGUIItemMake",
    "uGUIRingShortcutEditor",  // radial menu editors
    "uGUIRingShortcutEditorPC",
    "uGUIShortCutEditWindow",
};

using GuiFn = std::int64_t(__fastcall*)(void* gui);

struct Watched {
    void* gui = nullptr;
    bool open = false;
};

GuiFn g_open = nullptr;
GuiFn g_close = nullptr;
GuiFn g_destructor = nullptr;
SRWLOCK g_lock = SRWLOCK_INIT;
std::array<Watched, 32> g_watched{};
ULONGLONG g_last_close = 0;

// Reads the class name through the getDTI stub without calling into the object.
bool ClassName(void* gui, char* name, std::size_t size) noexcept {
    auto* vtable = mem::Read<std::uint8_t*>(gui).value_or(nullptr);
    auto* get_dti = vtable == nullptr ? nullptr
                                      : mem::Read<std::uint8_t*>(vtable + kGetDtiSlot * sizeof(void*)).value_or(nullptr);
    if (get_dti == nullptr) {
        return false;
    }
    constexpr std::array<std::uint8_t, 3> kLeaRax{0x48, 0x8D, 0x05};
    const auto displacement = mem::Read<std::int32_t>(get_dti + 3);
    if (!mem::Matches(get_dti, kLeaRax) || !displacement) {
        return false;
    }
    auto* dti = get_dti + 7 + *displacement;
    auto* text = mem::Read<const char*>(dti + kDtiName).value_or(nullptr);
    if (text == nullptr || size == 0) {
        return false;
    }
    // Class names are short; read until the terminator or the buffer is full.
    for (std::size_t i = 0; i + 1 < size; ++i) {
        const auto c = mem::Read<char>(text + i);
        if (!c) {
            return false;
        }
        name[i] = *c;
        if (*c == '\0') {
            return true;
        }
    }
    name[size - 1] = '\0';
    return true;
}

bool IsPouchMenu(void* gui) noexcept {
    char name[64]{};
    if (!ClassName(gui, name, sizeof(name))) {
        return false;
    }
    const std::string_view view(name);
    for (const auto menu : kPouchMenus) {
        if (view == menu) {
            return true;
        }
    }
    return false;
}

Watched* Find(void* gui) noexcept {
    for (auto& entry : g_watched) {
        if (entry.gui == gui) {
            return &entry;
        }
    }
    return nullptr;
}

std::int64_t __fastcall OpenDetour(void* gui) {
    const std::int64_t result = g_open(gui);
    AcquireSRWLockExclusive(&g_lock);
    Watched* entry = Find(gui);
    if (entry == nullptr && IsPouchMenu(gui)) {
        entry = Find(nullptr);
        if (entry == nullptr) {
            // Every slot taken by a live pouch menu would mean dozens of them; keep sorting off.
            g_last_close = ~0ULL;
        } else {
            entry->gui = gui;
        }
    }
    if (entry != nullptr) {
        entry->open = true;
    }
    ReleaseSRWLockExclusive(&g_lock);
    return result;
}

std::int64_t __fastcall CloseDetour(void* gui) {
    // The original runs first: callees of close may take values in volatile registers.
    const std::int64_t result = g_close(gui);
    AcquireSRWLockExclusive(&g_lock);
    if (Watched* entry = Find(gui)) {
        entry->open = false;
        g_last_close = GetTickCount64();
    }
    ReleaseSRWLockExclusive(&g_lock);
    return result;
}

std::int64_t __fastcall DestructorDetour(void* gui) {
    AcquireSRWLockExclusive(&g_lock);
    if (Watched* entry = Find(gui)) {
        *entry = {};
        g_last_close = GetTickCount64();
    }
    ReleaseSRWLockExclusive(&g_lock);
    return g_destructor(gui);
}

}  // namespace

bool Install(std::uint8_t* exe) {
    auto* open = exe + kOpenRva;
    auto* close = exe + kCloseRva;
    auto* destructor = exe + kDestructorRva;
    if (!mem::Matches(open, kOpenPrologue) || !mem::Matches(close, kClosePrologue) ||
        !mem::Matches(destructor, kDestructorPrologue)) {
        log::Write("Menus: GUI open/close code does not match the known build; not installed.");
        return false;
    }
    const MH_STATUS open_status =
        MH_CreateHook(open, reinterpret_cast<void*>(&OpenDetour), reinterpret_cast<void**>(&g_open));
    const MH_STATUS close_status =
        MH_CreateHook(close, reinterpret_cast<void*>(&CloseDetour), reinterpret_cast<void**>(&g_close));
    const MH_STATUS destructor_status = MH_CreateHook(destructor, reinterpret_cast<void*>(&DestructorDetour),
                                                      reinterpret_cast<void**>(&g_destructor));
    if (open_status != MH_OK || close_status != MH_OK || destructor_status != MH_OK) {
        log::Log("Menus: hooks failed (open {}, close {}, destructor {}).", MH_StatusToString(open_status),
                 MH_StatusToString(close_status), MH_StatusToString(destructor_status));
        MH_RemoveHook(open);
        MH_RemoveHook(close);
        MH_RemoveHook(destructor);
        return false;
    }
    return true;
}

bool PouchMenuActive() noexcept {
    AcquireSRWLockShared(&g_lock);
    bool active = false;
    for (const auto& entry : g_watched) {
        // A GUI hidden without the close call (area change, forced close) has its open bit cleared.
        if (entry.open && (mem::Read<std::uint32_t>(static_cast<std::uint8_t*>(entry.gui) + kGuiState)
                               .value_or(0) & kGuiOpen) != 0) {
            active = true;
            break;
        }
    }
    const ULONGLONG last_close = g_last_close;
    ReleaseSRWLockShared(&g_lock);
    return active || GetTickCount64() - last_close < kCloseGraceMs || last_close == ~0ULL;
}

}  // namespace auto_auto_sort::game::menus
