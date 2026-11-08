# MHW-SteamOffline: notes for working on this project

Stracker's Loader plugin for Monster Hunter: World (Iceborne, 15.23) that makes the game believe Steam is in offline mode. C++20, MSVC, CMake + Ninja, one source file (`src/dllmain.cpp`). Version in `project(... VERSION ...)` in `CMakeLists.txt`. Licence BSD-2-Clause, "Copyright (c) 2026, odt1". Part of the MHW-RandomBagOfMods monorepo; the root `CLAUDE.md` has the shared build notes, the game folder and the hook-target list. Sister project of MHW-DLSS4 (`C:\Projects\MHW DLSS`): its CLAUDE.md has the game folder, test workflow (launch via `steam://rungameid/582010`, end with `Stop-Process -Name MonsterHunterWorld -Force`), the unpacked exe for RE (`re\` there) and the RE tool scripts.

## Build, deploy, package

```
pwsh:  . $PROFILE; devshell; cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
pwsh:  . $PROFILE; devshell; cmake --build build
tools\deploy.ps1       copies build\MHW-SteamOffline.dll + .pdb into nativePC\plugins (game closed);
                       -Remove deletes them
tools\package.ps1      dist\MHW-SteamOffline-<version>.zip: DLL, README, LICENSE
```

## How it works (15.23 exe, absolute VAs)

- The game's Steam platform class (factory `0x1421f63e0` type 0x16, constructor `0x142214860`, object size 0x97d0, vtable `0x143503668`) holds the connection state in a byte at +0x974c. The constructor sets it (and +0x974d) from `ISteamUser::BLoggedOn` (SteamUser021 slot +0x8; those are the game's only two BLoggedOn calls) and the initial state +0x9744 = 2 or 0. Callbacks: SteamServersConnected (101, `0x142214c30`) sets +0x974c/+0x974d, Disconnected (103, `0x142214c40`) clears +0x974d; the update clears +0x974c 10 s later.
- Vtable +0x60 `GetOnlineState` (`0x142215220`): 2 when +0x974c is set, else 0. Its only reader of +0x974c that matters. +0x78 `IsOnline` = (+0x60)() == 2. The update (+0x50, `0x1422153c0`) copies (+0x60)() into +0x9744 every frame and on a change notifies the listener (nNetwork::Context, `0x1425ab110` -> `0x1425ab3e0`): state 0 puts the context in its state 2, the same state it reaches at boot when the platform is up.
- The plugin finds `GetOnlineState` by its exact bytes (the pattern without the 0x974c offset also matches two other getters), finds its only `.rdata` vtable slot and replaces it with a function that logs the real state on change and returns 0. No game code is patched. The slot is restored on FreeLibrary.
- Load timing: with Stracker's Loader the plugin loads before the platform object exists; the constructor still reads BLoggedOn = true (+0x9744 = 2), so the first update sends one 2 -> 0 notification that real offline mode would not send. Harmless in the test (hub showed "Offline").

## Tested

- 0.1.0: game started through Steam (online), plugin log "Steam connection state 2 (online), the game is told offline" 4 s after load, Astera HUD shows "Offline".
