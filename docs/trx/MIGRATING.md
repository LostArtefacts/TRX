---
title: Migrating levels
order: 3
---

# Migration guide for level builders

## TRX

### Version 1.9 to 1.10

1. **Update scripts that use item handles**
   `trx.items` hands out opaque item handles rather than `{ idx = ... }` tables.
   - `trx.items.fn` was removed. Replace `trx.items.fn.get(arg)` with
     `trx.items.get(arg)`, or index the module directly: `trx.items[17]`,
     `trx.items["lara"]`.
   - `item.idx` no longer exists, and a handle can no longer carry extra keys of
     your own. Pass the handle itself where you used to pass the index.
   - A handle to a killed or recycled item now raises `stale ITEM handle` when
     read or written, instead of silently addressing whatever item took over the
     slot. Guard handles held across time with `item:is_valid()`.
   - Writing a value a field cannot hold now raises instead of truncating - e.g.
     `item.hit_points = 99999`, where the field is 16-bit.
   - `pairs(item)` iterates the item's public fields; it used to yield `idx`
     alone.

2. **Update scripts that use room handles**
   `trx.rooms` hands out opaque room handles rather than `{ idx = ... }` tables.
   - `room.idx` was replaced by `room.num`. Both count from 1.
   - `trx.rooms.fn` was removed. Replace `trx.rooms.fn.get(num)` with
     `trx.rooms.get(num)`, or index the module directly: `trx.rooms[15]`.
     `trx.rooms.fn.FlipStatus` is now `trx.rooms.FlipStatus`, and
     `trx.rooms.fn.Room`, `trx.rooms.fn.flip` and `trx.rooms.fn.flip_effect`
     became `trx.rooms.Room`, `trx.rooms.flip` and `trx.rooms.flip_effect`.
   - An unset room flag now reads as `false` rather than `nil`, so
     `if room.underwater == nil` no longer detects a dry room. Test the flag
     itself.
   - Room flags accept booleans only. `room.wind = 1` and `room.cold = nil` used
     to be silently accepted and now raise.
   - `trx.rooms["5"]` no longer resolves - index with a number, not a numeric
     string.
   - Writing to an out-of-range room raises instead of silently doing nothing.

3. **Update scripts that compare `trx.config.get()` against a string**
   It returns the option's own type now: a boolean option reads as a boolean and
   a number as a number. `trx.config.get("flow.cheat_keys") == "true"` is now
   false whatever the setting says - drop the comparison and test the value:
   `if trx.config.get("flow.cheat_keys") then`. Colors and enums are still
   strings.

   `trx.config.set()` still writes to the player's settings and keeps the
   change. Use the new `trx.config.override()` for anything a level wants only
   while it is running - it leaves the player's own value underneath, and
   `trx.config.restore()` puts it back.

4. **Update scripts that read `trx.lara.extra_anim`**
   It is a boolean now - whether a scripted animation is driving Lara - where
   it used to be the relative animation number of `O_LARA_EXTRA`, or `-1` when
   she was in no such animation. `if trx.lara.extra_anim ~= -1` is now always
   true, and comparing it against a number raises. Test the boolean:
   `if trx.lara.extra_anim then`. The animation number itself is no longer
   exposed; read `trx.lara.item.anim` if you need it.

5. **Update scripts that compensate for `on_pickup`'s item number**
   The handler passes a 1-based item number, as it is documented to; it used to
   pass the 0-based engine index. A script that
   worked around this with `trx.items[item_num + 1]` is now off by one, and
   picks up the wrong item rather than failing - pass `item_num` straight
   through.

6. **Update scripts that write to a catalog**
   The catalogs, and every other enum, are read-only now. Writing to one used to
   succeed and silently break every later lookup, the engine's own scripts
   included. Reading is unchanged, and a catalog answers to a name in any case:
   `trx.catalog.objects.wolf` and `trx.catalog.objects.WOLF` are the same
   constant. Upper case is the documented spelling.

