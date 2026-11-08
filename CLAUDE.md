# MHW Random Bag of Mods: notes for working on this repository

A monorepo of small Stracker's Loader plugins for Monster Hunter: World (Iceborne, 15.23). Each mod keeps its own CMake project, tools, tests and reverse-engineering notes; this file only covers what is shared.

- `MHW-AutoAutoSort` keeps the item pouch and item bar sorted.
- `MHW-StickDeadzone` replaces the game's gamepad stick deadzones with the user's own.
- `MHW-SteamOffline` makes the game see Steam as offline while Steam stays online.

FairTransmog is **not** in this repository. It lives in its own repo at `C:\Projects\MHW Mods` (remote `odt1/MHW-FairTransmog`) and must not be edited from here.

## Build, test, deploy, package

Every mod builds with MSVC and Ninja. Call `devshell` first in the same pwsh invocation (the tool shell does not load the profile).

```
pwsh: . $PROFILE; devshell
      cd MHW-AutoAutoSort;  cmake --preset release; .\tools\build.ps1   # build + ctest
      cd MHW-StickDeadzone; cmake --preset release; .\tools\build.ps1   # build + ctest
      cd MHW-SteamOffline;  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release; cmake --build build
```

`tools\deploy.ps1` in each mod refuses while `MonsterHunterWorld` runs, backs up the installed DLL and PDB into the mod's `re\deployment-backups\`, then copies and verifies the new ones. `tools\package.ps1` builds `dist\<Mod>-<version>.zip` laid out to extract into the game folder. AutoAutoSort's `deploy.ps1` defaults to `build\release`; FairTransmog's own copy defaults to an old diagnostic build, so always pass `-BuildDir build\release` there.

`build\` is disposable: CMake caches absolute source paths, so after moving or renaming this repository, delete the mod's `build\` and configure again.

## Shared local inputs (not in git)

The game executable, its unpacked copy, the Binary Ninja database and the extracted assets live in the FairTransmog project, not here:

- `C:\Projects\MHW Mods\re\inputs\mhw-current\MonsterHunterWorld.exe.unpacked.exe` and its `.bndb` beside it (open with `bnctl open ...`).
- `C:\Projects\MHW Mods\assets\` for extracted game files (`tools/bn/gmd.py` reads item names from `assets/chunk/common/text/item_eng.gmd`).
- The BN helpers `tools/bn/run_hlil.sh` and `run_callers.sh` call `C:\Projects\tools\binja\bnctl.py exec`.

## Hook targets: do not collide

Several plugins hook the same game objects. Before adding a per-frame hook, check this list and the mod's `DEVELOPMENT.md`:

- FairTransmog: sTicketManager update slot `+0x3404AB0` (code-hooked sMhMain's update `+0x1AF2B60` before 0.9.2).
- MHW-DLSS4: sMhMain update vtable slot `+0x33FA9E8`, TAA draw `+0x351BE08`, mouse update `+0x2F86E78`.
- MHW-AutoAutoSort: item bar HUD update slot `+0x3471E40`; GUI base open/close/destructor via MinHook.
- MHW-StickDeadzone: `sMhSteamController` update slot `+0x2F87168`.
- MHW-SteamOffline: the Steam platform class's `GetOnlineState` vtable slot in `.rdata`.

Never code-hook sMhMain's update `+0x1AF2B60` and never swap its slot `+0x33FA9E8`: two MinHook installs of one target from different DLLs bypass each other, and MHW-DLSS4 finds that slot by its original value. Use `mem::HookPointer` (compare-exchange, retried) for vtable slot swaps so hooks chain.

Live testing: never hard-kill a Frida session that has Interceptor hooks installed; a long session detached from the GUI open/close hooks crashed the game once. Read plugin state with `cdb -pv` or the plugin's own log instead. Ask before attaching Frida, Cheat Engine or a debugger to a game with online anti-cheat.

## Release convention

Tags are `MHW-<Mod>/v<version>` on that mod's own commit, with one GitHub release per tag holding `dist/<Mod>-<version>.zip`. Version numbers come from `project(... VERSION ...)` in each `CMakeLists.txt`; there is no beta suffix. Keep each mod's `README.md`, `README.nexus.bbcode` (where present) and `DEVELOPMENT.md` in step with the version and package name.

## Writing docs

Write Markdown one paragraph or list item per line, never hard-wrapped. `C:\Projects\tools\mdreflow.py FILE... [--check]` rewraps a file.
