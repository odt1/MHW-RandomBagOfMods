// MHW-SteamOffline: Monster Hunter: World behaves as if Steam were in offline mode.
//
// The game keeps its Steam connection state in its Steam platform object (constructor 0x142214860
// on 15.23). ISteamUser::BLoggedOn at construction and the SteamServersConnected / Disconnected
// callbacks set a byte at +0x974c, and the game reads it only through the virtual GetOnlineState
// (vtable 0x143503668 + 0x60, function 0x142215220: 2 = online, 0 = offline). IsOnline (+0x78)
// calls it, and the platform update (+0x50) copies it into the state at +0x9744 every frame and
// tells the network context when it changes. The plugin points that vtable slot at a function
// that always answers 0, which is what the game sees when Steam runs in offline mode. Like
// MHW-DLSS4 it patches only a .rdata vtable slot, no game code.

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <mutex>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace {

std::mutex g_logMutex;
FILE* g_log = nullptr;

template <typename... Args>
void Log(std::format_string<Args...> fmt, Args&&... args) {
    std::string text = std::format(fmt, std::forward<Args>(args)...);
    SYSTEMTIME t;
    GetLocalTime(&t);
    std::lock_guard lock(g_logMutex);
    if (!g_log) return;
    std::fprintf(g_log, "%02u:%02u:%02u.%03u %s\n", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, text.c_str());
    std::fflush(g_log);
}

struct Section {
    uint8_t* begin = nullptr;
    uint8_t* end = nullptr;
};

Section FindSection(HMODULE module, std::string_view name) {
    auto* base = reinterpret_cast<uint8_t*>(module);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + reinterpret_cast<IMAGE_DOS_HEADER*>(base)->e_lfanew);
    auto* sec = IMAGE_FIRST_SECTION(nt);
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
        char secName[9] = {};
        std::memcpy(secName, sec->Name, 8);
        if (name == secName) return {base + sec->VirtualAddress, base + sec->VirtualAddress + sec->Misc.VirtualSize};
    }
    return {};
}

// GetOnlineState of the Steam platform: movzx eax, byte [rcx+0x974c]; neg al; sbb eax, eax;
// and eax, 2; ret. Without the offset the same bytes match two other getters.
constexpr uint8_t kGetOnlineState[] = {0x0F, 0xB6, 0x81, 0x4C, 0x97, 0x00, 0x00, 0xF6,
                                       0xD8, 0x1B, 0xC0, 0x83, 0xE0, 0x02, 0xC3};

using GetOnlineStateFn = int32_t (*)(void* platform);

uint8_t* g_base = nullptr;
void** g_slot = nullptr;
GetOnlineStateFn g_original = nullptr;
std::atomic<int32_t> g_lastState{-1};

uintptr_t Rva(const void* p) { return reinterpret_cast<const uint8_t*>(p) - g_base; }

int32_t GetOnlineStateOffline(void* platform) {
    // The real state is only logged, so the log shows that the game asked and what Steam said.
    int32_t state = g_original(platform);
    if (g_lastState.exchange(state) != state)
        Log("Steam connection state {} ({}), the game is told offline", state, state == 2 ? "online" : "offline");
    return 0;
}

bool WriteSlot(void** slot, void* expected, void* value) {
    DWORD oldProtect;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &oldProtect)) return false;
    void* old = InterlockedCompareExchangePointer(slot, value, expected);
    DWORD dummy;
    VirtualProtect(slot, sizeof(void*), oldProtect, &dummy);
    return old == expected;
}

void Start(HMODULE module) {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(module, buf, MAX_PATH);
    fs::path dll(buf);
    g_log = _wfopen((dll.parent_path() / (dll.stem().wstring() + L".log")).c_str(), L"w");
    Log("MHW-SteamOffline {} loaded from {}", MHW_STEAMOFFLINE_VERSION, dll.string());

    HMODULE exe = GetModuleHandleW(nullptr);
    g_base = reinterpret_cast<uint8_t*>(exe);
    Section text = FindSection(exe, ".text");
    Section rdata = FindSection(exe, ".rdata");
    if (!text.begin || !rdata.begin) {
        Log("game sections not found, plugin inactive");
        return;
    }

    uint8_t* getter = nullptr;
    int matches = 0;
    for (uint8_t* p = text.begin; (p = std::search(p, text.end, std::begin(kGetOnlineState), std::end(kGetOnlineState))) != text.end; ++p) {
        getter = p;
        ++matches;
    }
    if (matches != 1) {
        Log("GetOnlineState found {} times (expected once), plugin inactive; is this game version 15.2x?", matches);
        return;
    }

    void** slot = nullptr;
    matches = 0;
    for (auto** p = reinterpret_cast<void**>(rdata.begin); p + 1 <= reinterpret_cast<void**>(rdata.end); ++p) {
        if (*p == getter) {
            slot = p;
            ++matches;
        }
    }
    if (matches != 1) {
        Log("GetOnlineState (+0x{:x}) is in {} vtable slots (expected one), plugin inactive", Rva(getter), matches);
        return;
    }

    g_original = reinterpret_cast<GetOnlineStateFn>(getter);
    if (!WriteSlot(slot, getter, reinterpret_cast<void*>(&GetOnlineStateOffline))) {
        Log("could not write the vtable slot at +0x{:x}, plugin inactive", Rva(slot));
        return;
    }
    g_slot = slot;
    Log("Steam platform GetOnlineState (+0x{:x}, vtable slot +0x{:x}) replaced: the game sees Steam offline",
        Rva(getter), Rva(slot));
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        try {
            Start(module);
        } catch (const std::exception& e) {
            Log("startup failed: {}", e.what());
        } catch (...) {
            Log("startup failed");
        }
    } else if (reason == DLL_PROCESS_DETACH && reserved == nullptr && g_slot) {
        // Unloaded with FreeLibrary while the game keeps running: give the slot back.
        WriteSlot(g_slot, reinterpret_cast<void*>(&GetOnlineStateOffline), reinterpret_cast<void*>(g_original));
    }
    return TRUE;
}
