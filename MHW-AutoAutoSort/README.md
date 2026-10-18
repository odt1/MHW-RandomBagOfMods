# MHW Auto-Auto-Sort

A Monster Hunter: World (Iceborne, PC) plugin that keeps your item pouch and item bar sorted all the time.

The game can already sort both, but only when you ask: **Auto-sort** in the Item Pouch menu, and **Sort by Item Pouch** for the item bar. This plugin runs those same two game functions for you whenever the pouch changes: an item you gather, carve or pick up, something you craft or auto-craft, items you move from the item box, buy, or load from an item loadout.

- The pouch stays in the game's own Auto-sort order.
- The item bar stays in pouch order, exactly as Sort by Item Pouch leaves it. Items you hid from the item bar stay hidden.
- Sorting never happens while a menu that shows your pouch is open (Item Pouch, item box, item loadouts, shop, crafting list, item bar editor). It happens right after you close it, so nothing moves under your cursor.
- Nothing runs at the title screen or before your hunter is in the world.

## Install

Requires [Stracker's Loader](https://www.nexusmods.com/monsterhunterworld/mods/1982). Copy `MHW-AutoAutoSort.dll` into `<game folder>\nativePC\plugins`.

The plugin checks the game executable's SHA-256 at startup and does nothing on any other build than the one it was made for (the final PC release, `C2EBBBD2…D90EA`). It writes `MHW-AutoAutoSort.log` next to itself; that log says whether it is active.

## Settings

On first start it creates `MHW-AutoAutoSort.ini` next to the DLL:

```ini
[AutoSort]
Pouch=1
ItemBar=1
```

Set either to `0` to turn that part off. Changes apply within a second, no restart needed.

## Notes

- Manual item bar arrangements are replaced by pouch order the next time the pouch or the bar changes. That is the point of the mod; turn `ItemBar` off to keep your own order.
- It only touches your own pouch and item bar, never the item box, and never adds or removes items.
- Do not unload or replace the DLL while the game runs.

## Uninstall

Delete `MHW-AutoAutoSort.dll` (and the `.ini` and `.log`) from `nativePC\plugins`. Your pouch stays as it was last sorted.

Licensing of included third-party code: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
