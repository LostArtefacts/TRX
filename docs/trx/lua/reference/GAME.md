---
title: Game
order: 10
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/game.lua. Edit it there.
-->

## <a id="game" name="game"></a>Game module

Module for the game flow: which levels there are, and which one is being played.

### Properties

- <a id="game.levels" name="game.levels"></a>**`trx.game.levels`** (a list of [trx.game.Level](#game.Level)). The levels of the game, in order, counted from one. *(read-only)*
- <a id="game.cutscenes" name="game.cutscenes"></a>**`trx.game.cutscenes`** (a list of [trx.game.Level](#game.Level)). The cutscene levels, counted from one. TR4's in-game cutscenes are a different thing, and live in [`trx.cutscenes`](CUTSCENES.md#cutscenes). *(read-only)*
- <a id="game.demos" name="game.demos"></a>**`trx.game.demos`** (a list of [trx.game.Level](#game.Level)). The demos, counted from one. *(read-only)*
- <a id="game.current_level" name="game.current_level"></a>**`trx.game.current_level`** ([trx.game.Level](#game.Level)). The level being played, or `nil` if none is. *(read-only)*
- <a id="game.gym" name="game.gym"></a>**`trx.game.gym`** ([trx.game.Level](#game.Level)). The gym level, or `nil` if this game has no gym. *(read-only)*
- <a id="game.version" name="game.version"></a>**`trx.game.version`** (integer). Which Tomb Raider this build is: 1, 2, 3 or 4. *(read-only)*
- <a id="game.trx_version" name="game.trx_version"></a>**`trx.game.trx_version`** (string). What this build reports as its version: `1.9.3` for a release, and the tag with the commits since it for a development build. *(read-only)*
- <a id="game.is_loaded" name="game.is_loaded"></a>**`trx.game.is_loaded`** (boolean). Whether a level is loaded. *(read-only)*
- <a id="game.is_playable" name="game.is_playable"></a>**`trx.game.is_playable`** (boolean). Whether the game is loaded and taking input - not in a menu, and not in a cutscene. *(read-only)*
- <a id="game.is_ngplus" name="game.is_ngplus"></a>**`trx.game.is_ngplus`** (boolean). Whether this is a new game plus run, which is what the passport's bonus start sets. Lara keeps her weapons between levels and her ammunition does not run down. *(read-only)*

### Constants

- <a id="game.LOGIC_FPS" name="game.LOGIC_FPS"></a>[lua]`trx.game.LOGIC_FPS` = `30` (integer)  
  How many logical frames the game runs a second, which is the rate [`trx.events.before_control`](EVENTS.md#events.before_control) fires at. A script that counts frames divides by this to reach seconds.

### Enums

- <a id="game.LevelTable" name="game.LevelTable"></a>[lua]`trx.game.LevelTable`

    One of the lists of levels the game flow declares.

    - `trx.game.LevelTable.TITLE` = `0`  
        The title screen.
    - `trx.game.LevelTable.MAIN` = `1`  
        The levels of the game proper.
    - `trx.game.LevelTable.CUTSCENES` = `2`  
        The cutscenes.
    - `trx.game.LevelTable.DEMOS` = `3`  
        The demos that play when the title screen is left alone.

- <a id="game.LevelType" name="game.LevelType"></a>[lua]`trx.game.LevelType`

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

- <a id="game.LevelNum" name="game.LevelNum"></a>[lua]`trx.game.LevelNum`

    The number a level goes by, which is what the player is shown and what a gameflow names. Not its place in a table: a level the game flow skips does not count, and a gym level has no number at all and reads 0. Counted from 1.

- <a id="game.DemoNum" name="game.DemoNum"></a>[lua]`trx.game.DemoNum`

    Where a demo sits in the table of demos. Counted from 1.

- <a id="game.Frames" name="game.Frames"></a>[lua]`trx.game.Frames` (integer)

    A length of time counted in the frames the engine runs the world at, which
    is what the engine measures its own timers in.

- <a id="game.Seconds" name="game.Seconds"></a>[lua]`trx.game.Seconds` (number)

    A length of time in seconds, as a player would read it off a clock.

- <a id="game.Level" name="game.Level"></a>[lua]`trx.game.Level`

    A level, as the game flow file declares it. Everything on it is read-only: a level is what the game flow says it is.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="game.Level.key" name="game.Level.key"></a>**`key`**: string. What the level is called, taken from the name of the file it loads: `wall.tr2` reads
      back as `wall`. Lower case, regardless of the case on disk, and `nil` for a level that loads no file
      of its own.

      This is the name to write into a table of per-level data. [`num`](#game.Level.num) is a position and
      moves as soon as a game flow gains a level, and [`path`](#game.Level.path) is wherever the file sits on
      this install. *(read-only)*
    - <a id="game.Level.lara_outfit" name="game.Level.lara_outfit"></a>**`lara_outfit`**: string. The outfit Lara starts the level in. *(read-only)*
    - <a id="game.Level.music_track" name="game.Level.music_track"></a>**`music_track`**: [trx.catalog.music](CATALOG.md#catalog.music). The track that plays when the level starts. *(read-only)*
    - <a id="game.Level.num" name="game.Level.num"></a>**`num`**: [trx.game.LevelNum](#game.LevelNum). *(read-only)*
    - <a id="game.Level.path" name="game.Level.path"></a>**`path`**: string. Path to the level file. *(read-only)*
    - <a id="game.Level.script_path" name="game.Level.script_path"></a>**`script_path`**: string. Path to the Lua script that runs when the level loads, or `nil` if it has none. *(read-only)*
    - <a id="game.Level.title" name="game.Level.title"></a>**`title`**: string. The level's name, as shown to the player. *(read-only)*
    - <a id="game.Level.type" name="game.Level.type"></a>**`type`**: [trx.game.LevelType](#game.LevelType). What kind of level it is. *(read-only)*
    - <a id="game.Level.unobtainable_ally_kills" name="game.Level.unobtainable_ally_kills"></a>**`unobtainable_ally_kills`**: integer. Ally kills the stats screen must not hold against the player. *(read-only)*
    - <a id="game.Level.unobtainable_kills" name="game.Level.unobtainable_kills"></a>**`unobtainable_kills`**: integer. Kills the stats screen must not hold against the player. *(read-only)*
    - <a id="game.Level.unobtainable_pickups" name="game.Level.unobtainable_pickups"></a>**`unobtainable_pickups`**: integer. Pickups the stats screen must not hold against the player, because they cannot be got. *(read-only)*
    - <a id="game.Level.unobtainable_secrets" name="game.Level.unobtainable_secrets"></a>**`unobtainable_secrets`**: integer. Secrets the stats screen must not hold against the player. *(read-only)*
    - <a id="game.Level.water_particles" name="game.Level.water_particles"></a>**`water_particles`**: boolean. Whether water particles are visible in the level's water. *(read-only)*

    Computed properties (derived, not stored on the object):
    - <a id="game.Level.inventory" name="game.Level.inventory"></a>**`inventory`**: [trx.inventory.Inventory](INVENTORY.md#inventory.Inventory). What the level keeps for Lara's return, or `nil` for a level that keeps nothing: the title screen and the cutscenes. It is what she will arrive there with rather than what she is carrying now, which is [`trx.inventory`](INVENTORY.md#inventory) itself.
    - <a id="game.Level.stats" name="game.Level.stats"></a>**`stats`**: [trx.stats.Stats](STATS.md#stats.Stats). What the level keeps count of, or `nil` for a level that counts nothing: the title screen and the cutscenes. The level being played is also [`trx.stats`](STATS.md#stats) itself.

### Functions

- <a id="game.play_level" name="game.play_level"></a>[lua]`trx.game.play_level(level_num, [opts])`  
  Starts a level from [`trx.game.levels`](#game.levels).

  Parameters:
  - <a id="game.play_level.level_num" name="game.play_level.level_num"></a>**`level_num`** ([trx.game.LevelNum](#game.LevelNum)).
  - <a id="game.play_level.opts" name="game.play_level.opts"></a>**`opts`** (table, optional). How to start it.

    Keys:
    - <a id="game.play_level.opts.select" name="game.play_level.opts.select"></a>**`select`** (boolean, optional). Start the level as the level-select screen does, rebuilding Lara's inventory to what she would carry on reaching it. Without it the level continues from the one in progress.

  Example:
  ```lua
  trx.game.play_level(1)
  ```

- <a id="game.play_cutscene" name="game.play_cutscene"></a>[lua]`trx.game.play_cutscene(cutscene_num)`  
  Plays a cutscene.

  Parameters:
  - <a id="game.play_cutscene.cutscene_num" name="game.play_cutscene.cutscene_num"></a>**`cutscene_num`** ([trx.cutscenes.Num](CUTSCENES.md#cutscenes.Num)).

- <a id="game.play_demo" name="game.play_demo"></a>[lua]`trx.game.play_demo([demo_num])`  
  Plays a demo, and returns the one that started.

  Parameters:
  - <a id="game.play_demo.demo_num" name="game.play_demo.demo_num"></a>**`demo_num`** ([trx.game.DemoNum](#game.DemoNum), optional). Omit to play the next demo in rotation.

  Returns:
  - [trx.game.Level](#game.Level) or `nil`. The demo that started, or `nil` if the game has no demos.

- <a id="game.play_gym" name="game.play_gym"></a>[lua]`trx.game.play_gym()`  
  Starts the gym. Raises if this game has no gym.

- <a id="game.end_level" name="game.end_level"></a>[lua]`trx.game.end_level()`  
  Ends the current level, as though Lara had reached its exit.

- <a id="game.exit_to_title" name="game.exit_to_title"></a>[lua]`trx.game.exit_to_title()`  
  Leaves the current game and returns to the title screen.

- <a id="game.exit_game" name="game.exit_game"></a>[lua]`trx.game.exit_game()`  
  Closes the game.

- <a id="game.screenshot" name="game.screenshot"></a>[lua]`trx.game.screenshot([path])`  
  Takes a screenshot. Without a path, writes one to the screenshots folder in the player's configured format; with a path, writes to that file.

  Parameters:
  - <a id="game.screenshot.path" name="game.screenshot.path"></a>**`path`** (string, optional). File to write to.
