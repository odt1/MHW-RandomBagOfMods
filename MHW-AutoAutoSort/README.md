# MHW Auto-Auto-Sort

Keeps your item pouch and item bar sorted all the time, without pressing a button.

The game can sort both, but only when you ask: **Auto-sort** in the Item Pouch, and **Sort by Item Pouch** in the item bar editor. This plugin runs those same two game functions for you every time the pouch changes: an item you gather, carve or pick up, something you craft or auto-craft, items you take from the item box, buy, or load from an item loadout.

## Features

- The pouch (items, ammo and coatings, materials) stays in the game's own Auto-sort order.
- The item bar stays in pouch order, exactly as Sort by Item Pouch leaves it. A new item shows up at its sorted place right away and the selected item stays selected. Items you hid from the item bar stay hidden, and the Palico gadget entry stays at the end.
- Nothing moves under your cursor: sorting waits while a menu that shows your pouch is open (Item Pouch, item box, item loadouts, shop, crafting list, item bar editor) and happens right after you close it.
- Only the game's own sorting is used. Items are never added, removed or moved between the pouch and the item box.

## Settings

`MHW-AutoAutoSort.ini` is created next to the plugin on first launch:

```ini
[AutoSort]
Pouch=1
ItemBar=1
```

- `Pouch=0` stops sorting the pouch. `ItemBar=0` stops reordering the item bar, so you can keep your own arrangement.
- Changes apply within about a second while the game runs; no restart needed.

## Requirements

- Monster Hunter: World: Iceborne on PC, the final Steam version (15.23). The plugin checks the game executable when it starts and does nothing on any other version.
- [Stracker's Loader](https://www.nexusmods.com/monsterhunterworld/mods/1982).
- Tested with Stracker's Loader together with MHW FairTransmog, MHW-DLSS4 and a CRC-bypass plugin (`!CRCBypass.dll`). Other setups are untested.

## Installation

1. Back up your save (Steam: `<Steam>\userdata\<your id>\582010\remote`).
2. Copy `nativePC\plugins\MHW-AutoAutoSort.dll` into the game's `nativePC\plugins` folder.
3. Start the game. The plugin creates `MHW-AutoAutoSort.ini` and `MHW-AutoAutoSort.log` next to itself.

## Uninstalling

Delete `MHW-AutoAutoSort.dll`, `.ini` and `.log` from `nativePC\plugins`. The pouch and item bar stay as they were last sorted.

## Known limitations

- This is a beta, tested offline on a single character. Multiplayer is untested.
- A manual item bar arrangement is replaced by pouch order the next time the pouch or the bar changes. Set `ItemBar=0` to keep your own order. Item bars saved in item loadouts are not changed.
- Sorting runs along with the on-screen item bar, so while the HUD is hidden it may wait until the HUD shows again.

## Reporting problems

Include `nativePC\plugins\MHW-AutoAutoSort.log`. It is rewritten on every launch, so copy it right after the problem happens.

## Credits

Built with [MinHook](https://github.com/TsudaKageyu/minhook). See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Developer documentation: [DEVELOPMENT.md](DEVELOPMENT.md).
