# MHW Auto-Auto-Sort: development

Player documentation is in [README.md](README.md). Reverse-engineering evidence, with every claim marked observed, inferred or unknown, is in [re/FINDINGS.md](re/FINDINGS.md).

## How it works

A Stracker's Loader plugin. `DllMain` starts a worker thread that opens the log, hashes the game executable on disk, and installs hooks only when the SHA-256 matches the one supported build (`src/runtime/profile.h`). The first 16 bytes of every called or hooked game function are compared with the known build first; any mismatch installs nothing. GUI hooks use MinHook; the HUD update is a vtable slot swap (`mem::HookPointer`: compare-exchange, retried when another plugin changes the slot at the same moment, so both hooks chain). All addresses are RVAs in the unpacked image of that build.

- `src/game/autosort.cpp` hooks the item bar HUD's per-frame update (`uGUIHudItemWindow`, RVA `0x1E2B890`) by swapping its vtable slot (RVA `0x3471E40`) and calling whatever the slot held. It does not patch sMhMain's update: FairTransmog code-hooks it and MHW-DLSS4 swaps its slot, and two code hooks installed at the same moment can bypass each other. Before each HUD update, with a hunter in the world and no pouch menu open, it snapshots the pouch block (`user+0x38080`–`+0x38550`) and both item bar orders (`user+0xEDF38`, `+0xEDFD8`). When the snapshot changed, under the character lock (`user+8`):
  - if any pouch list is out of the game's sort-key order, it calls Auto-sort's prepare `sub_14135D610(user, 2)` and apply `sub_14134AB90(user, 2)`, the calls the Item Pouch menu makes;
  - it builds the list "Sort by Item Pouch" would save (pouch items, non-ammo tools, specialized tools; ammo list and ammo-type tools; each passing `sub_141895E80`), and if the bar's filled entries do not start with it, clears the bars with `sub_1413504C0(user, -1)` and writes each entry with `sub_141368690(user, i, item, -1)`, the calls the item bar editor makes when it saves.
- `src/game/menus.cpp` hooks the GUI base class's open (`0x1FB79B0`), close (`0x1FB55E0`) and destructor (`0x1FB4210`). On open it reads the instance's class name through its getDTI stub and remembers instances of the pouch menus listed there; sorting waits while one of them is open (open hook seen and bit 0 of its state word `+0x2298` set) and for 300 ms after the last close.
- `src/game/pouch_order.h` holds the pure ordering checks, unit-tested in `tests/test_pouch_order.cpp`.
- `src/runtime` (log, guarded memory access, executable identity) is shared with MHW FairTransmog.

## Build and test

Requirements: Windows x64, Visual Studio 2026 (MSVC, Windows SDK), CMake 3.25+, Ninja, vcpkg with `VCPKG_ROOT` set (MinHook comes from `vcpkg.json`, static triplet), PowerShell 7.

From a developer shell (`devshell` in this environment):

```powershell
cmake --preset release
cmake --build --preset release
ctest --preset test-release
```

`tools/build.ps1` runs all three. CTest covers the memory helpers, executable identity hashing, a bootstrap test that loads the DLL into a disposable unsupported host and checks the exact fail-closed log, the pouch-order logic, and the deployment script. Host tests are not in-game tests.

## Deploy and package

- `tools/deploy.ps1` refuses while the game runs, backs up the installed DLL and PDB into `re/deployment-backups/`, copies and verifies the new ones.
- `tools/package.ps1` builds `dist/MHW-AutoAutoSort-<version>-beta.zip` (DLL in `nativePC\plugins`, README, notices).
- `README.nexus.bbcode` is the Nexus Mods description; keep it in step with `README.md`.

## Live tools (`tools/live`)

Frida scripts used for the reverse engineering. Run with `frida -q -t <seconds> -n MonsterHunterWorld.exe -l <script>` and keep the game running until Frida exits; never `--kill-on-exit`. Detaching from the GUI open/close hooks crashed the game once (see FINDINGS), so prefer short runs of `read_pouch.js` and close the game rather than detach long hook sessions.

| Script | Purpose |
|---|---|
| `read_pouch.js` | read-only: pouch lists, item bar and ammo bar |
| `trace_sort.js` | counts calls to every function referencing the pouch offset; found Auto-sort |
| `trace_bar.js` | item bar writers and pouch-sort calls with backtraces; found Sort by Item Pouch |
| `gui_track.js`, `gui_open.js` | GUI construction, open and close with DTI class names |
| `find_item_runs.js` | locates runs of item slot objects in the character data |

Binary Ninja helpers (`tools/bn`, need BN open on the game database via `bnctl`): `run_hlil.sh RVA…` prints HLIL, `run_callers.sh RVA…` prints callers, `gmd.py FILE REGEX` searches game text files. Current state and next steps: [re/STATUS.md](re/STATUS.md).

## Source boundaries

Native code in `src`, tools in `tools`, host tests in `tests`, evidence in `re`. The game executable, its unpacked copy and the BN database are local inputs kept in the FairTransmog project, not here.
