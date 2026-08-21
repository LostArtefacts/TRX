---
title: Config
order: 25
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/config.lua. Edit it there.
-->

## <a id="config" name="config"></a>Config module

Module for reading, changing and declaring engine settings.

These are the player's settings, not the level's. [`trx.config.set`](#config.set) writes to them and keeps the
change: it is remembered across saves and relaunches, exactly as if the player had made it
themselves. A level that wants to tint the water or pull the fog in wants [`trx.config.override`](#config.override)
instead, which lasts as long as the script keeps it and leaves the player's own value untouched
underneath.

A game can also add settings of its own with [`trx.config.declare`](#config.declare): they are saved and loaded with
the player's own, and shown in the settings menu where the declaration asks for. A setting a
game's `scripts/_game.lua` declares belongs to that game and goes when it does.

### Structures

- <a id="config.Shape" name="config.Shape"></a>[lua]`trx.config.Shape`

    Everything about a setting but the value it holds now.

    [`trx.config.describe`](#config.describe) hands one back and [`trx.config.declare`](#config.declare) takes one, so what a script reads
    of a setting is what a script writes to make one. The row a declaration asks for is the part
    [`trx.config.describe`](#config.describe) does not report: see [`trx.config.Row`](#config.Row).

    Properties:
    - <a id="config.Shape.default" name="config.Shape.default"></a>**`default`**: any. What the setting holds until the player changes it, and what [`trx.config.reset`](#config.reset) puts back.
    - <a id="config.Shape.key" name="config.Shape.key"></a>**`key`**: string. Dotted path the setting answers to. The last segment is what the settings file keys it on, so it has to differ from every other setting's.
    - <a id="config.Shape.kind" name="config.Shape.kind"></a>**`kind`**: string. One of `boolean`, `integer`, `number`, `color`, `enum`, `dynamic_enum` or `string`. A declaration writes `boolean`, `integer` or `dynamic_enum`; the rest name storage the engine owns.
    - <a id="config.Shape.max" name="config.Shape.max"></a>**`max`**: integer, optional. Highest value a number takes. Absent, it takes as high a number as it can hold.
    - <a id="config.Shape.min" name="config.Shape.min"></a>**`min`**: integer, optional. Lowest value a number takes. Absent, it takes as low a number as it can hold.
    - <a id="config.Shape.percent" name="config.Shape.percent"></a>**`percent`**: boolean, optional. Marks a number stored 0-1 but entered and shown as a 0-100 percentage.
    - <a id="config.Shape.ui" name="config.Shape.ui"></a>**`ui`**: [trx.config.Row](#config.Row), optional. The row a declared setting takes in the settings menu. Without one the setting has no row, and is the script's to read and write.
    - <a id="config.Shape.values" name="config.Shape.values"></a>**`values`**: a list of string, optional. What the setting accepts, for the enum kinds. A declared `dynamic_enum` needs them, and they have to list the default.

- <a id="config.Row" name="config.Row"></a>[lua]`trx.config.Row`

    The settings row a declared setting is shown on: where it sits, and what it
    does that the setting itself cannot say.

    Every callback below is optional, and one that raises is logged and answered as though it were
    absent. They are read as the setting is declared and are not reported back by
    [`trx.config.describe`](#config.describe).

    Properties:
    - <a id="config.Row.after" name="config.Row.after"></a>**`after`**: string, optional. Setting the row sits below. The row lands at the end of the tab where neither anchor is given.
    - <a id="config.Row.before" name="config.Row.before"></a>**`before`**: string, optional. Setting the row sits above. Steadier than a position: the row stays put when the tab is reordered.
    - <a id="config.Row.can_change_value" name="config.Row.can_change_value"></a>**`can_change_value`**: function, optional. Called with the value and the direction, `-1` or `1`. Return false to refuse that press.
    - <a id="config.Row.delta_fast" name="config.Row.delta_fast"></a>**`delta_fast`**: integer, optional. How far one press moves a number. One step where it is absent.
    - <a id="config.Row.delta_slow" name="config.Row.delta_slow"></a>**`delta_slow`**: integer, optional. How far one press moves a number while fine adjustment is held.
    - <a id="config.Row.format_value" name="config.Row.format_value"></a>**`format_value`**: function, optional. Called with the value, returning what the row prints in place of it.
    - <a id="config.Row.is_available" name="config.Row.is_available"></a>**`is_available`**: function, optional. Called with the value. Return false to grey the row out: it stays visible, and the player cannot move it.
    - <a id="config.Row.is_visible" name="config.Row.is_visible"></a>**`is_visible`**: function, optional. Called with the value. Return false to leave the row out of the tab.
    - <a id="config.Row.request_change_value" name="config.Row.request_change_value"></a>**`request_change_value`**: function, optional. Called with the value and the direction. Return true to take the press over; the row is left alone, and moving the setting is the script's to do.
    - <a id="config.Row.tab" name="config.Row.tab"></a>**`tab`**: string. Settings tab the row sits on: `gameplay_general`, `gameplay_controls`, `gameplay_mods`, `gameplay_fixes`, `graphic_visuals`, `graphic_ui`, `graphic_ui_stats`, `graphic_ui_bars`, `graphic_rendering`, `sound_volume` or `sound_misc`.

- <a id="config.Watcher" name="config.Watcher"></a>[lua]`trx.config.Watcher`

    A setting being watched. [`trx.config.on_change`](#config.on_change) hands one back, and holding it
    is what lets the watcher be dropped later. A watcher is spent once detached, and the end of a level
    spends every one a level script attached.

    Methods:

    - <a id="config.Watcher.detach" name="config.Watcher.detach"></a>[lua]`watcher:detach()`  
      Stops the watcher, which hears of no further change.

      Returns: boolean. Whether it was still watching.

### Functions

- <a id="config.get" name="config.get"></a>[lua]`trx.config.get(key)`  
  Reads a setting. The value comes back as the type the option is declared with, so a boolean option reads as a boolean and a color as a [`trx.math.Color`](MATH.md#math.Color). Enums read as strings.

  Parameters:
  - <a id="config.get.key" name="config.get.key"></a>**`key`** (string). Dotted path, e.g. `visuals.water_color`.

  Returns: any. Raises if no option has that key.

  Example:
  ```lua
  if trx.config.get("audio.enable_music") then
    trx.music.play(trx.catalog.music.SECRET)
  end
  ```

- <a id="config.describe" name="config.describe"></a>[lua]`trx.config.describe(key)`  
  Everything about a setting but the value it holds now: what it is, what it accepts, and what it falls back to. This is the shape [`trx.config.declare`](#config.declare) takes, so a script can read one setting and declare another like it.

  Parameters:
  - <a id="config.describe.key" name="config.describe.key"></a>**`key`** (string). Dotted path.

  Returns: [trx.config.Shape](#config.Shape). What the setting is and how it is entered.

  Example:
  ```lua
  for _, value in ipairs(trx.config.describe("visuals.shadow_type").values) do
    trx.log.info(value)
  end
  ```

- <a id="config.format_value" name="config.format_value"></a>[lua]`trx.config.format_value(key)`  
  The current value as the console prints it: `1` or `0` for a boolean, two decimals for a plain number, a 0-100 percentage where the option is one, and enum values with dashes for underscores.

  Parameters:
  - <a id="config.format_value.key" name="config.format_value.key"></a>**`key`** (string). Dotted path.

  Returns: string. The text, ready to print.

  Example:
  ```lua
  trx.console.log(trx.config.format_value("visuals.fov"))
  ```

- <a id="config.accepted_values" name="config.accepted_values"></a>[lua]`trx.config.accepted_values(key)`  
  What a setting accepts, as text for an error message: `on, off` for a boolean, a marker like `[integer]` for the number kinds, or the value names for the enum kinds, with dashes for underscores.

  Parameters:
  - <a id="config.accepted_values.key" name="config.accepted_values.key"></a>**`key`** (string). Dotted path.

  Returns: string or `nil`. `nil` for the kinds with nothing to list, such as a color.

- <a id="config.set" name="config.set"></a>[lua]`trx.config.set(key, value, [force])`  
  Changes the player's setting, and keeps the change. Raises if the key is unknown or the value will not parse.

  The old value is not kept anywhere: the new one becomes the active setting as if the player had chosen it, and is remembered across saves and relaunches. Prefer [`trx.config.override`](#config.override) for anything a level wants only while it is running.

  Parameters:
  - <a id="config.set.key" name="config.set.key"></a>**`key`** (string). Dotted path.
  - <a id="config.set.value" name="config.set.value"></a>**`value`** (any). A boolean, a number, or a string, matching the option's type. A color is a [`trx.math.Color`](MATH.md#math.Color) or the hex text one is written as. An enum value is taken in either spelling: underscores or the dashes the console shows.
  - <a id="config.set.force" name="config.set.force"></a>**`force`** (boolean, optional). Write through a setting the game flow enforces.

- <a id="config.reset" name="config.reset"></a>[lua]`trx.config.reset(key, [force])`  
  Puts a setting back to its default, and keeps the change, as [`trx.config.set`](#config.set) does.

  Parameters:
  - <a id="config.reset.key" name="config.reset.key"></a>**`key`** (string). Dotted path.
  - <a id="config.reset.force" name="config.reset.force"></a>**`force`** (boolean, optional). As for [`trx.config.set`](#config.set).

  Returns: boolean. `false` when a script or the game flow is holding the setting (see [`trx.config.is_overridden`](#config.is_overridden)).

- <a id="config.override" name="config.override"></a>[lua]`trx.config.override(key, value)`  
  Changes a setting for as long as the script keeps the override, without touching the player's own value.

  The player's value sits underneath and comes back on [`trx.config.restore`](#config.restore). Nothing is written to disk. Overrides stack, so one can be pushed over another; each [`trx.config.restore`](#config.restore) lifts one off. A setting the game flow enforces cannot be overridden.

  Parameters:
  - <a id="config.override.key" name="config.override.key"></a>**`key`** (string). Dotted path.
  - <a id="config.override.value" name="config.override.value"></a>**`value`** (any). As for [`trx.config.set`](#config.set).

  Example:
  ```lua
  trx.config.override("visuals.water_color", "0080ff")
  -- ... and when the level is done with it:
  trx.config.restore("visuals.water_color")
  ```

- <a id="config.restore" name="config.restore"></a>[lua]`trx.config.restore(key)`  
  Lifts one override off a setting, putting back the value underneath it.

  Parameters:
  - <a id="config.restore.key" name="config.restore.key"></a>**`key`** (string). Dotted path.

  Returns: boolean. `false` if the setting was not overridden.

- <a id="config.is_overridden" name="config.is_overridden"></a>[lua]`trx.config.is_overridden(key)`  
  Whether a script or the game flow is currently holding this setting away from the player's own value.

  Parameters:
  - <a id="config.is_overridden.key" name="config.is_overridden.key"></a>**`key`** (string). Dotted path.

  Returns: boolean. True while an override stands, and false once the last is lifted.

- <a id="config.declare" name="config.declare"></a>[lua]`trx.config.declare(spec)`  
  Adds a setting of the game's own.

  The declaration carries no text. The engine derives `settings/<key>/title`,
  `settings/<key>/description` and, for an enum, `settings/<key>/values/<value>`, and looks each up
  in the game strings, so a declared setting is translated as every other one is.

  The setting comes up holding the player's saved value for it, whether the declaration runs before
  the settings file is read or after.

  Raises where the key is taken, or where the declaration describes a setting that could hold
  nothing it allows: an enum defaulting to a value it does not list, or an integer defaulting
  outside its own bounds.

  Parameters:
  - <a id="config.declare.spec" name="config.declare.spec"></a>**`spec`** ([trx.config.Shape](#config.Shape)). The setting's declaration.

  Example:
  ```lua
  trx.config.declare({
    key = "mod.water_color_mode",
    kind = "dynamic_enum",
    values = { "tombati", "dos", "custom" },
    default = "custom",
    ui = {
      tab = "graphic_visuals",
      before = "visuals.water_color",
    },
  })
  ```

- <a id="config.on_change" name="config.on_change"></a>[lua]`trx.config.on_change(key, fn)`  
  Calls `fn(value)` whenever the setting changes, and once as the watcher is
  attached with the value it holds now - so a script applies the player's saved value rather than
  waiting for them to touch it again.

  A watcher that changes a setting itself is heard by that setting's watchers too. One that raises
  is logged and the rest still run; it is called again on the next change.

  A watcher a level script attaches goes when the level ends, as a [`trx.events`](EVENTS.md#events) listener does. One a
  game script attaches stays for as long as the game.

  Parameters:
  - <a id="config.on_change.key" name="config.on_change.key"></a>**`key`** (string). Dotted path to watch.
  - <a id="config.on_change.fn" name="config.on_change.fn"></a>**`fn`** (function). Called with the setting's value.

  Returns: [trx.config.Watcher](#config.Watcher). The watcher, for dropping it later.

  Example:
  ```lua
  local watcher = trx.config.on_change("mod.scanlines", function(value)
    trx.log.info("scanlines are now " .. tostring(value))
  end)
  -- ... and when the script is done with it:
  watcher:detach()
  ```

- <a id="config.list" name="config.list"></a>[lua]`trx.config.list()`  
  Every setting and its current value.

  Returns: table. Maps each option's key to its value.

  Example:
  ```lua
  for key, value in pairs(trx.config.list()) do
    trx.log.info(key .. " = " .. tostring(value))
  end
  ```
