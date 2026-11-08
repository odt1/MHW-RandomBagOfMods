# MHW Random Bag of Mods

Small Stracker's Loader plugins for Monster Hunter: World: Iceborne (PC, final Steam version 15.23). Each mod lives in its own folder with its own README, build files and reverse-engineering notes; this repository only collects them and gives each one a home.

| Mod | What it does |
|---|---|
| [MHW-AutoAutoSort](MHW-AutoAutoSort/README.md) | Keeps the item pouch and item bar sorted after every pouch change, using the game's own Auto-sort and Sort by Item Pouch. |
| [MHW-StickDeadzone](MHW-StickDeadzone/README.md) | Replaces the game's 30% left-stick and 25% right-stick deadzones with your own, spreading the rest of the stick over the range the game reacts to. |
| [MHW-SteamOffline](MHW-SteamOffline/README.md) | Makes the game behave as if Steam were in offline mode while Steam itself stays online. |

Each mod is independent: install any of them on its own, or all three together. They hook different game code and are tested alongside MHW Fair Transmog and MHW-DLSS4.

## Requirements

- Monster Hunter: World: Iceborne on PC, the final Steam version (15.23). Every plugin checks the game executable when it starts and does nothing on any other version.
- [Stracker's Loader](https://www.nexusmods.com/monsterhunterworld/mods/1982).

## Installation

Each mod's release zip is laid out to be extracted into the game folder, so the DLL lands in `Monster Hunter World\nativePC\plugins`. See the mod's own README for settings, uninstall steps and known limitations.

## Building

Windows x64, Visual Studio 2026 (MSVC, Windows SDK), CMake 3.25+, Ninja and PowerShell 7. MHW-AutoAutoSort also needs vcpkg with `VCPKG_ROOT` set (MinHook comes from its `vcpkg.json`).

```
pwsh: . $PROFILE; devshell
      cd MHW-AutoAutoSort; cmake --preset release; .\tools\build.ps1   # build + ctest
      cd MHW-StickDeadzone; cmake --preset release; .\tools\build.ps1  # build + ctest
      cd MHW-SteamOffline; cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
      cmake --build build
```

`tools\deploy.ps1` in each mod copies the built DLL and PDB into the game's `nativePC\plugins` (refuses while the game runs), and `tools\package.ps1` builds the release zip under `dist\`. See each mod's `DEVELOPMENT.md` for the architecture and `re\FINDINGS.md` for the reverse-engineering evidence behind it.

## Licence

BSD 2-Clause, see [LICENSE](LICENSE). Third-party notices are in each mod's `THIRD_PARTY_NOTICES.md`.
