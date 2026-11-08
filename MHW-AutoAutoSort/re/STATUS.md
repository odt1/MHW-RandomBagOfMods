# Status and next steps

Read this first when resuming. Evidence for every claim is in [FINDINGS.md](FINDINGS.md); architecture and build steps in [../DEVELOPMENT.md](../DEVELOPMENT.md).

## Where things stand (2026-10-19 09:30)

- Repository: `C:\Projects\MHW-RandomBagOfMods`, branch `main`, a subfolder of the MHW-RandomBagOfMods monorepo (`MHW-AutoAutoSort/`); FairTransmog is a separate repo at `C:\Projects\MHW Mods`.
- Deployed and confirmed in game: commit `a282061` (0.1.0), DLL SHA-256 `F6608F1B129EE7330E416E1F8F0BE28B21BC7B4476BBEF84590910EAA6C1E248`. The user confirmed sorting, the item bar selection staying on its item, and FairTransmog's hot reload alongside it (FINDINGS, "Live result, HUD update build").
- Deployed 2026-10-19 09:33 and confirmed in game (09:43 session): 0.9.0, `dist/MHW-AutoAutoSort-0.9.0-beta.zip` (SHA-256 `547A04853650BDCB999698DC6CBD74375E74432B1AB7608780F89E7EA9DE9DFC`), DLL SHA-256 `3B704AB5715A9AE57D3E0B93FDFA2BFF1FA518D518389D0E0697303E53DF0D4C`. Review changes: handled-state snapshot taken under the character lock, `mem::HookPointer` for the slot hook, materials list named, player README and `README.nexus.bbcode` in FairTransmog's style.
- FairTransmog 0.9.2 (parent repo) built and packaged with the same slot-hook fix: its tick moved from sMhMain's update to sTicketManager's update (slot `+0x3404AB0`); `dist/MHW-FairTransmog-0.9.2-beta.zip`, DLL SHA-256 `F19F726CFF22F8E93311E4FEAA0EEFCDAC0C9F9E9FDBDE1C49464E2BAE8C2EE7`, deployed 09:33 and confirmed in game (frame tick on the main thread, ini hot reload).

## Verified, and what is left

Done 2026-10-19 09:43 session (FINDINGS, "Live result, 0.9.0 with FairTransmog 0.9.2"):

1. Auto-Auto-Sort log: `Auto-sort: installed (item bar HUD update slot +0x3471E40, …)`, then `Auto-sort: active (first item bar HUD update on thread N)` once in the world; pickups sort as before.
2. FairTransmog log: `Transmog: direct override hooks installed (frame tick slot +0x3404AB0, refresh +0x2041700)`; an ini edit (for example `All=300`, then `-1`) logs `Config:` and a refresh override within a second, on the main thread.
Still open, low priority: sorting while the HUD is hidden; multiplayer.

## Reading the plugin's state in a running game

Non-invasive (no breakpoints, threads resume on quit); the PDB sits next to the DLL:

```powershell
$p = (Get-Process MonsterHunterWorld).Id
cdb -pv -p $p -y "F:\SteamLibrary\steamapps\common\Monster Hunter World\nativePC\plugins" -c 'x MHW_AutoAutoSort!*g_pending*; x MHW_AutoAutoSort!*g_have_last*; x MHW_AutoAutoSort!*g_last_close*; x MHW_AutoAutoSort!*g_reported_thread*; dq MonsterHunterWorld+0x3471e40 L1; q'
```

Avoid long Frida sessions with Interceptor hooks on the GUI open/close functions: detaching one crashed the game (FINDINGS, "Tooling incident"). Read-only Frida scripts without hooks detach cleanly.

## Environment notes

- Build: in pwsh, `. $PROFILE; devshell` first (the tool shell does not load the profile; `devshell` comes from it), then `cmake --build --preset release` and `ctest --preset test-release`. Deploy with `tools\deploy.ps1 -BuildDir build\release` only while the game is closed. FairTransmog's `tools\deploy.ps1` defaults to `build\diagnostic`, an old build; always pass `-BuildDir build\release` there too (on 2026-10-19 the default installed that old DLL until it was redeployed).
- BN: `bnctl open "C:\Projects\MHW Mods\re\inputs\mhw-current\MonsterHunterWorld.exe.unpacked.exe"` resumes the shared database (FairTransmog's). The `binja-codemode-mcp` tools may not be connected in a new session; `tools/bn/run_hlil.sh RVA…` and `tools/bn/run_callers.sh RVA…` go through `bnctl exec` instead. `tools/bn/gmd.py` reads game text (`assets/chunk/common/text/item_eng.gmd`, name of item `id` at entry `2*id`) from the FairTransmog project's `assets`.
- Live tests so far ran on the user's character in save slot 0 (hub and field). FairTransmog's convention of using the TEST character (slot 3) was not followed here; sorting only reorders the pouch and the bar.
- Other plugins in the folder that hook the game: MHW-FairTransmog (up to 0.9.1 a code hook on sMhMain update `+0x1AF2B60`; from 0.9.2 the sTicketManager update slot `+0x3404AB0`, plus code hooks on the model refresh and layered menu), MHW-DLSS4 0.9.4 (vtable slots of sMhMain update `+0x33FA9E8`, TAA draw `+0x351BE08` and mouse update `+0x2F86E78`), MonsterHunterFixorld, MHW-SteamOffline, BetterInputs, DPSTickFix, CutsceneSkip, QuestLoader, MonsterLoader, !CRCBypass.

## Known issues and ideas

- Ready to publish both zips; the Nexus description is `README.nexus.bbcode` (no banner image yet).
