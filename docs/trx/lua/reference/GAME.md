---
title: Game
order: 11
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  data/scripting/game.lua. Edit it there.
-->

## Game module

Module for the game flow: which levels there are, and which one is being played.

### Properties

- **`trx.game.levels`** (table). The levels of the game proper, in order, as a list of `trx.game.Level`. *(read-only)*
- **`trx.game.cutscenes`** (table). The cutscenes, as a list of `trx.game.Level`. *(read-only)*
- **`trx.game.demos`** (table). The demos, as a list of `trx.game.Level`. *(read-only)*
- **`trx.game.current_level`** (Level). The level being played, or `nil` if none is. *(read-only)*
- **`trx.game.version`** (integer). Which Tomb Raider this build is: 1, 2, 3 or 4. *(read-only)*
- **`trx.game.trx_version`** (string). The TRX version string. *(read-only)*

### Enums

- [lua]`trx.game.LevelTable`

    One of the lists of levels the game flow declares.

    - `trx.game.LevelTable.TITLE` = `0`  
        The title screen.
    - `trx.game.LevelTable.MAIN` = `1`  
        The levels of the game proper.
    - `trx.game.LevelTable.CUTSCENES` = `2`  
        The cutscenes.
    - `trx.game.LevelTable.DEMOS` = `3`  
        The demos that play when the title screen is left alone.

- [lua]`trx.game.LevelType`

    What kind of level it is.

    - `trx.game.LevelType.TITLE` = `0`  
        The title screen.
    - `trx.game.LevelType.NORMAL` = `1`  
        An ordinary level.
    - `trx.game.LevelType.CUTSCENE` = `2`  
        A cutscene.
    - `trx.game.LevelType.DEMO` = `3`  
        A demo.
    - `trx.game.LevelType.GYM` = `4`  
        Lara's home, which has no level number.
    - `trx.game.LevelType.BONUS` = `5`  
        A bonus level, played once the game is finished.
    - `trx.game.LevelType.DUMMY` = `6`  
        Not a level. Kept only because old savegames refer to it.
    - `trx.game.LevelType.CURRENT` = `7`  
        Not a level. Kept only because old savegames refer to it.

### Structures

- [lua]`trx.game.Level`

    A level, as the game flow file declares it. Everything on it is read-only: a level is what the game flow says it is.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`lara_outfit`**: string. The outfit Lara starts the level in. *(read-only)*
    - **`music_track`**: integer. The track that plays when the level starts. Compare against `trx.catalog.music`. *(read-only)*
    - **`num`**: integer. The number the level goes by. Not its place in the table: levels the game flow skips do not count, and a gym level has no number at all and reads 0. *(read-only)*
    - **`path`**: string. Path to the level file. *(read-only)*
    - **`script_path`**: string. Path to the Lua script that runs when the level loads, or `nil` if it has none. *(read-only)*
    - **`title`**: string. The level's name, as shown to the player. *(read-only)*
    - **`type`**: integer. What kind of level it is. Compare against `trx.game.LevelType`. *(read-only)*
    - **`unobtainable_ally_kills`**: integer. Ally kills the stats screen must not hold against the player. *(read-only)*
    - **`unobtainable_kills`**: integer. Kills the stats screen must not hold against the player. *(read-only)*
    - **`unobtainable_pickups`**: integer. Pickups the stats screen must not hold against the player, because they cannot be got. *(read-only)*
    - **`unobtainable_secrets`**: integer. Secrets the stats screen must not hold against the player. *(read-only)*
    - **`water_particles`**: boolean. Whether bubbles rise through the level's water. *(read-only)*

### Functions

- [lua]`trx.game.play_level(num)`  
  Starts a level of the game proper.

  Parameters:
  - **`num`** (integer). The level's number.

  Example:
  ```lua
  trx.game.play_level(1)
  ```

- [lua]`trx.game.play_cutscene(num)`  
  Plays a cutscene.

  Parameters:
  - **`num`** (integer). The cutscene's number.

- [lua]`trx.game.play_demo(num)`  
  Plays a demo.

  Parameters:
  - **`num`** (integer). The demo's number.
