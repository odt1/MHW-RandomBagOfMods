# MHW Stick Deadzone: development

Player documentation is in [README.md](README.md). Reverse-engineering evidence, with every claim marked observed, inferred or unknown, is in [re/FINDINGS.md](re/FINDINGS.md); current state and next steps in [re/STATUS.md](re/STATUS.md).

## How it works

A Stracker's Loader plugin. `DllMain` starts a worker thread that opens the log, hashes the game executable on disk, and installs the hook only when the SHA-256 matches the one supported build (`src/runtime/profile.h`) and the first 16 bytes of the hooked function match. All addresses are RVAs in the unpacked image of that build.

The game reads the selected pad once per frame in the pad singleton's update (class `sMhSteamController`, RVA `0x1B168C0`), converts each stick axis to a float with no deadzone, and copies the sticks into two contexts. The deadzones come later and are all plain cuts (below the cut is 0, above it the value is used unchanged):

- left stick: the hunter's controller treats the stick as neutral below a length of `rPlayerParam+0x124` / `+0x134` (both 0.30);
- right stick: the main camera turns only while the stick's length reaches `rPlayerParam+0x128` (0.25), then zeroes each axis below `+0x12C` / `+0x130` (0.15) and turns at the axis squared.

`src/game/deadzone.cpp` swaps the update's vtable slot (RVA `0x2F87168`, `mem::HookPointer`: compare-exchange, retried when another plugin changes the slot at the same moment, so both hooks chain). After the original runs, each stick is remapped as one radius (`src/game/stick_math.h`, `stick::Radial`): travel past the user's deadzone is spread over the game's cut to 1, direction kept. The cuts are read from `rPlayerParam` every second, with the values above as fallback. The remap is written to the converted block and to each context copy that still holds the converted value; a copy that keyboard or mouse input replaced is left alone. Game thresholds, curves and settings are not touched.

The `.ini` is re-read on the game thread when its write time changes, at most once a second.

`src/runtime` (log, guarded memory access, executable identity) is shared with MHW FairTransmog and MHW Auto-Auto-Sort.

## Build and test

Requirements: Windows x64, Visual Studio 2026 (MSVC, Windows SDK), CMake 3.25+, Ninja, PowerShell 7. No third-party libraries.

From a developer shell (`devshell` in this environment):

```powershell
cmake --preset release
cmake --build --preset release
ctest --preset test-release
```

`tools/build.ps1` runs all three. CTest covers the memory helpers, executable identity hashing, a bootstrap test that loads the DLL into a disposable unsupported host and checks the exact fail-closed log, the stick math (including the game's gates for movement and camera), and the deployment script. Host tests are not in-game tests.

## Deploy and package

- `tools/deploy.ps1 -BuildDir build\release` refuses while the game runs, backs up the installed DLL and PDB into `re/deployment-backups/`, copies and verifies the new ones.
- `tools/package.ps1` builds `dist/MHW-StickDeadzone-<version>-beta.zip` (DLL in `nativePC\plugins`, README, notices).

## Tools

Binary Ninja helpers (`tools/bn`, need BN open on the game database through `bnctl`):

| Script | Purpose |
|---|---|
| `run_hlil.sh RVA…` | HLIL of the functions containing each RVA |
| `run_callers.sh RVA…` | callers of each function |
| `run_dti.sh CLASS…` | an MT Framework class's DTI object and vtable, from its name |
| `run_disp.sh OFFSET…` | every instruction using a displacement, to find a field's readers and writers |

Frida scripts used for the reverse engineering (`tools/live`). Run with `frida -q -t <seconds> -n MonsterHunterWorld.exe -l <script>` and let `-t` end the session; never kill a session that has hooks attached, and keep the number of sessions in one game process low (see FINDINGS, "Tooling incidents"). Do not use Frida hardware watchpoints in this game.

| Script | Purpose |
|---|---|
| `acc_trace.js` | callers of the pad singleton's stick accessors, with context |
| `plctl.js` | the hunter's controller object, its class and its stick fields |
| `params.js` | `rPlayerParam` stick thresholds and the pad's stick-as-button threshold |
| `singles.js` | game singletons by DTI class name |
| `copycnt.js` | how often the camera copies its thresholds and applies its per-axis cut |

## Source boundaries

Native code in `src`, tools in `tools`, host tests in `tests`, evidence in `re`. The game executable, its unpacked copy and the BN database are local inputs kept in the FairTransmog project (`..\re\inputs\mhw-current`), not here.
