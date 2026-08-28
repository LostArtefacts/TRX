---
title: Migrating levels
order: 3
---

# Migration guide for level builders

## TRX

### Version 1.10 to 1.11

1. **The waterfall mist object was renamed**
   The object that emits waterfall mist is `waterfall_mist` in every game,
   leaving the `waterfall` name to TR4's animated waterfall meshes. A script
   naming the old one, and a strings file overriding its name, need updating:
   - `trx.catalog.objects.WATERFALL` is now `trx.catalog.objects.WATERFALL_MIST`
   - the string key `objects/waterfall/name` is now
     `objects/waterfall_mist/name`

2. **A color is a value, not a string**
   Anything that reads a color hands back a `trx.math.Color`, which carries its
   `r`, `g` and `b` channels and the hex text as `hex`. A script comparing one
   with a string needs the text:
   - `trx.config.get("visuals.water_color") == "0080ff"` becomes
     `trx.config.get("visuals.water_color").hex == "0080ff"`

   Writing is unchanged: hex text is still taken, as is a color.

3. **Drop the golden outfits**
   Lara turns to gold in whichever outfit she has on, so the golden outfits are
   gone from `outfits.json5`, and with them:
   - the gold gun map,
   - the braid `gold_offset`,
   - the `BRAID_MODE_TR1_GOLD` mode,
   - the two gold braid extra meshes and their equivalent Lua
     `trx.lara.ExtraMesh` enum members,
   - the `is_reflective` property.

   Remove them from your own file, and from any game flow or `lara_outfit`
   setting that names one; `extra_outfits` no longer takes
   `LS_EXTRA_MIDAS_KILL` either. An outfit says what gold it turns with
   `gold_color`, and the mesh objects the golden ones used are free for outfits
   of your own.

4. **`trx.game.end_level()` is silent**
   The call ends the level without announcing it; the "Level complete!"
   message belongs to the `/endlevel` console command. A script that wants the
   message shown prints it itself.

5. **`trx.game.trx_version` is now a constant**
   The build's own version never changes while the game runs, so it is spelled
   as the constant it is:
   - `trx.game.trx_version` is now `trx.game.TRX_VERSION`

6. **A cutscene is reached by number**
   `trx.cutscenes[30]` is the cutscene itself, and `trx.cutscenes.current`
   hands over one of those rather than a number:
   - `trx.cutscenes.current == 5` becomes
     `trx.cutscenes.current == trx.cutscenes[5]`, or
     `trx.cutscenes.current.num == 5`

   The cutscene events hand one over as well, `on_cutscene_trigger` apart,
   which still names a number because a trigger may name one no scene
   answers to:
   - `trx.events.on_cutscene_start(function(num)` becomes
     `function(cutscene)`, and `num` becomes `cutscene.num`
   - `trx.events.on_cutscene_end` the same, as does the new
     `trx.events.on_cutscene_frame`

   The functions taking a number still work and say what to use instead:
   - `trx.cutscenes.play(5)` becomes `trx.cutscenes[5]:play()`, taking
     `{ fade = false }` where it took `false`
   - `trx.cutscenes.is_played(5)` becomes `trx.cutscenes[5].is_played`
   - `trx.cutscenes.set_played(5, true)` becomes
     `trx.cutscenes[5].is_played = true`

7. **The TR1 moored boat was renamed**
   The boat moored in TR1 is `moored_boat`, leaving the `boat` name to TR2's
   speedboat. A script naming the old one, and a strings file overriding its
   name, need updating:
   - `trx.catalog.objects.MOTOR_BOAT` is now `trx.catalog.objects.MOORED_BOAT`
   - `trx.objects.boat` is now `trx.objects.moored_boat`
   - the string key `objects/boat/name` is now `objects/moored_boat/name`

8. **TR1's grenade pickup was removed**
   No TR1 level places it, so the pickup and its inventory item are gone, and
   so are the TR1 catalog rows that bound them to slots 92 and 107. A script naming either one, and
   a catalog file listing either row, need updating:
   - `trx.catalog.objects.EXPLOSIVE_ITEM` and
     `trx.catalog.objects.EXPLOSIVE_OPTION` no longer exist
   - the slots are free for an object of your own to take

