# Status and next steps

Read this first when resuming. Evidence for every claim is in [FINDINGS.md](FINDINGS.md); architecture, build and tools in [../DEVELOPMENT.md](../DEVELOPMENT.md).

## Where things stand (2026-10-23)

- Repository: `C:\Projects\MHW Mods\MHW-StickDeadzone`, branch `main`, its own git repo, excluded from the parent FairTransmog repo through `C:\Projects\MHW Mods\.git\info\exclude`.
- Confirmed in game (deployed 01:19): commit `c83cb3c`, DLL SHA-256 `E3F56FF4697348686DF6DCA9CF1E43AC791102B7FA0D33E30C3C211E3939C4BA`. Both sticks remapped as one radius onto the game's own gates (left 0.30, right 0.25). The user: "Finally, a real progress." Settings in use: left deadzone 0.05, right 0.0, LogSticks=1.
- Since then only cleanup: one remap path per stick in `deadzone.cpp`, README.md, DEVELOPMENT.md, FINDINGS rewritten, BN helpers in `tools/bn`, dead-end Frida probes removed. Behavior unchanged. Deployed 01:31:41 (commit `bfea3c9`, DLL SHA-256 `12141574F8D8AA9C4407F50830DBFD3278FA176083895579B347B702E90E26EC`).
- Release defaults chosen by the user: deadzone 0.05 for both sticks (code default and generated ini). The installed ini keeps the user's own test settings.

## Next

1. Version 0.9.0 (as the other two mods did for their first release), README.nexus.bbcode in their style, `tools/package.ps1`.
2. Open, only if the user notices: other stick readers with inline 0.30 or 0.25 cuts (some aim and special camera modes, the guild card camera, some menus), see FINDINGS.

## Environment notes

- Build: in pwsh, `. $PROFILE; devshell`, then `cmake --preset release`, `cmake --build --preset release`, `ctest --preset test-release`. No third-party libraries. Deploy with `tools\deploy.ps1 -BuildDir build\release` while the game is closed; while it runs, a background `while (Get-Process MonsterHunterWorld) { sleep }` followed by the deploy has worked well.
- The installed `MHW-StickDeadzone.ini` holds the user's test settings; the deploy script does not touch it.
- BN: `bnctl open "C:\Projects\MHW Mods\re\inputs\mhw-current\MonsterHunterWorld.exe.unpacked.exe"`; `tools/bn/run_*.sh` go through `bnctl exec`.
- Frida: see FINDINGS, "Tooling incidents". Both crashes of this project came from Frida, not from the plugin.
