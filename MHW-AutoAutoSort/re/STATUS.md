# Status and next steps

Read this first when resuming. Evidence for every claim is in [FINDINGS.md](FINDINGS.md); architecture and build steps in [../DEVELOPMENT.md](../DEVELOPMENT.md).

## Where things stand (2026-10-19 09:10)

- Repository: `C:\Projects\MHW Mods\MHW-AutoAutoSort`, branch `main`, its own git repo, excluded from the parent FairTransmog repo through `C:\Projects\MHW Mods\.git\info\exclude`.
- Deployed and confirmed working in game: commit `a282061`, `MHW-AutoAutoSort.dll` SHA-256 `F6608F1B129EE7330E416E1F8F0BE28B21BC7B4476BBEF84590910EAA6C1E248` in `F:\SteamLibrary\steamapps\common\Monster Hunter World
ativePC\plugins`. The previous DLL is in `re/deployment-backups/20261019T034639372-…`.
- This build runs the sort tick from the item bar HUD update `uGUIHudItemWindow` (`sub_141E2B890`, vtable slot `+0x3471E40`, swapped with a compare-exchange), not from sMhMain's update, which FairTransmog and MHW-DLSS4 contest. It keeps the Palico gadget entry (843 for gadget 0) and the other game-added IDs at the end of the bar, and logs every changed pouch list slot by slot.
- 08:57–09:08 session (FINDINGS, "Live result, HUD update build"): the HUD hook ran, 21 sort passes with no rewrite bursts, new items and ammo moved to their sorted place, used-up slots closed up, the gadget entry stayed at the end. The HUD update runs on several job threads; passes are serialized by the plugin's try-lock and the character lock. The user: "seems to work proper".

## Still open (low priority, needs the user in game)

1. Whether the item bar selection always stays on its item after a reorder (no problem reported).
2. Whether sorting continues while the HUD is hidden (HUD display options, cutscenes).
3. FairTransmog's ini hot reload with both plugins installed (not exercised).
4. Multiplayer sessions.

## Reading the plugin's state in a running game

Non-invasive (no breakpoints, threads resume on quit); the PDB sits next to the DLL:

```powershell
$p = (Get-Process MonsterHunterWorld).Id
cdb -pv -p $p -y "F:\SteamLibrary\steamapps\common\Monster Hunter World\nativePC\plugins" -c 'x MHW_AutoAutoSort!*g_pending*; x MHW_AutoAutoSort!*g_have_last*; x MHW_AutoAutoSort!*g_last_close*; x MHW_AutoAutoSort!*g_reported_thread*; dq MonsterHunterWorld+0x3471e40 L1; q'
```

Avoid long Frida sessions with Interceptor hooks on the GUI open/close functions: detaching one crashed the game (FINDINGS, "Tooling incident"). Read-only Frida scripts without hooks detach cleanly.

## Environment notes

- Build: in pwsh, `. $PROFILE; devshell` first (the tool shell does not load the profile; `devshell` comes from it), then `cmake --build --preset release` and `ctest --preset test-release`. Deploy with `tools\deploy.ps1 -BuildDir build\release` only while the game is closed.
- BN: `bnctl open "C:\Projects\MHW Mods\re\inputs\mhw-current\MonsterHunterWorld.exe.unpacked.exe"` resumes the shared database (FairTransmog's). The `binja-codemode-mcp` tools may not be connected in a new session; `tools/bn/run_hlil.sh RVA…` and `tools/bn/run_callers.sh RVA…` go through `bnctl exec` instead. `tools/bn/gmd.py` reads game text (`assets/chunk/common/text/item_eng.gmd`, name of item `id` at entry `2*id`) from the FairTransmog project's `assets`.
- Live tests so far ran on the user's character in save slot 0 (hub and field). FairTransmog's convention of using the TEST character (slot 3) was not followed here; sorting only reorders the pouch and the bar.
- Other plugins in the folder that hook the game: MHW-FairTransmog (code hook on sMhMain update `+0x1AF2B60`), MHW-DLSS4 0.9.4 (vtable slots of that update, TAA draw and mouse update), MonsterHunterFixorld, MHW-SteamOffline, BetterInputs, DPSTickFix, CutsceneSkip, QuestLoader, MonsterLoader, !CRCBypass.

## Known issues and ideas

- FairTransmog has the same latent race on `+0x1AF2B60` with any other plugin that code-hooks it at startup; with this plugin moved off it, the remaining risk there is other plugins. Worth fixing in FairTransmog the same way (vtable slot, chained).
- Next candidates: package with `tools\package.ps1` and write a Nexus description (FairTransmog has `README.nexus.bbcode` as a model); fix FairTransmog's main update hook the same way.