9. **Some objects were renamed**
   The catalog takes an object's name from its C spelling, so the spellings
   that read badly have been fixed. The old name no longer resolves: a
   script, a catalog CSV or a strings file naming one of these needs
   updating, and `trx.catalog.objects.PISTOL_ITEM` is now
   `trx.catalog.objects.PISTOLS_ITEM`.

   | Was                     | Is now                      |
   | ---                     | ---                         |
   | `O_PISTOL_ITEM`         | `O_PISTOLS_ITEM`            |
   | `O_PISTOL_OPTION`       | `O_PISTOLS_OPTION`          |
   | `O_PISTOL_AMMO_ITEM`    | `O_PISTOLS_AMMO_ITEM`       |
   | `O_PISTOL_AMMO_OPTION`  | `O_PISTOLS_AMMO_OPTION`     |
   | `O_MAGNUM_ITEM`         | `O_MAGNUMS_ITEM`            |
   | `O_MAGNUM_OPTION`       | `O_MAGNUMS_OPTION`          |
   | `O_MAGNUM_AMMO_ITEM`    | `O_MAGNUMS_AMMO_ITEM`       |
   | `O_MAGNUM_AMMO_OPTION`  | `O_MAGNUMS_AMMO_OPTION`     |
   | `O_UZI_ITEM`            | `O_UZIS_ITEM`               |
   | `O_UZI_OPTION`          | `O_UZIS_OPTION`             |
   | `O_UZI_AMMO_ITEM`       | `O_UZIS_AMMO_ITEM`          |
   | `O_UZI_AMMO_OPTION`     | `O_UZIS_AMMO_OPTION`        |
   | `O_HARPOON_ITEM`        | `O_HARPOON_GUN_ITEM`        |
   | `O_HARPOON_OPTION`      | `O_HARPOON_GUN_OPTION`      |
   | `O_HARPOON_AMMO_ITEM`   | `O_HARPOON_GUN_AMMO_ITEM`   |
   | `O_HARPOON_AMMO_OPTION` | `O_HARPOON_GUN_AMMO_OPTION` |
   | `O_LEADBAR_ITEM`        | `O_LEAD_BAR_ITEM`           |
   | `O_LEADBAR_OPTION`      | `O_LEAD_BAR_OPTION`         |
   | `O_FLAREBOX_ITEM`       | `O_FLARES_BOX_ITEM`         |
   | `O_FLAREBOX_OPTION`     | `O_FLARES_BOX_OPTION`       |
   | `O_DINO_WARRIOR`        | `O_DINO_MUTANT`             |
   | `O_FISH`                | `O_FISH_MUTANT`             |
   | `O_DETAIL_OPTION`       | `O_GRAPHICS_OPTION`         |
   | `O_CONTROL_OPTION`      | `O_CONTROLS_OPTION`         |
   | `O_GLOBE_SELECT_OPTION` | `O_GLOBE_OPTION`            |
   | `O_PIRAHNAS`            | `O_PIRANHAS`                |
   | `O_PIRAHNA_GFX`         | `O_PIRANHA_GFX`             |
   | `O_SKATEKID`            | `O_SKATE_KID`               |
   | `O_DOOR_TYPE_1`         | `O_DOOR_1`                  |
   | `O_DOOR_TYPE_2`         | `O_DOOR_2`                  |
   | `O_DOOR_TYPE_3`         | `O_DOOR_3`                  |
   | `O_DOOR_TYPE_4`         | `O_DOOR_4`                  |
   | `O_DOOR_TYPE_5`         | `O_DOOR_5`                  |
   | `O_DOOR_TYPE_6`         | `O_DOOR_6`                  |
   | `O_DOOR_TYPE_7`         | `O_DOOR_7`                  |
   | `O_DOOR_TYPE_8`         | `O_DOOR_8`                  |
   | `O_TRAPDOOR_TYPE_1`     | `O_TRAPDOOR_1`              |
   | `O_TRAPDOOR_TYPE_2`     | `O_TRAPDOOR_2`              |
   | `O_TRAPDOOR_TYPE_3`     | `O_TRAPDOOR_3`              |
   | `O_SMASH_OBJECT_1`      | `O_SMASHABLE_1`             |
   | `O_SMASH_OBJECT_2`      | `O_SMASHABLE_2`             |
   | `O_SMASH_OBJECT_3`      | `O_SMASHABLE_3`             |
   | `O_SMASH_OBJECT_4`      | `O_SMASHABLE_4`             |

