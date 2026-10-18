#include "game/autosort.h"

#include "game/menus.h"
#include "game/pouch_order.h"
#include "runtime/log.h"
#include "runtime/memory.h"

#include <windows.h>

#include <MinHook.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <span>
#include <string>
#include <utility>

namespace auto_auto_sort::game::autosort {
namespace {

// RVAs in the unpacked image of profile::kKnownPackedExecutableSha256 (re/FINDINGS.md).
constexpr std::uintptr_t kMainUpdateRva = 0x1AF2B60;     // sMhMain per-frame update, main thread
constexpr std::uintptr_t kUserSingletonRva = 0x5013950;   // argument of the user accessor
constexpr std::uintptr_t kPlayerManagerRva = 0x500ECA0;   // argument of the local player lookup
constexpr std::uintptr_t kUserAccessorRva = 0x1B8DBB0;    // (singleton) -> selected character's data
constexpr std::uintptr_t kLocalPlayerRva = 0x1B42010;     // (manager) -> local player or null
constexpr std::uintptr_t kPrepareSortRva = 0x135D610;     // (user, area) Auto-sort, sorted view
constexpr std::uintptr_t kApplySortRva = 0x134AB90;       // (user, area) Auto-sort, write back
constexpr std::uintptr_t kSortKeyRva = 0x18978C0;         // (item) -> sort key
constexpr std::uintptr_t kItemTypeRva = 0x18978F0;        // (item) -> type; 1 and 4 go to the ammo bar
constexpr std::uintptr_t kShowInBarRva = 0x1895E80;       // (item, ammo, loadout) -> listed in the bar
constexpr std::uintptr_t kPouchItemRva = 0x1355970;       // (user, list, index) -> item ID
constexpr std::uintptr_t kClearBarRva = 0x13504C0;        // (user, loadout) empties item and ammo bar
constexpr std::uintptr_t kSetBarSlotRva = 0x1368690;      // (user, index, item, loadout)

struct Expected {
    std::uintptr_t rva;
    std::array<std::uint8_t, 16> bytes;
};
// First bytes of every function called; a mismatch installs nothing.
constexpr std::array<Expected, 10> kCalled{{
    {kUserAccessorRva, {0x48, 0x63, 0x81, 0xA0, 0x00, 0x00, 0x00, 0x48, 0x69, 0xC0, 0x00, 0xCC, 0x26, 0x00, 0x48, 0x03}},
    {kLocalPlayerRva, {0x45, 0x33, 0xC9, 0x4C, 0x8D, 0x41, 0x50, 0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {kPrepareSortRva, {0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48}},
    {kApplySortRva, {0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48}},
    {kSortKeyRva, {0x48, 0x83, 0xEC, 0x28, 0x8B, 0xD1, 0x48, 0x8B, 0x0D, 0x33, 0xA7, 0x77, 0x03, 0x48, 0x85, 0xC9}},
    {kItemTypeRva, {0x48, 0x83, 0xEC, 0x28, 0x85, 0xC9, 0x74, 0x29, 0x83, 0xF9, 0xFF, 0x74, 0x24, 0x48, 0x8B, 0x05}},
    {kShowInBarRva, {0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57}},
    {kPouchItemRva, {0x4C, 0x8B, 0xC9, 0x85, 0xD2, 0x0F, 0x84, 0x82, 0x00, 0x00, 0x00, 0x83, 0xEA, 0x01, 0x74, 0x6F}},
    {kClearBarRva, {0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x48}},
    {kSetBarSlotRva, {0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x6C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x48}},
}};
// Another plugin may already have replaced these nine bytes with a jump; MinHook chains over it.
constexpr std::array<std::uint8_t, 9> kMainUpdatePrologue{0x40, 0x53, 0x48, 0x81, 0xEC, 0x80, 0x00, 0x00, 0x00};
constexpr std::ptrdiff_t kMainUpdateBodyOffset = 9;
constexpr std::array<std::uint8_t, 4> kMainUpdateBody{0x48, 0x8B, 0xD9, 0xE8};

// Character data (the user accessor's result). Pouch slots are 16-byte objects: vtable, item ID,
// count. Auto-sort area 2, the one the start menu's Item Pouch uses, sorts these four lists.
constexpr std::ptrdiff_t kUserLock = 0x8;  // CRITICAL_SECTION the game takes around pouch changes
constexpr std::ptrdiff_t kPouchBegin = 0x38080;
constexpr std::ptrdiff_t kPouchSlot = 0x10;
constexpr std::ptrdiff_t kSlotItem = 0x8;
struct PouchList {
    std::ptrdiff_t offset;
    std::size_t slots;
};
constexpr std::array<PouchList, 4> kPouchLists{{
    {0x38080, 24},  // items (list 0)
    {0x38200, 16},  // ammo and coatings (list 1)
    {0x38380, 24},
    {0x38500, 5},   // fixed tools: whetstone, capture net, fishing rod, BBQ spit (list 3)
}};
constexpr std::ptrdiff_t kPouchEnd = 0x38550;
constexpr int kSortArea = 2;
// Item bar order: 40 item IDs, then 40 ammo IDs, for the current (not a loadout's) bar.
constexpr std::ptrdiff_t kItemBar = 0xEDF38;
constexpr std::ptrdiff_t kAmmoBar = 0xEDFD8;
constexpr std::size_t kBarLength = 40;
constexpr int kCurrentLoadout = -1;
// Lists of the pouch item getter.
constexpr std::uint32_t kListItems = 0;
constexpr std::uint32_t kListAmmo = 1;
constexpr std::uint32_t kListTools = 3;
constexpr std::uint32_t kListMantles = 4;  // the two equipped specialized tools
constexpr std::uint32_t kTypeAmmo = 1;
constexpr std::uint32_t kTypeCoating = 4;

constexpr unsigned kTickFrames = 6;
constexpr ULONGLONG kConfigCheckMs = 1000;

constexpr char kDefaultConfig[] =
    "; MHW Auto-Auto-Sort\n"
    ";\n"
    "; Pouch=1 keeps the item pouch sorted, as if you pressed Auto-sort after every change.\n"
    "; ItemBar=1 keeps the item bar in item pouch order, as if you chose Sort by Item Pouch after every change.\n"
    "; 0 turns either off. Changes apply within a second in game.\n"
    "[AutoSort]\n"
    "Pouch=1\n"
    "ItemBar=1\n";

using MainUpdateFn = void*(__fastcall*)(void* self);
using UserAccessorFn = std::uint8_t*(__fastcall*)(void* singleton);
using LocalPlayerFn = void*(__fastcall*)(void* manager);
using SortAreaFn = void(__fastcall*)(std::uint8_t* user, int area);
using ItemFn = std::uint32_t(__fastcall*)(std::uint32_t item);
using ShowInBarFn = bool(__fastcall*)(std::uint32_t item, bool ammo, bool loadout);
using PouchItemFn = std::uint32_t(__fastcall*)(std::uint8_t* user, std::uint32_t list, std::uint32_t index);
using ClearBarFn = void(__fastcall*)(std::uint8_t* user, int loadout);
using SetBarSlotFn = void(__fastcall*)(std::uint8_t* user, std::uint32_t index, std::uint32_t item, int loadout);

struct Snapshot {
    std::array<std::uint8_t, kPouchEnd - kPouchBegin> pouch{};
    std::array<std::uint32_t, kBarLength * 2> bars{};

    bool operator==(const Snapshot&) const = default;
};

std::uint8_t* g_exe = nullptr;
std::filesystem::path g_config_path;
MainUpdateFn g_main_update = nullptr;
UserAccessorFn g_user_accessor = nullptr;
LocalPlayerFn g_local_player = nullptr;
SortAreaFn g_prepare_sort = nullptr;
SortAreaFn g_apply_sort = nullptr;
ItemFn g_sort_key = nullptr;
ItemFn g_item_type = nullptr;
ShowInBarFn g_show_in_bar = nullptr;
PouchItemFn g_pouch_item = nullptr;
ClearBarFn g_clear_bar = nullptr;
SetBarSlotFn g_set_bar_slot = nullptr;

// Main update thread only.
bool g_sort_pouch = true;
bool g_sort_bar = true;
FILETIME g_config_time{};
ULONGLONG g_next_config_check = 0;
unsigned g_frame = 0;
bool g_pending = true;
Snapshot g_last{};
bool g_have_last = false;

template <typename... Args>
void SafeLog(std::format_string<Args...> format, Args&&... args) noexcept {
    try {
        log::Log(format, std::forward<Args>(args)...);
    } catch (...) {
        // Logging must never unwind through game code.
    }
}

std::string Join(std::span<const std::uint32_t> ids) {
    std::string text;
    for (const std::uint32_t id : ids) {
        if (id == 0) {
            continue;
        }
        if (!text.empty()) {
            text += ',';
        }
        text += std::to_string(id);
    }
    return text;
}

bool TakeSnapshot(std::uint8_t* user, Snapshot& snapshot) noexcept {
    return mem::Read(user + kPouchBegin, snapshot.pouch.data(), snapshot.pouch.size()) &&
           mem::Read(user + kItemBar, snapshot.bars.data(), snapshot.bars.size() * sizeof(std::uint32_t));
}

std::uint32_t SlotItem(const Snapshot& snapshot, std::ptrdiff_t list_offset, std::size_t slot) noexcept {
    std::uint32_t id = 0;
    std::memcpy(&id, snapshot.pouch.data() + (list_offset - kPouchBegin) + slot * kPouchSlot + kSlotItem, sizeof(id));
    return id;
}

std::array<std::uint32_t, 24> ListItems(const Snapshot& snapshot, const PouchList& list) noexcept {
    std::array<std::uint32_t, 24> ids{};
    for (std::size_t i = 0; i != list.slots; ++i) {
        ids[i] = SlotItem(snapshot, list.offset, i);
    }
    return ids;
}

bool PouchSorted(const Snapshot& snapshot) {
    for (const auto& list : kPouchLists) {
        const auto ids = ListItems(snapshot, list);
        if (!pouch_order::IsSorted(std::span<const std::uint32_t>(ids.data(), list.slots), g_sort_key)) {
            return false;
        }
    }
    return true;
}

struct Bars {
    pouch_order::IdList<kBarLength> items;
    pouch_order::IdList<kBarLength> ammo;
};

// What the item bar editor's "Sort by Item Pouch" lists (builders +0x10C2DC0 and +0x10C27A0 with
// their bar argument 0), minus the special entries the game appends to the bar by itself.
Bars ExpectedBars(std::uint8_t* user) {
    Bars bars;
    const auto add = [](auto& list, std::uint32_t id, bool ammo) {
        if (id != 0 && !list.Contains(id) && g_show_in_bar(id, ammo, false)) {
            list.Add(id);
        }
    };
    const auto is_ammo = [](std::uint32_t id) {
        const std::uint32_t type = id == 0 ? 0 : g_item_type(id);
        return type == kTypeAmmo || type == kTypeCoating;
    };
    for (std::uint32_t i = 0; i != kPouchLists[0].slots; ++i) {
        add(bars.items, g_pouch_item(user, kListItems, i), false);
    }
    for (std::uint32_t i = 0; i != kPouchLists[3].slots; ++i) {
        const std::uint32_t id = g_pouch_item(user, kListTools, i);
        if (!is_ammo(id)) {
            add(bars.items, id, false);
        }
    }
    for (std::uint32_t i = 0; i != 2; ++i) {
        add(bars.items, g_pouch_item(user, kListMantles, i), false);
    }
    for (std::uint32_t i = 0; i != kPouchLists[1].slots; ++i) {
        add(bars.ammo, g_pouch_item(user, kListAmmo, i), true);
    }
    for (std::uint32_t i = 0; i != kPouchLists[3].slots; ++i) {
        const std::uint32_t id = g_pouch_item(user, kListTools, i);
        if (is_ammo(id)) {
            add(bars.ammo, id, true);
        }
    }
    return bars;
}

// Runs under the character's lock on the main update thread, no pouch menu open.
void SortNow(std::uint8_t* user, const Snapshot& before) {
    if (g_sort_pouch && !PouchSorted(before)) {
        g_prepare_sort(user, kSortArea);
        g_apply_sort(user, kSortArea);
        Snapshot after;
        if (TakeSnapshot(user, after)) {
            const auto old_items = ListItems(before, kPouchLists[0]);
            const auto new_items = ListItems(after, kPouchLists[0]);
            SafeLog("Pouch sorted: items [{}] -> [{}].", Join(old_items), Join(new_items));
        }
    }
    if (!g_sort_bar) {
        return;
    }
    const Bars expected = ExpectedBars(user);
    std::array<std::uint32_t, kBarLength * 2> bars{};
    if (!mem::Read(user + kItemBar, bars.data(), bars.size() * sizeof(std::uint32_t))) {
        return;
    }
    const std::span<const std::uint32_t> item_bar(bars.data(), kBarLength);
    const std::span<const std::uint32_t> ammo_bar(bars.data() + kBarLength, kBarLength);
    if (pouch_order::BarStartsWith(item_bar, expected.items.View()) &&
        pouch_order::BarStartsWith(ammo_bar, expected.ammo.View())) {
        return;
    }
    // The same calls the item bar editor makes when it saves (+0x10C4470).
    g_clear_bar(user, kCurrentLoadout);
    for (std::uint32_t i = 0; i != expected.items.size; ++i) {
        g_set_bar_slot(user, i, expected.items.ids[i], kCurrentLoadout);
    }
    for (std::uint32_t i = 0; i != expected.ammo.size; ++i) {
        g_set_bar_slot(user, i, expected.ammo.ids[i], kCurrentLoadout);
    }
    SafeLog("Item bar set to pouch order: items [{}] -> [{}], ammo [{}] -> [{}].", Join(item_bar),
            Join(expected.items.View()), Join(ammo_bar), Join(expected.ammo.View()));
}

bool ReadFlag(const wchar_t* key) noexcept {
    return GetPrivateProfileIntW(L"AutoSort", key, 1, g_config_path.c_str()) != 0;
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
    g_sort_pouch = ReadFlag(L"Pouch");
    g_sort_bar = ReadFlag(L"ItemBar");
    g_pending = true;
    SafeLog("Config: Pouch={} ItemBar={}.", g_sort_pouch ? 1 : 0, g_sort_bar ? 1 : 0);
}

void Tick() {
    const ULONGLONG now = GetTickCount64();
    if (now >= g_next_config_check) {
        g_next_config_check = now + kConfigCheckMs;
        ReloadConfig();
    }
    if (!g_sort_pouch && !g_sort_bar) {
        return;
    }
    if (menus::PouchMenuActive()) {
        g_pending = true;
        return;
    }
    // Only with a character in the world: at the title screen the selected slot is not loaded.
    void* manager = mem::Read<void*>(g_exe + kPlayerManagerRva).value_or(nullptr);
    void* singleton = mem::Read<void*>(g_exe + kUserSingletonRva).value_or(nullptr);
    if (manager == nullptr || singleton == nullptr || g_local_player(manager) == nullptr) {
        return;
    }
    std::uint8_t* user = g_user_accessor(singleton);
    Snapshot snapshot;
    if (user == nullptr || !TakeSnapshot(user, snapshot)) {
        return;
    }
    if (!g_pending && g_have_last && snapshot == g_last) {
        return;
    }
    g_pending = false;
    auto* lock = reinterpret_cast<CRITICAL_SECTION*>(user + kUserLock);
    EnterCriticalSection(lock);
    try {
        // Re-read under the lock; job threads change the pouch under the same lock.
        if (TakeSnapshot(user, snapshot)) {
            SortNow(user, snapshot);
        }
    } catch (...) {
        SafeLog("Sort pass failed with an exception; continuing.");
    }
    LeaveCriticalSection(lock);
    g_have_last = TakeSnapshot(user, g_last);
}

void* __fastcall MainUpdateDetour(void* self) {
    void* result = g_main_update(self);
    if (++g_frame % kTickFrames == 0) {
        Tick();
    }
    return result;
}

bool WriteDefaultConfig() {
    std::ofstream file(g_config_path, std::ios::binary);
    file << kDefaultConfig;
    return static_cast<bool>(file);
}

template <typename Fn>
Fn At(std::uintptr_t rva) {
    return reinterpret_cast<Fn>(g_exe + rva);
}

}  // namespace

bool Install(const std::filesystem::path& config_path) {
    g_exe = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    g_config_path = config_path;

    for (const auto& expected : kCalled) {
        if (!mem::Matches(g_exe + expected.rva, expected.bytes)) {
            log::Log("Auto-sort: game code at +0x{:X} does not match the known build; not installed.", expected.rva);
            return false;
        }
    }
    auto* main_update = g_exe + kMainUpdateRva;
    const bool main_update_hooked = mem::Read<std::uint8_t>(main_update) == std::uint8_t{0xE9};
    if (!mem::Matches(main_update + kMainUpdateBodyOffset, kMainUpdateBody) ||
        !(main_update_hooked || mem::Matches(main_update, kMainUpdatePrologue))) {
        log::Write("Auto-sort: main update code does not match the known build; not installed.");
        return false;
    }
    g_user_accessor = At<UserAccessorFn>(kUserAccessorRva);
    g_local_player = At<LocalPlayerFn>(kLocalPlayerRva);
    g_prepare_sort = At<SortAreaFn>(kPrepareSortRva);
    g_apply_sort = At<SortAreaFn>(kApplySortRva);
    g_sort_key = At<ItemFn>(kSortKeyRva);
    g_item_type = At<ItemFn>(kItemTypeRva);
    g_show_in_bar = At<ShowInBarFn>(kShowInBarRva);
    g_pouch_item = At<PouchItemFn>(kPouchItemRva);
    g_clear_bar = At<ClearBarFn>(kClearBarRva);
    g_set_bar_slot = At<SetBarSlotFn>(kSetBarSlotRva);

    std::error_code error;
    if (!std::filesystem::exists(g_config_path, error) && !WriteDefaultConfig()) {
        log::Write("Auto-sort: could not create the default config; using Pouch=1 ItemBar=1.");
    }
    ReloadConfig();

    if (MH_Initialize() != MH_OK) {
        log::Write("Auto-sort: MinHook initialization failed; not installed.");
        return false;
    }
    if (!menus::Install(g_exe)) {
        MH_Uninitialize();
        return false;
    }
    const MH_STATUS main_status = MH_CreateHook(main_update, reinterpret_cast<void*>(&MainUpdateDetour),
                                                reinterpret_cast<void**>(&g_main_update));
    const MH_STATUS enable_status = main_status == MH_OK ? MH_EnableHook(MH_ALL_HOOKS) : MH_UNKNOWN;
    if (enable_status != MH_OK) {
        log::Log("Auto-sort: hooks failed (main update {}, enable {}); not installed.",
                 MH_StatusToString(main_status), MH_StatusToString(enable_status));
        MH_Uninitialize();
        return false;
    }
    log::Log("Auto-sort: installed (main update +0x{:X}{}, GUI open/close hooked); sorting waits while a "
             "pouch, item box, shop, crafting or item bar menu is open.",
             kMainUpdateRva, main_update_hooked ? " over an existing jump" : "");
    return true;
}

}  // namespace auto_auto_sort::game::autosort
