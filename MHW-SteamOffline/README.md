# MHW-SteamOffline

A plugin for Monster Hunter: World (Iceborne, PC) that makes the game behave as if Steam were in
offline mode, while Steam itself stays online.

The game starts offline, as it does when Steam is set to offline mode: the HUD shows "Offline" and
the game does not join or create online sessions. Steam itself is not affected: your friends list,
chat and other games keep working.

Remove the plugin to play online again.

## Requirements

- Monster Hunter: World with Iceborne, the latest Steam version (tested on 15.23)
- [Stracker's Loader](https://www.nexusmods.com/monsterhunterworld/mods/1982)

## Install

Extract the zip into the game folder, so that `MHW-SteamOffline.dll` ends up in
`Monster Hunter World\nativePC\plugins`.

## Uninstall

Delete `MHW-SteamOffline.dll` and `MHW-SteamOffline.log` from `nativePC\plugins`.

## Log

`nativePC\plugins\MHW-SteamOffline.log` says whether the plugin is active. A line like
`Steam connection state 2 (online), the game is told offline` means the game asked and was told
that Steam is offline.

## Licence

BSD 2-Clause, see `MHW-SteamOffline-LICENSE.txt`.