10. **An object's key is its C spelling without the prefix**
   Object keys now come from the C object name without `O_`, in lower case:
   `O_SHOTGUN_ITEM` is `shotgun_item`. Before this,
   `objects/names.def` declared separate keys. Update strings files, game
   flows and weapon definitions that name an object:
   - the string key `objects/shotgun/name` is now `objects/shotgun_item/name`
   - `"object_id": "pistols"` becomes `"object_id": "pistols_item"`

   | Was                     | Is now                   |
   | ---                     | ---                      |
   | `autos`                 | `autos_item`             |
   | `autos_ammo`            | `autos_ammo_item`        |
   | `binoculars`            | `binoculars_item`        |
   | `compass`               | `compass_option`         |
   | `controls`              | `controls_option`        |
   | `crossbow`              | `crossbow_item`          |
   | `crossbow_ammo`         | `crossbow_ammo_1_item`   |
   | `crowbar`               | `crowbar_item`           |
   | `crystal`               | `save_crystal_option`    |
   | `desert_eagle`          | `desert_eagle_item`      |
   | `desert_eagle_ammo`     | `desert_eagle_ammo_item` |
   | `examine_1`             | `examine_item_1`         |
   | `examine_2`             | `examine_item_2`         |
   | `examine_3`             | `examine_item_3`         |
   | `flare`                 | `flare_item`             |
   | `flares_box`            | `flares_box_item`        |
   | `gamma`                 | `gamma_option`           |
   | `globe`                 | `globe_option`           |
   | `graphics`              | `graphics_option`        |
   | `grenade_launcher`      | `grenade_gun_item`       |
   | `grenade_launcher_ammo` | `grenade_ammo_item`      |
   | `harpoon_gun`           | `harpoon_gun_item`       |
   | `harpoon_gun_ammo`      | `harpoon_gun_ammo_item`  |
   | `key_1`                 | `key_item_1`             |
   | `key_10`                | `key_item_10`            |
   | `key_11`                | `key_item_11`            |
   | `key_12`                | `key_item_12`            |
   | `key_1_combo_1`         | `key_item_1_combo_1`     |
   | `key_1_combo_2`         | `key_item_1_combo_2`     |
   | `key_2`                 | `key_item_2`             |
   | `key_2_combo_1`         | `key_item_2_combo_1`     |
   | `key_2_combo_2`         | `key_item_2_combo_2`     |
   | `key_3`                 | `key_item_3`             |
   | `key_3_combo_1`         | `key_item_3_combo_1`     |
   | `key_3_combo_2`         | `key_item_3_combo_2`     |
   | `key_4`                 | `key_item_4`             |
   | `key_4_combo_1`         | `key_item_4_combo_1`     |
   | `key_4_combo_2`         | `key_item_4_combo_2`     |
   | `key_5`                 | `key_item_5`             |
   | `key_5_combo_1`         | `key_item_5_combo_1`     |
   | `key_5_combo_2`         | `key_item_5_combo_2`     |
   | `key_6`                 | `key_item_6`             |
   | `key_6_combo_1`         | `key_item_6_combo_1`     |
   | `key_6_combo_2`         | `key_item_6_combo_2`     |
   | `key_7`                 | `key_item_7`             |
   | `key_7_combo_1`         | `key_item_7_combo_1`     |
   | `key_7_combo_2`         | `key_item_7_combo_2`     |
   | `key_8`                 | `key_item_8`             |
   | `key_8_combo_1`         | `key_item_8_combo_1`     |
   | `key_8_combo_2`         | `key_item_8_combo_2`     |
   | `key_9`                 | `key_item_9`             |
   | `lara_grenade`          | `lara_grenade_gun`       |
   | `lara_harpoon`          | `lara_harpoon_gun`       |
   | `lara_rocket`           | `lara_rocket_gun`        |
   | `large_medipack`        | `large_medipack_item`    |
   | `lasersight`            | `lasersight_item`        |
   | `lead_bar`              | `lead_bar_item`          |
   | `m16`                   | `m16_item`               |
   | `m16_ammo`              | `m16_ammo_item`          |
   | `magnums`               | `magnums_item`           |
   | `magnums_ammo`          | `magnums_ammo_item`      |
   | `mp5`                   | `mp5_item`               |
   | `mp5_ammo`              | `mp5_ammo_item`          |
   | `passport`              | `passport_option`        |
   | `pda`                   | `pda_option`             |
   | `photo`                 | `photo_option`           |
   | `pickup_1`              | `pickup_item_1`          |
   | `pickup_1_combo_1`      | `pickup_item_1_combo_1`  |
   | `pickup_1_combo_2`      | `pickup_item_1_combo_2`  |
   | `pickup_2`              | `pickup_item_2`          |
   | `pickup_2_combo_1`      | `pickup_item_2_combo_1`  |
   | `pickup_2_combo_2`      | `pickup_item_2_combo_2`  |
   | `pickup_3`              | `pickup_item_3`          |
   | `pickup_3_combo_1`      | `pickup_item_3_combo_1`  |
   | `pickup_3_combo_2`      | `pickup_item_3_combo_2`  |
   | `pickup_4`              | `pickup_item_4`          |
   | `pickup_4_combo_1`      | `pickup_item_4_combo_1`  |
   | `pickup_4_combo_2`      | `pickup_item_4_combo_2`  |
   | `pirahnas`              | `piranhas`               |
   | `pistols`               | `pistols_item`           |
   | `pistols_ammo`          | `pistols_ammo_item`      |
   | `puzzle_1`              | `puzzle_item_1`          |
   | `puzzle_10`             | `puzzle_item_10`         |
   | `puzzle_11`             | `puzzle_item_11`         |
   | `puzzle_12`             | `puzzle_item_12`         |
   | `puzzle_1_combo_1`      | `puzzle_item_1_combo_1`  |
   | `puzzle_1_combo_2`      | `puzzle_item_1_combo_2`  |
   | `puzzle_2`              | `puzzle_item_2`          |
   | `puzzle_2_combo_1`      | `puzzle_item_2_combo_1`  |
   | `puzzle_2_combo_2`      | `puzzle_item_2_combo_2`  |
   | `puzzle_3`              | `puzzle_item_3`          |
   | `puzzle_3_combo_1`      | `puzzle_item_3_combo_1`  |
   | `puzzle_3_combo_2`      | `puzzle_item_3_combo_2`  |
   | `puzzle_4`              | `puzzle_item_4`          |
   | `puzzle_4_combo_1`      | `puzzle_item_4_combo_1`  |
   | `puzzle_4_combo_2`      | `puzzle_item_4_combo_2`  |
   | `puzzle_5`              | `puzzle_item_5`          |
   | `puzzle_5_combo_1`      | `puzzle_item_5_combo_1`  |
   | `puzzle_5_combo_2`      | `puzzle_item_5_combo_2`  |
   | `puzzle_6`              | `puzzle_item_6`          |
   | `puzzle_6_combo_1`      | `puzzle_item_6_combo_1`  |
   | `puzzle_6_combo_2`      | `puzzle_item_6_combo_2`  |
   | `puzzle_7`              | `puzzle_item_7`          |
   | `puzzle_7_combo_1`      | `puzzle_item_7_combo_1`  |
   | `puzzle_7_combo_2`      | `puzzle_item_7_combo_2`  |
   | `puzzle_8`              | `puzzle_item_8`          |
   | `puzzle_8_combo_1`      | `puzzle_item_8_combo_1`  |
   | `puzzle_8_combo_2`      | `puzzle_item_8_combo_2`  |
   | `puzzle_9`              | `puzzle_item_9`          |
   | `quest_1`               | `quest_item_1`           |
   | `quest_2`               | `quest_item_2`           |
   | `quest_3`               | `quest_item_3`           |
   | `quest_4`               | `quest_item_4`           |
   | `quest_5`               | `quest_item_5`           |
   | `quest_6`               | `quest_item_6`           |
   | `revolver`              | `revolver_item`          |
   | `revolver_ammo`         | `revolver_ammo_item`     |
   | `rocket_launcher`       | `rocket_gun_item`        |
   | `rocket_launcher_ammo`  | `rocket_ammo_item`       |
   | `save_crystal`          | `save_crystal_item`      |
   | `scion`                 | `scion_item_1`           |
   | `shotgun`               | `shotgun_item`           |
   | `shotgun_ammo`          | `shotgun_ammo_item`      |
   | `small_medipack`        | `small_medipack_item`    |
   | `snake`                 | `cobra`                  |
   | `sound`                 | `sound_option`           |
   | `stopwatch`             | `stopwatch_option`       |
   | `uzis`                  | `uzis_item`              |
   | `uzis_ammo`             | `uzis_ammo_item`         |
   | `waterskin_1`           | `waterskin_1_empty`      |
   | `waterskin_2`           | `waterskin_2_empty`      |