7. **Update scripts that use Lara's mesh tables**
   `trx.lara.mesh` and `trx.lara.extra_mesh` became declared enums, so their
   names are upper case: replace `trx.lara.mesh.hand_r` with
   `trx.lara.Mesh.HAND_R`, and `trx.lara.extra_mesh.oar` with
   `trx.lara.ExtraMesh.OAR`.

8. **Update scripts that read a level's `name`**
   The field is called `title` now. It holds what it always did, the name shown
   to the player. Replace `trx.game.levels[1].name` with
   `trx.game.levels[1].title`. Every field on a level is read-only.

9. **Update scripts that use `trx.game.settings`**
   It was removed. It was five aliases over config options, and it wrote through
   `trx.config.set`, which keeps the change. Use `trx.config` directly, and
   prefer `trx.config.override` for anything a level wants only while it runs:
   - Before: `trx.game.settings.play_any_level = true`
   - After: `trx.config.override("flow.play_any_level", true)`

10. **Update scripts that assign to `item.object_id`**
   It is read-only now. Writing it swapped the item's type underneath the
   engine without reinitializing it, which left the item half-built. Spawn the
   type you want instead: `trx.items.spawn(trx.catalog.objects.WOLF, pos)`.

11. **Update scripts that set `pickup_mode`**
   The `trx.pickup` module is gone. `pickup_mode` is an item property, so its
   enum now lives with the items: replace `trx.pickup.Mode.PLINTH_LOW` with
   `trx.items.PickupMode.PLINTH_LOW`.

12. **Update scripts that index `trx.objects` with an unknown id**
   `trx.objects[id]` returns `nil` for an id the game does not have, where it
   used to hand back an object that answered to nothing, so `if trx.objects[id]`
   now tells a missing object from a present one. Code that went straight on to
   `trx.objects[id].properties` raises at the lookup rather than further along.

13. **Update scripts that log more than one string per call**
   The logging functions take a single message. `trx.console.log("a", "b")`
   used to concatenate its arguments and now raises. Format the message before
   passing it: `trx.console.log(("a %s"):format(b))`. This applies to `trx.console.log` and
   its levels, and to `trx.log`.

14. **Update scripts that use event types**
   `trx.events.EventType` was removed, along with the `._type` field on each
   hook. The nine hooks are the whole API, and attaching is unchanged:
   `trx.events.before_control(fn)`.

15. **Update scripts that use `trx.console.log.LogLevel`**
   It was removed. Use `trx.log.LogLevel`, which is the same enum:
   `trx.console.log.generic(trx.log.LogLevel.ERROR, "...")`.

16. **Update scripts that call `trx.music.play_track`**
   It was an undocumented alias of `trx.music.play` and was removed. Use
   `trx.music.play(id[, opts])`.

17. **Update scripts that address levels by number**
   `trx.game.play_level` and the `trx.game.levels` list leaves out the gym.
   In a game with a gym `trx.game.levels[1]` used to be the gym; drop any offset
   that stepped over it; use `trx.game.play_gym()` and `trx.game.gym`.

18. **Update Lara's outfit definitions**
   The `hair_pos` entries for Lara's braids were changed to `positions`. This
   field is now an array (of at most two values) rather than a single position.
   Update `outfits.json5` accordingly.

### Version 1.8 to 1.9

1. **Update Lara pushblock animations**
   Lara's pushblock animations (non-continuous) are now split to line up with
   the length of the animations of the blocks themselves. Ensure to update
   `catalog_lara_anims.csv` and either `lara_animations.bin` or the Lara object
   in your level WADs.

2. **Update Lara's outfit definitions and samples**
   The `footstep_sample_id` SFX reference was removed from Lara's outfit
   definitions and replaced with an `is_barefoot` flag. Update `outfits.json5`
   and `catalog_samples.csv` accordingly - refer to OG shipped assets.

3. **Update game mode selection config option**
   `enable_game_modes` (boolean) was changed to `game_modes_policy`, with the
   options being `never`, `always` and `on-completion`. Update the gameflow if
   this setting is enforced.

