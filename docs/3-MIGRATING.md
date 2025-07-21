---
title: Migrating levels
---

# Migration guide for level builders

## TR1X

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