11. **An object's names are one string**
   A strings file now gives an object one `name` string. Put `|` between the
   name the game shows and the names the console accepts:

   ```json5
   "objects": {
       "large_medipack_item": {
           "name": "Large Medipack|Big Medipack",
       },
   }
   ```

   The names live as the game string `objects/<key>/name`, so
   `trx.locale.get("objects/large_medipack_item/name")` reads them. An object
   can also name another object with a reference such as
   `$objects/key_item_1`. It then reads that object's fields, including
   `name` and `description`, and follows level-specific names.

   `object.names` still returns a list. Inventory options reference the items
   they show, so `object.default_names` is empty for those options.

12. **Weapon, outfit and mesh names drop their C prefix**
   The names for weapons, weapon types, extra meshes, braid modes and extra
   outfit states now come from the C constant without its prefix, in lower
   case, the same way an object key does. Before this, a builder wrote
   `LGT_SHOTGUN` in one file and `shotgun_item` in the next. Update
   `weapons.json5` and `outfits.json5`:

   | Field                            | Was                    | Is now         |
   | ---                              | ---                    | ---            |
   | a weapon, and a `gun_maps` entry | `LGT_SHOTGUN`          | `shotgun`      |
   | `type`                           | `WEAPON_TYPE_RIFLE`    | `rifle`        |
   | `extra_meshes`                   | `EXTRA_MESH_OAR`       | `oar`          |
   | a braid `mode`                   | `BRAID_MODE_TR1_FULL`  | `tr1_full`     |
   | `extra_outfits`                  | `LS_EXTRA_TREX_KILL`   | `trex_kill`    |

   `extra_mesh_positions` takes the same names as `extra_meshes`.

   A name that is not recognized is reported where the file is read, except in
   `gun_maps`, where an entry the engine does not know is skipped.