4. **Update O_SPARKS_GFX sprites**
   The `O_SPARKS_GFX` sprites from TR3 were combined with TR4. Download the TR3X
   assets file from https://lostartefacts.dev/pub/tr3-assets.zip, or use the
   shipped `sparks_gfx.bin` injection.

### Version 1.7 to 1.8

1. **Update Assault Course Lua stats access**:
   The separate `trx.assault_stats` module has been merged into
   `trx.assault.stats`.
   - Before: `trx.assault_stats.add_record(30.0)`
   - After: `trx.assault.stats.add_record(30.0)`

2. **TR3 sparks object was renamed**:
   In `cfg/catalog_objects.csv`, update the following object name:
    - `O_EXPLOSION_1` → `O_SPARKS_GFX`

3. **Update fish/piranha setup**:
   Fish and piranha objects no longer require a timer field to be set in
   triggers, and instead their swim range needs to be defined in Lua. Refer to
   the OG TR3 level scripts for reference.
   The `O_EXPLOSION_1` sprite sequence is no longer used for these objects. Use
   `fish_sprites.bin` for TR3 levels, or define `O_PIRAHNA_GFX` and
   `O_TROPICAL_FISH_GFX` in your level WAD. The TRX assets WAD for TR3 contains
   the default setup.

4. **Update bat emitter sprites**:
   The `O_EXPLOSION_1` sprite sequence is no longer used for bat emitters. Use
   `bat_sprites.bin` for TR3 levels, or define `O_BAT_GFX` in your level WAD.
   The TRX assets WAD for TR3 contains the default setup.

5. **Update Cobra setup**:
   Cobras in level sequence 9 and above are no longer hard-coded to have a small
   attack, forget and alert radius. Use Lua to specify this setup if required.

6. **Update quest item end-level handling**:
   TR3's quest items will no longer end the level by default when picked up. Use
   Lua or regular pickup triggers instead; refer to the OG TR3 level scripts for
   reference.

7. **Update side flame emitters**:
   `O_FLAME_EMITTER_SIDE` instances will no longer have a hard-coded 4 second
   interval in level sequence 7; all instances will default to 2 seconds. Refer
   to the Madubu Gorge Lua script to alter the interval.

8. **Update spikes sound effects**:
   Animated spikes in TR3 are no longer hard-coded to play specific sound
   effects in levels 5 and 7 only. Regular animation commands can be used
   instead to play in any level.

9. **Update AI Patrol 1**:
   Levels with sequence 14 and 15 are no longer hard-coded to retain
   `O_AI_PATROL_1` items where an enemy should have the AI bits set but also use
   the item as a pathing target. Instead, place two `O_AI_PATROL_1` items in the
   same position to retain behaviour.

10. **Vehicles and heavy triggers**:
   All vehicle types except for the mounted gun can now activate heavy triggers.
   This is configurable per object and item in Lua, and the setting is enabled
   by default. Disable the option in cases where this may interfere with
   triggers intended for other heavy activators e.g. pushblocks. This is not
   configurable for the mine cart, which still relies on Lara striking switches.

11. **Replace `cold_water` with room flags**:
   The `cold_water` game-flow property has been removed. Use room flags instead:
   - `damaging` controls Lara's exposure meter.
   - `cold` controls Lara's visible breath.

   You can set these flags from Lua:
   - `trx.rooms[room_num].damaging = true`
   - `trx.rooms[room_num].cold = true`

12. **Replace disposable animating objects**:
   `O_DISPOSABLE_ANIMATING_1…10` have been removed. Use
   `O_ANIMATING_EXT_1…10` instead, and set the `kill_on_trigger` item property
   to `true` when you want the old disposable behavior.

### Version 1.6 to 1.7

1. **Update Lua item maximum HP access**:
   The direct `item.max_hit_points` Lua field has been removed. Use item
   properties instead:
   - Before: `item.max_hit_points = 20`
   - After: `item.properties.max_hit_points = 20`

### Version 1.5 to 1.6

