# Auto-Auto-Sort reverse-engineering findings

Every claim is marked **observed** (seen in disassembly, decompiled code, memory, or a live trace), **inferred** (concluded from observations), or **unknown**. Static evidence comes from Binary Ninja HLIL of `MonsterHunterWorld.exe.unpacked.exe` SHA-256 `FE50D8468881FF8CB094C5F9A599A5D6B2965950331A22EE34148C612651C521` (image base `0x140000000`; the analysis copy and BN database live in the FairTransmog project under `re/inputs/mhw-current/`). Live evidence comes from Frida 17.23.3 attached to the installed game, packed `MonsterHunterWorld.exe` SHA-256 `C2EBBBD2C49F216D484E31A5219BED419EB1E5E7D206D02CBA040A3AB79D90EA`, character slot 0, in the hub, 2026-10-19. All addresses are RVAs.

## Character data and the item pouch

- **Observed, HLIL `sub_141B8DBB0` (RVA `0x1B8DBB0`):** returns `*(singleton+0xA8) + *(s32*)(singleton+0xA0) * 0x26CC00` for `singleton = *(exe+0x5013950)`. This is the character data block ("user") the FairTransmog project already uses.
- **Observed, live:** item slot objects in the user block are 16 bytes: vtable RVA `0x2F206B0`, `+0x8` u32 item ID, `+0xC` u32 count. A run of 77 such objects starts at `user+0x38080`; another of 2,150 starts at `user+0x38A08` (item box).
- **Observed, HLIL `sub_141355970` (RVA `0x1355970`, item ID getter `(user, list, index)`):** list 0 reads `user+0x38088+i*0x10` (items), 1 `+0x38208` (ammo and coatings), 2 `+0x38388`, 3 `+0x38508`, 4 the two equipped specialized tools (`sub_141672A50` of `user+0xC4+i*4`, or an equipment-box entry when `user+0x2691A0` is set).
- **Observed, live:** list 0 held Potion, Antidote, Ration, Raw Meat, Empty Phial (IDs 1, 5, 9, 65, 174); list 1 eleven ammo IDs; list 3 Whetstone, Capture Net, Fishing Rod, BBQ Spit (IDs 114–117; names from `item_eng.gmd`, name index `2*id`). List 2 (24 slots at `+0x38380`) was empty. **Unknown:** what list 2 holds.

## Auto-sort (pouch)

- **Observed, HLIL `sub_14135D610` (prepare, RVA `0x135D610`, `(user, area)`):** under the user lock (`EnterCriticalSection(user+8)`), for each list of the area calls `sub_14135D7D0(user, list, count, table)`, which fills a static pointer table (`data_144FA4940`, `…4A00`, `…4A80`, `…4B40` for area 0–2) with the filled slots in order of `sub_1418978C0(item)` (sort key, item data `+0x14`). Insertion goes after every slot whose key is not greater: a stable ascending sort, empty slots (item ID 0) left out.
- **Observed, HLIL `sub_14134AB90` (apply, RVA `0x134AB90`, `(user, area)`):** for each list of the area, `sub_14134ADA0` copies the slots the table points to into scratch `data_144FA8EA0` (empty where the table is null), then writes them back into the list with `sub_141187E50` (set ID) and `sub_1411CEA20` (set count).
- **Observed, area mapping (both functions):** 0 = lists 0 and 1 (24 + 16), 1 = list 2, 2 = lists 0, 1, 2 and 3, 3 = box items and ammo, 4 = all four box lists.
- **Observed, live trace (`trace_sort.js`, user pressed Auto-sort in the start menu's Item Pouch):** `sub_14135D610(user, 2)` from `+0x1E75D8D` (in `sub_141E75B60`, the menu's button handler, which then refreshes its five tabs), then one second later `sub_14134AB90(user, 2)` from `+0x1E75256` (`sub_141E75220`, the confirmation callback: `if (*arg3 == 0)`). Both on the main thread. Slot writes came from `+0x134AE5A` under that call.
- **Inferred:** prepare followed by apply with area 2, under the user lock, is the menu's Auto-sort. Calling both back to back without the menu does the same; the static tables are shared with the menu, so nothing may run while the Item Pouch menu shows its preview.

## Sort by Item Pouch (item bar)

- **Observed, HLIL `sub_1413550A0` / `sub_141353570`:** the current item bar order is 40 u32 item IDs at `user+0xEDF38`, the ammo bar 40 at `user+0xEDFD8`; loadout `n` keeps its own at `user+0x9E0+n*0x768` and `+0xA80`.
- **Observed, live:** item bar `[1,5,9,65,114,115,116,117]`, ammo bar the eleven ammo IDs in pouch order; Empty Phial (not usable) absent.
- **Observed, HLIL, item bar editor `uGUIItemWindowReplace` (DTI `0x144F8DB80`, size `0x4A40`):** the confirmation callback `sub_1410C0070` rebuilds its item list with `sub_1410C2DC0(gui, 0)` and `sub_1410C27A0` (ammo), i.e. without the old bar order: pouch list 0, list 3 entries whose type (`sub_1418978F0`, item data `+0x10`) is not 1 or 4, list 4, then special entries (IDs `0x3ED`, `0x6F6`, `0x7AC`, palico gadgets) under game conditions; the ammo list is pouch list 1 plus list 3 entries of type 1 or 4. Every entry must pass `sub_141895E80(item, ammo, loadout)` (third argument 0 for the current bar); among other things it rejects items hidden from the bar (bit set in `user+0x26A264`, toggled in the Item Pouch menu).
- **Observed, live trace (`trace_bar.js`):** saving that order is `sub_1410C4470`: `sub_1413504C0(user, -1)` (clear both bars) from `+0x10C44B5`, then `sub_141368690(user, i, item, -1)` per entry from `+0x10C4503` (seen with `(0,1) (1,5) (2,9) (3,65) (4,114) (5,115)`), which writes through `sub_141368900` into the item or ammo bar by item type, and records the previous entry in the link table at `user+0xEE078`.
- **Inferred:** writing the pouch-derived list with those two calls reproduces the game's sort; the special entries are added back by the game's own code (`sub_1413687F0` from the bar builders), after the pouch entries.

