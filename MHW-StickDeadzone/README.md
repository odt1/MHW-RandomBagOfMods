# MHW Stick Deadzone

Removes the large built-in deadzones of the gamepad sticks, so the hunter and the camera react to the slightest push.

The game ignores the first 30% of the left stick's travel (walking) and the first 25% of the right stick's (camera). This plugin lets you set your own deadzone for each stick and spreads the rest of the stick over the range the game does react to. The game's own response stays the same: the first touch past your deadzone gives its slowest walk or camera turn, full tilt is still full tilt, and the camera keeps its curve and speed settings. You just get more of the stick to work with.

## Features

- A deadzone per stick, from 0 (none) up, set in a small `.ini` file.
- Changes apply within a second while the game runs, so you can tune it while you play.
- The plugin adjusts the sticks after the game has read them, not the pad driver, so it should work with every kind of pad the game supports (XInput, DirectInput, Steam Controller). Only an XInput pad was tested.
- Keyboard and mouse input are left alone.

## Settings

`MHW-StickDeadzone.ini` is created next to the plugin on first launch:

```ini
[LeftStick]
Enabled=1
Deadzone=0.01

[RightStick]
Enabled=1
Deadzone=0.01

[Debug]
LogSticks=0
```

- `Deadzone` is how far a stick must move from its center before the game sees it, from 0.0 to 0.9 (0.05 = 5% of the stick's travel). Raise it if the hunter walks or the camera turns on its own after you let go of the stick. Worn sticks usually need 0.05 to 0.1; precise sticks (Hall effect, TMR) can go down to 0.
- `Enabled=0` leaves that stick to the game, with its original deadzone.
- `LogSticks=1` writes the stick values before and after the plugin to the log once a second while you hold a stick. Use it to see what your stick reports at rest.

## Requirements

- Monster Hunter: World: Iceborne on PC, the final Steam version (15.23). The plugin checks the game executable when it starts and does nothing on any other version.
- [Stracker's Loader](https://www.nexusmods.com/monsterhunterworld/mods/1982).
- Tested with an XInput pad, together with MHW FairTransmog, MHW Auto-Auto-Sort, MHW-DLSS4 and a CRC-bypass plugin (`!CRCBypass.dll`). Other setups are untested.

## Installation

1. Copy `nativePC\plugins\MHW-StickDeadzone.dll` into the game's `nativePC\plugins` folder.
2. Start the game. The plugin creates `MHW-StickDeadzone.ini` and `MHW-StickDeadzone.log` next to itself.

## Uninstalling

Delete `MHW-StickDeadzone.dll`, `.ini` and `.log` from `nativePC\plugins`. Nothing in your save is changed.

## Known limitations

- With Steam Input enabled for the game, Steam applies its own deadzone before the game sees the pad. Set it to 0 in Steam's controller settings for Monster Hunter: World, or turn Steam Input off for the game.
- Only the hunter's movement and the main camera were measured. A few other screens read the sticks with their own built-in deadzones (some aiming and special camera modes, the guild card camera, some menus); they keep part of theirs.
- Every speed the game had is still there, but it now comes a little earlier in the stick's travel, because the part that used to do nothing is used too.
- This is a beta, tested offline. Multiplayer is untested.

## Reporting problems

Include `nativePC\plugins\MHW-StickDeadzone.log`, ideally with `LogSticks=1`. The log is rewritten on every launch, so copy it right after the problem happens.

## Credits

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Developer documentation: [DEVELOPMENT.md](DEVELOPMENT.md).