13. **`-` and `_` name the same value**
   Anywhere a name is read, `-` and `_` are the same separator, so
   `software-renderer` and `software_renderer` name one value. A settings file
   needs no updating, and still keeps `-` until 1.15.

   A script does. A setting's value now reaches Lua with `_`, so a comparison
   against the old spelling no longer matches and reports nothing:
   - `trx.config.get("ui.enemy_healthbar_show_mode") == "boss-only"` becomes
     `== "boss_only"`

   The values a setting lists, and its default, read back the same way.

### Version 1.9 to 1.10

The Lua API was rewritten, and most of what it breaks is a rename. Run your
scripts with `trx.api.strict()` turned on and fix what it reports, then read
"Changed behavior" below - those are the changes that leave a script running
and doing something else.

#### Game flow and level data

1. **Name Lua scripts after what they belong to**
   A game flow no longer declares its scripts; both the global `main_script`
   key and each level's `script` key were removed. A level loading `wall.tr2`
   runs `scripts/wall.lua` in its own game's directory, so rename any script
   whose name does not already match its level. What `main_script` pointed at
   goes in `scripts/_game.lua`, which the game runs as it starts.

   A game that extends another looks in its own directory alone, and brings
   its own copy of any level script it wants. One that ships no `_game.lua`
   runs its base game's.

2. **Update Lara's outfit definitions**
   The `braid` entries became arrays, to support up to two, and a
   `joints_object` can be given for TR4/5 outfits. Update `outfits.json5`
   against the shipped file and the [outfits documentation](OUTFITS.md).

3. **Update weapon definitions**
   The ammunition keys in `weapons.json5` were renamed to say what they count.
   A shot is one pull of the trigger, which for the shotgun spends six rounds;
   the flare counts a flare where a weapon counts a shot. The old names are
   still read, so a file that keeps them goes on working.
   - `initial_qty` is now `initial_shots`
   - `pickup_qty` is now `box_shots`
   - `inventory_qty` is now `box_label_qty`

   `ammo.pickup_qty_alt` is ignored. It only applied to flares in Japanese NG,
   which is no longer a game mode, and a flare box now always gives
   `ammo.pickup_qty` flares.

4. **Update game flows that anchor Bacon Lara**
   The `setup_bacon_lara` sequence event was removed. The anchor room is an
   `anchor_room` object property now, which a level editor can set on the
   object or on a single item, and a script can set in `on_game_start`:
   ```lua
   trx.objects.bacon_lara.properties.anchor_room = 10
   ```
   At its default of -1, the room Bacon Lara is placed in is the anchor. Refer
   to the Atlantis level in the default game flow.