## Menus

- **Observed, DTI registration strings:** `uGUIItemPouch` (DTI `0x14502B000`, size `0x32E0`, vtable `0x14347A800`, constructor `sub_141E74510`), `uGUIItemWindowReplace`, `uGUIBoxItemReplace`, `uGUIBoxItemMySet`, `uGUIBoxItemSell`, `uGUIItemShop`, `uGUISupplyBox`, `uGUINumSelectPouch`, `uGUIItemReplace(Sub)`, `uGUIItemMake(List)`, `uGUIRingShortcutEditor(PC)`, `uGUIShortCutEditWindow`. MtObject vtable slot 4 is getDTI (`lea rax,[rip+dti]; ret`); the DTI's name pointer is at `+8`.
- **Observed, HLIL:** the GUI base constructor `sub_141FB3D10` (437 callers) initialises a state word at `+0x2298`; 291 sites test `state & 0x80000011`. `sub_141FB79B0` sets bit 0 and clears bit 1 (open); `sub_141FB55E0` sets bit 1 (close); `sub_141FB4210` is the base destructor.
- **Observed, live (`gui_track.js`, `gui_open.js`):** no GUI was constructed or destroyed while the user opened and closed menus; the same instances were reused. Every open passed through `sub_141FB79B0` and every close through `sub_141FB55E0`: start menu Item Pouch (`uGUIItemPouch`), the item bar editor (`uGUIItemWindowReplace`), item box transfer (`uGUIBoxItemReplace`), item loadouts (`uGUIBoxItemMySet`), shop (`uGUIItemShop`, opened together with `uGUIBoxItemReplace`), crafting list (`uGUIItemMakeList` + `uGUIItemMake`). The Item Pouch's state word read `0x60400C9` open and `0x60400CA` closed.
- **Inferred:** a menu is open while bit 0 of its state word is set; tracking the open/close calls per instance and filtering by DTI name finds the menus that show pouch slots by position.

## Plugin behaviour (`src/game`)

- `autosort.cpp` hooks sMhMain's per-frame update (`0x1AF2B60`, the same one FairTransmog hooks; MinHook chains over another plugin's jump). Every sixth frame, with a local player (`sub_141B42010(*(exe+0x500ECA0))` non-null) and no pouch menu open, it compares a snapshot of the pouch block (`+0x38080`–`+0x38550`) and both bars with the previous one. After a change it takes the user lock, runs prepare and apply with area 2 when a list is out of order, and rewrites the bars with the editor's two calls when their filled entries do not start with the pouch-derived lists. The snapshot taken afterwards stops it from repeating a write the game rejects.
- `menus.cpp` hooks the GUI base open, close and destructor and keeps the open/closed state of instances whose DTI name is in its list; sorting waits while one is open and for 300 ms after the last close.

## Live result, 0.1.0 beta (2026-10-19)

Deployed `MHW-AutoAutoSort.dll` 0.1.0-beta next to FairTransmog and other plugins; installed at startup without "over an existing jump" (main update still unhooked at load time).

- **Observed, plugin log 02:35–02:53:** 25 pouch sorts and 5 item bar rewrites in about 17 minutes, all on the main update thread (one thread ID), no failures. New items were sorted in as they arrived, e.g. Gunpowder Level 2 (102) into `[1,2,5,59,81,102,181,183,193,174]`, then Nulberry, Herb, Ivy, mushrooms and bugs one by one, ending with 24 item slots in sort order.
- **Observed, user-confirmed:** pouch and item bar stay sorted, including when the item pouch overflows (pouch full while gathering).
- **Observed, log:** the item bar held a special entry 843 after the pouch-derived entries (`item_eng.gmd` has no name for it, "Invalid Message"). The rewrite drops it; later lines show it back at the end of the bar. **Inferred:** the game appends it again by itself; one extra bar rewrite per pouch change, no loop.
- **Observed, log:** several "Pouch sorted" lines showed the same item order before and after. That log printed only filled item slots. **Inferred:** those sorts closed a gap left by a used-up item, or reordered the ammo or tool list. Later builds log every changed list slot by slot, `_` for an empty slot.
- **Unknown:** whether the HUD's selected item-bar entry follows the item or its position when the bar is rewritten during a hunt (user reported no problem, not asked specifically).

## Tooling incident

- **Observed, 2026-10-19 02:21:46:** the game crashed (`frida-agent.dll`, exception `0xC0000409`) when a Frida session with hooks on `sub_141FB79B0` and `sub_141FB55E0` reached its `-t 300` timeout and detached. Earlier sessions with other hooks detached cleanly. **Inferred:** detaching Frida from these hooks is unsafe; the plugin itself does not use Frida.

## Open questions

- **Unknown:** whether the HUD's selected item-bar entry follows the item or its position when the bar is rewritten during a hunt (see the 0.1.0 live result).
- **Unknown:** pouch list 2 contents; multiplayer behaviour; master-rank or story states where `sub_141895E80` filters differently.