1. **TR1 and TR2 blood catalog names were renamed**:
    In `cfg/catalog_objects.csv`, update old symbols to the new names:
    - `O_BLOOD_1` → `O_BLOOD`

    This also affects catalog-derived Lua names (`trx.catalog.objects`):
    - `blood_1` → `blood`

2. **Update weapon ammo quantities**:
   In `weapons.json5`, the old `pickup_qty` and `pickup_qty_alt` fields have
   been reorganized under a new nested `ammo` object. This lets weapon pickups
   grant a different amount of ammo than their matching ammo pickups.

   To match the previous setup:

   1. Open `weapons.json5`.
   2. For each weapon entry:
      - Create a nested `ammo` object if it doesn't already exist.
      - Move the value from `pickup_qty` into both `ammo.initial_qty` and
        `ammo.pickup_qty` fields.
      - Fill `ammo.inventory_qty` field.
   3. If the weapon had a `pickup_qty_alt` field (e.g. flares):
      - Move that value into `ammo.pickup_qty_alt`.
   4. Remove the old `pickup_qty` and `pickup_qty_alt` fields.

3. **TR1 Atlantean catalog names were changed**:
   In `cfg/catalog_objects.csv`, update old symbols to the new names:
    - `O_WARRIOR_1` → `O_ATLANTEAN_WINGED`
    - `O_WARRIOR_2` → `O_ATLANTEAN_SHOOTER`
    - `O_WARRIOR_3` → `O_ATLANTEAN_GROUND`

    This also affects catalog-derived Lua names (`trx.catalog.objects`):
    - `warrior_1` → `atlantean_winged`
    - `warrior_2` → `atlantean_shooter`
    - `warrior_3` → `atlantean_ground`

### Version 1.4 to 1.5