5. **Update TR3 artefact pickups and plinth scions**
   The glow color and rotation speed of TR3 artefacts are Lua properties now
   rather than hardcoded, and an `O_SCION_ITEM_1` pickup needs a pickup mode
   of `PLINTH_SCION` to invoke Lara's extra animation. Refer to the OG Lua
   scripts.

#### Renamed

Mechanical, one for one:

| 1.9                            | 1.10                      |
| ------------------------------ | ------------------------- |
| `trx.items.fn.get()`           | `trx.items.get()`         |
| `trx.rooms.fn.get()`           | `trx.rooms.get()`         |
| `trx.rooms.fn.Room`            | `trx.rooms.Room`          |
| `trx.rooms.fn.FlipStatus`      | `trx.rooms.FlipStatus`    |
| `trx.rooms.fn.flip()`          | `trx.rooms.flip()`        |
| `trx.rooms.fn.flip_effect()`   | `trx.rooms.flip_effect()` |
| `room.idx`                     | `room.num`                |
| `item.index`                   | `item.num`                |
| `item.anim`                    | `item.anim_num`           |
| `item.frame`                   | `item.frame_num`          |
| `level.name`                   | `level.title`             |
| `trx.lara.mesh.hand_r`         | `trx.lara.Mesh.HAND_R`    |
| `trx.lara.extra_mesh.oar`      | `trx.lara.ExtraMesh.OAR`  |
| `trx.pickup.Mode.X`            | `trx.items.PickupMode.X`  |
| `trx.console.log.LogLevel`     | `trx.log.LogLevel`        |
| `trx.music.get_track()`        | `trx.music.current_track` |
| `trx.music.get_looped_track()` | `trx.music.looped_track`  |
| `trx.music.available_tracks()` | `trx.music.tracks`        |

The `fn` namespaces are gone: index the module directly, `trx.items[16]`,
`trx.items["lara"]`, `trx.rooms[14]`. The mesh tables became declared enums,
so every name in them is upper case, not the two shown above alone.

Handler arguments now say what kind of number they carry: `on_room_change`
takes `old_room_num` and `new_room_num`, `on_flyby_end` takes `sequence_num`,
the `on_cutscene_*` handlers take `cutscene_num`, `trx.savegame`'s slot
argument is `slot_num` and `trx.inventory`'s object argument is `object_id`.
They are positional, so this only matters to a script's own documentation.

#### Removed

| 1.9                                     | Use instead                              |
| --------------------------------------- | ---------------------------------------- |
| `item.idx`                              | the handle itself                        |
| `item.flags`                            | `trigger_mask`, `is_reversed`, `is_triggered`, `is_killed`, `is_one_shot` |
| `item.status`, `items.Status`           | the boolean fields - see below            |
| `trx.items.find()`, `trx.items.first()` | `trx.items.query`, `:of_object()` and `:in_room()`, then `:matches()` or `:first()` |
| `trx.game.settings.play_any_level = true` | `trx.config.override("flow.play_any_level", true)` |
| `trx.pickup`                            | `trx.items.PickupMode`                   |
| `trx.events.EventType`, hook `._type`   | nothing; the nine hooks are the whole API |
| `trx.music.play_track()`                | `trx.music.play()`                       |
| `trx.music.is_available(id)`            | `trx.music.tracks[id] ~= nil`            |
| `trx.sound.is_available(id)`            | `trx.sound.samples[id] ~= nil`           |
| `before_level_file`, `after_level_file`, `before_item_setup`, `after_item_setup`, `after_level_state` | `on_game_start` - see below |

#### Changed behavior

The first four change what a script does without raising. The rest report
themselves.

1. **Items and rooms count from 0**
   The numbering matches what level editors show: `trx.items[13]` is
   `trx.items[12]` now, and `trx.rooms[15]` is `trx.rooms[14]`.
   `item.room_num`, `camera.room_num`, `camera.target_room_num` and
   `find_valid_pos`'s room argument follow, and
   `for i = 1, #trx.items do local item = trx.items[i]` becomes
   `for num, item in pairs(trx.items) do`. `on_pickup` always counted from 0,
   so drop the `item_num + 1` that bridged the gap.

