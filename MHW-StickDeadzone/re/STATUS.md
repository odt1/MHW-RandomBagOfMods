# Status and next steps

Read this first when resuming. Evidence for every claim is in [FINDINGS.md](FINDINGS.md).

## Where things stand (2026-10-23 00:40)

- Repository: `C:\Projects\MHW Mods\MHW-StickDeadzone`, its own git repo, excluded from the parent FairTransmog repo through `C:\Projects\MHW Mods\.git\info\exclude`. Runtime (`src/runtime`), bootstrap, tests and tools are copied from MHW-AutoAutoSort and renamed.
- Deployed, not yet tried in game: 0.1.0, DLL SHA-256 `35552C03F7BFD362DF0A24A8661D343829AEA6FEB873B3DFFFA89B2026010637`. The installed `MHW-StickDeadzone.ini` holds the test settings: both deadzones 0.01, `[Debug] LogSticks=1`.
- How it works: vtable slot swap on `sMhSteamController`'s update (`+0x2F87168`, `mem::HookPointer`). After the original converts the sticks, the left stick is remapped radially from the user's deadzone onto the game's cut (0.30 from `rPlayerParam+0x124/+0x134`) to 1, the right stick per axis onto 0.15 (`+0x12C/+0x130`) to 1, in the converted block and in each context copy that keyboard or mouse did not replace. Game thresholds and curves are untouched.

## Live result, 0.1.0 (2026-10-23 01:00)

- User, with calibrated TMR sticks: "Much better now." Scaling "about right". Left stick: wants deadzone 0.05, then reacts to the slightest touch. Right stick: "still needs >0.05 to start moving even with 0 deadzone set." Log confirms the remap (right -0.012 in, -0.160 to the game).
- Cause (inferred from FINDINGS, main camera): the camera squares each axis, so 0.15 to 0.19 give 2.3% to 3.6% of full speed, too slow to see.
- Deployed 01:03:35 (commit `8647a00`): with Response=Linear the user said "Now it triggers correctly, but the response itself feels off. Game was perfect by feel." Linear is replaced by `[RightStick] MinSpeed` (default 0.05): the travel starts at sqrt(MinSpeed) on the game's range instead of the cut, so the first touch turns the camera at 5% and the game's squared curve takes it to full speed. Waiting for the game to close to deploy; the installed ini has left 0.05, right 0.0, MinSpeed=0.05, LogSticks=1.

## Next

1. User test: walking from small tilts, camera turning from small tilts, full tilt and running unchanged. Read `MHW-StickDeadzone.log` (`Sticks:` lines show before and after).
2. If the user confirms, set release defaults (probably 0.05; ask), write README.md and README.nexus.bbcode in the style of the other two mods, package.
3. Open: other right-stick consumers with inline 0.30 or 0.25 cuts (aim modes, guild card camera, some menus) keep part of their deadzone; only change them if the user notices.

## Environment notes

- Build: in pwsh, `. $PROFILE; devshell`, then `cmake --preset release`, `cmake --build --preset release`, `ctest --preset test-release`. No vcpkg dependency (no MinHook). Deploy with `tools\deploy.ps1 -BuildDir build\release` while the game is closed.
- BN: `bnctl open "C:\Projects\MHW Mods\re\inputs\mhw-current\MonsterHunterWorld.exe.unpacked.exe"`; MHW-AutoAutoSort's `tools/bn/run_hlil.sh` and `run_callers.sh` work through `bnctl exec`.
- Frida: see FINDINGS, "Tooling incident". Two crashes this session came from Frida, not from game code.