1. **Update TR2 detonator box**
   Dynamic light output when using the detonator is no longer hard-coded and now
   uses animation commands. The updated OG asset is available to download 
   [here](INJECTIONS.md#builder-workflow-keep-the-codebincode-or-bake-into-your-wad).

### Version 1.3 to 1.4

1. **Update strings file structure**
   The flat string section has been replaced with nested root sections.
   Please see shipped string files or documentation for details.

### Version 1.2 to 1.3

1. **TR1 missile catalog names were renamed**:
    In `cfg/catalog_objects.csv`, update old missile symbols to the new names:
    - `O_MISSILE_1` → `O_NATLA_GUN`
    - `O_MISSILE_2` → `O_MISSILE_ATLANTEAN_SHARD`
    - `O_MISSILE_3` → `O_MISSILE_ATLANTEAN_BOMB`
    - `O_MISSILE_4` and `O_MISSILE_5` are no longer used and should be removed.

    This also affects catalog-derived Lua names (`trx.catalog.objects`):
    - `missile_1` → `natla_gun`
    - `missile_2` → `missile_atlantean_shard`
    - `missile_3` → `missile_atlantean_bomb`

2. **TR2 breakable window catalog names were renamed**:
    In `cfg/catalog_objects.csv`, update old breakable windows to the new names:
    - `O_WINDOW_1` → `O_SMASH_OBJECT_1`
    - `O_WINDOW_2` → `O_SMASH_OBJECT_2`

    This also affects catalog-derived Lua names (`trx.catalog.objects`):
    - `window_1` → `smash_object_1`
    - `window_2` → `smash_object_2`

3. **Flooding flip effect sound ID was changed**:
    In `cfg/catalog_samples.csv`, add an alias for `SFX_FLOOD`:
    - TR1: `81, SFX_FLOOD`
    - TR2: `79, SFX_FLOOD`

### Version 1.1 to 1.2

1. **Lara skin system**:  
    Lara's outfit must now be defined using additional skin objects, along with
    game-flow and JSON setup. Refer to [outfits documentation](OUTFITS.md).

2. **Lua event name cleanup**:  
    The following events got new names:
    - `on_level_init` → `before_level_file`
    - `on_level_start` → `after_level_file`
    - `on_level_load` → `after_level_state`
    - `on_control` → `before_control`
    - `on_control_post` → `after_control`

3. **Lua objects catalog name cleanup**:  
    All keys in `trx.catalog.objects` had their `O_` prefix removed and were
    converted to lowercase.  
    Before: `trx.catalog.objects.O_BANDIT_1`  
    After: `trx.catalog.objects.bandit_1`

4. **Savegame file pattern rename**:  
    Replace `savegame_fmt_bson` with `savegame_file_fmt` in game flow files.
    The old `savegame_fmt_bson` key is still accepted but logs a warning and is
    scheduled for removal in TRX 1.5.

5. **Legacy savegame pattern removed**:  
    Remove the `savegame_fmt_legacy` key from game flow files.

### Version 1.0 to 1.1

1. **Ally and ally target behavior moved to Lua**:  
    Monks being allies and bandits being enemies who will target allies is no
    longer hardcoded and instead must be defined in Lua. Refer to the game flow
    and linked script files of the original levels for reference.

## TR1X

### Version 4.15 to TRX 1.0

1. **Game flow options moved to the config module**:
    Certain settings are no longer part of the game flow spec and instead
    became hidden player settings. To change them, put them in the
    `enforced_config` section. List of the affected settings:
    - `demo_delay`
    - `enable_killer_pushblocks`

2. **Lara shotgun animation**: 
   Lara now uses the TR2+ approach of a separate shotgun mesh on her back. You
   must use the `lara_guns.bin` injection or otherwise refer to 
   https://github.com/LostArtefacts/TRXInjectionTool/blob/main/docs/ASSETS.md

3. **Lara extra animations**: 
   Lara now uses the TR2+ approach of having defined state changes for extra
   animations (scion pickups, Midas touch etc). You must use the `lara_extra.bin`
   injection or otherwise refer to 
   https://github.com/LostArtefacts/TRXInjectionTool/blob/main/docs/ASSETS.md

### Version 4.13 to 4.14

1. **Update file paths**  
   - Move and rename the `cfg/TR1X_gameflow.json5` file to `cfg/tr1/gameflow.json5`.
   - Move and rename the `cfg/TR1X_strings*.json5` files to `cfg/tr1/strings*.json5`.
   - Move and rename the `cfg/TRX_common_strings*.json5` files to `cfg/base_strings*.json5`.
   - Remove leftover `TR1X_strings_ub.json5`.

    This is how the directory should look:
    ```
    .
    └── cfg
        ├── base_strings.json5
        ├── base_strings-pl.json5 (in case you want to provide translation files)
        ├── base_strings-….json5 (in case you want to provide translation files)
        ├── tr1
        │   ├── gameflow.json5
        │   ├── strings.json5
        │   ├── strings-pl.json5 (in case you want to provide translation files)
        │   └── strings-….json5 (in case you want to provide translation files)
        └── poses.json5
    ```

### Version 4.9 to 4.10

1. **Update fog configuration**  
    If you wish to force your fog settings on player:
    - Rename `draw_distance_fade` to `fog_start`
    - Rename `draw_distance_max` to `fog_end`

    If you wish to give the player agency to change the fog:
    - Remove `draw_distance_fade` and `draw_distance_max`

### Version 4.7 to 4.8

1. **Rename basic keys**  
   - Replace `file` key with `path` for every level.
   - Replace `music` key with `music_track` for every level.

2. **Update level enumeration structure**:
   - The `"type": "title"` property is no longer supported. Instead, the title
     level needs to be placed in the top-level `"title"` key.
   - The `"type": "cutscene"` property is no longer supported. Instead, the
     cutscenes need to be placed in the top-level `"cutscenes"` array.
   - All FMVs need to be placed in its own top-level `"fmvs"` array.

3. **Update individual level sequences**  
   - `start_game` should be removed.
   - `exit_to_cine` should be removed.
   - `exit_to_level` should be replaced with `level_complete`. No parameter needed.
   - `display_picture` no longer takes a `picture_path` argument and instead just takes a `path`.
   - `loading_screen` no longer takes a `picture_path` argument and instead just takes a `path`.
   - `level_stats` no longer takes a `level_id` argument.
   - `total_stats` no longer takes a `picture_path` argument and instead takes a `background_path`.
   - `play_fmv` no longer takes a `fmv_path` argument and instead takes a `fmv_id`.
   - `play_synced_audio` is renamed to `play_music` and takes a `music_track` argument rather than `audio_id`.

4. **Update strings**  
   The game strings are now placed in a separate file, `TR1X_strings.json5` in
   preparation to eventually support internationalization. Elements such as
   item titles or item names need to be configured entirely in the new file, so
   all `"strings"` keys can be safely removed from the game flow. Refer to
   [game strings documentation](4-GAME_STRINGS.md) for more details.



## TR2X

### Version 1.5 to TRX 1.0

1. **Game flow options moved to the config module**:
    Certain settings are no longer part of the game flow spec and instead
    became hidden player settings. To change them, put them in the
    `enforced_config` section. List of the affected settings:
    - `lockout_option_ring`
    - `load_save_disabled`
    - `play_any_level`
    - `demo_delay`
    - `cheat_keys`
    - `enable_killer_pushblocks`

2. **Removed game flow settings**
    The following game flow features were removed and are no longer available:
    - `cmd_init`
    - `cmd_title`
    - `cmd_death_in_demo`
    - `cmd_death_in_game`
    - `cmd_demo_end`
    - `cmd_demo_interrupt`
    - `single_level`
    - `is_demo_version`

3. **Lara extra animations**: 
   Lara's extra animations have been combined with TR1. You must use the
   `lara_extra.bin` injection or otherwise refer to 
   https://github.com/LostArtefacts/TRXInjectionTool/blob/main/docs/ASSETS.md

4. **Secret track**:
   The setting `secret_track` is no longer present – the engine will always
   play `MX_SECRET` track. To change its slot, please refer to the
   `catalog_music.csv` file.

### Version 1.3 to 1.4

1. **Update file paths**  
   - Move and rename the `cfg/TR2X_gameflow.json5` file to `cfg/tr2/gameflow.json5`.
   - Move and rename the `cfg/TR2X_strings*.json5` files to `cfg/tr2/strings*.json5`.
   - Move and rename the `cfg/TRX_common_strings*.json5` files to `cfg/base_strings*.json5`.
   - Remove leftover `TR2X_strings_ub.json5`.

    This is how the directory should look:
    ```
    .
    └── cfg
        ├── base_strings.json5
        ├── base_strings-pl.json5 (in case you want to provide translation files)
        ├── base_strings-….json5 (in case you want to provide translation files)
        ├── tr2
        │   ├── gameflow.json5
        │   ├── strings.json5
        │   ├── strings-pl.json5 (in case you want to provide translation files)
        │   └── strings-….json5 (in case you want to provide translation files)
        └── poses.json5
    ```

### Version 1.2 to 1.3

1. **Rename objects**
    - Replace `"detonator_1"` with `"gong"`.
    - Replace `"detonator_2"` with `"detonator_box"`.

2. **Re-add pistols**  
   Pistols are no longer added automatically to a level that follows one in
   which Lara previously lost her weapons. A game flow entry to re-add pistols
   will be required - refer to the Diving Area level in the default game flow.

3. **Bears, wolves and ice warriors**  
   If you wish to use the bear, wolf or ice warrior (monk with no shadow) from
   The Golden Mask while still being able to use big spiders, small spiders and
   other monks, use the following object slots.
    - Bear: slot 265
    - Wolf: slot 266
    - Ice warrior: slot 267

4. **Disabling gym**
    The option `gym_enabled` is no longer available. If you need to remove the
    access to Lara's Home, please either remove the relevant level from the
    game flow (this may break existing saves), or change its type to `"dummy"`
    to get it ignored (this will work with existing saves).

### Version 1.0.2 to 1.1

1. **Update first level inventory allocation**  
   The first level no longer hard-codes the shotgun, flare and small/large medi
   pack allocations. To continue to have Lara start with these items, refer to
   the shipped game flow file's `Great Wall` sequences, specifically the
   `give_item` entries.