2. **`trx.config.get()` returns the option's own type**
   `trx.config.get("flow.cheat_keys") == "true"` is false whatever the setting
   holds; test the value itself. Colors and enums are still strings. `set()`
   still writes to the player's settings and keeps the change - use the new
   `override()` and `restore()` for what a level wants only while it runs.

3. **`trx.lara.extra_anim` is a boolean**
   It says whether a scripted animation is driving Lara, where it used to be
   the relative animation number of `O_LARA_EXTRA`, or `-1`. `~= -1` is now
   always true. The number itself is `trx.lara.item.anim_num`.

4. **`max_hit_points` carries `hit_points` with it**
   Writing it moves the item's current hit points by the same difference, so
   the companion write is no longer needed.

5. **Handles are opaque, compare by identity, and go stale**
   An item or room handle is no longer a `{ idx = ... }` table and cannot
   carry keys of your own; pass the handle where you passed the index.
   `trx.items[0] == trx.items[0]` is true now, where every lookup used to hand
   back a fresh table. A handle to a killed item, or any room handle after a
   level change, raises `stale ITEM handle` rather than addressing whatever
   took the slot - guard one held across time with `:is_valid()`.

6. **`item.status` became separate boolean fields**

   | `status` in 1.9             | 1.10                                     |
   | --------------------------- | ---------------------------------------- |
   | `ACTIVE`, running           | `is_simulated`, started by `activate()`   |
   | `ACTIVE`, targetable enemy  | `is_in_play`; `is_targetable` for auto-aim |
   | `INVISIBLE`                 | `not is_visible`                          |
   | `DEACTIVATED`               | `is_finished`                             |

   `is_present` is new: in the world at all, linked in its room. The item
   query narrows on each - `simulated`, `present`, `visible`, `finished`,
   `in_play`, `alive`, `targetable`.

7. **`on_game_start` replaces the level lifecycle events**
   It is the one moment a level script gets before play: the level file is
   loaded, its items are set up, savegame state has been applied, and nothing
   has been drawn. A handler moves across as it stands, and an object property
   no longer has to be written before its item is initialised, which is what
   the earlier moments were for.
   ```lua
   trx.events.on_game_start(function(is_save)
     trx.items[65].properties.range = { x = 14, y = 6, z = 14 }
   end)
   ```
   It fires for cutscene and demo levels too, and the title screen has
   `on_title_start`. The level is `trx.game.current_level` rather than a
   number handed to the handler.

8. **`trx.game.levels` leaves out the gym**
   Where a game flow has one, every entry has shifted down by one and the last
   level - previously unreachable - is in the list. Drop any offset that
   stepped over the gym; it is `trx.game.gym` and `trx.game.play_gym()`. The
   same holds for `trx.game.cutscenes`, `trx.game.demos` and their `play_`
   functions. Every field on a level is read-only.

9. **Music and sound take a catalog id**
   `trx.music.play`, `trx.sound.play` and `trx.sound.stop` take a
   `trx.catalog.music` or `trx.catalog.samples` value, which maps to the right
   track or sample per game, rather than the level's own slot. Reach a slot
   through its handle - `trx.music.tracks[slot]:play()`,
   `trx.sound.samples[slot]:play()` - and both `play` functions hand back the
   stream they started. `trx.sound.stop_all` is unchanged.

10. **These raise where they used to pass**

    | Call                              | Why                                |
    | --------------------------------- | ---------------------------------- |
    | `item.hit_points = 99999`         | truncated to the field's width     |
    | `room.wind = 1`, `room.cold = nil` | room flags take booleans only     |
    | writing an out-of-range room      | did nothing                        |
    | `trx.rooms["5"]`                  | index with a number                |
    | `trx.console.log("a", "b")`       | format the message yourself        |
    | `item.object_id = ...`            | spawn the type you want instead    |
    | writing to an enum or catalog     | broke every later lookup           |
    | `{ x = , y = }`                   | a position needs all three         |

11. **These read differently**
    - An unset room flag is `false`, not `nil`, so
      `if room.underwater == nil` no longer detects a dry room
    - `trx.objects[id]` is `nil` for an id the game does not have, where it
      used to hand back an object that answered to nothing
    - An enum answers to a constant's name in any case, and `pairs()` over one
      yields the constants alone
    - `trx.events.detach` takes the `Listener` an attach handed back, not a
      number; `listener.id` is the number

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
   [game strings documentation](GAME_STRINGS.md) for more details.



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
