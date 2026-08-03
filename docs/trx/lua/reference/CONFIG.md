---
title: Config
order: 22
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/config.lua. Edit it there.
-->

## <a id="config" name="config"></a>Config module

Module for reading and changing engine settings.

These are the player's settings, not the level's. [`trx.config.set`](#config.set) writes to them and keeps the change: it is remembered across saves and relaunches, exactly as if the player had made it themselves. A level that wants to tint the water or pull the fog in wants [`trx.config.override`](#config.override) instead, which lasts as long as the script keeps it and leaves the player's own value untouched underneath.

### Structures

- <a id="config.Shape" name="config.Shape"></a>[lua]`trx.config.Shape`

    How a setting is entered and shown, beyond the type it reads back as.

    Properties:
    - <a id="config.Shape.kind" name="config.Shape.kind"></a>**`kind`**: string. One of `boolean`, `integer`, `number`, `color`, `enum`, `dynamic_enum` or `string`.
    - <a id="config.Shape.percent" name="config.Shape.percent"></a>**`percent`**: boolean. Marks a number stored 0-1 but entered and shown as a 0-100 percentage.
    - <a id="config.Shape.values" name="config.Shape.values"></a>**`values`**: a list of string, optional. What the setting accepts, for the enum kinds.

### Functions

- <a id="config.get" name="config.get"></a>[lua]`trx.config.get(key)`  
  Reads a setting. The value comes back as the type the option is declared with, so a boolean option reads as a boolean. Colors and enums read as strings.

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
  What shape a setting has: how a value is entered and shown, beyond the type [`trx.config.get`](#config.get) reads back.

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
  - <a id="config.set.value" name="config.set.value"></a>**`value`** (any). A boolean, a number, or a string, matching the option's type. A color is a 6-digit hex string. An enum value is taken in either spelling: underscores or the dashes the console shows.
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
  Lifts one override off a setting, putting back whatever was underneath it.

  Parameters:
  - <a id="config.restore.key" name="config.restore.key"></a>**`key`** (string). Dotted path.

  Returns: boolean. `false` if the setting was not overridden.

- <a id="config.is_overridden" name="config.is_overridden"></a>[lua]`trx.config.is_overridden(key)`  
  Whether a script or the game flow is currently holding this setting away from the player's own value.

  Parameters:
  - <a id="config.is_overridden.key" name="config.is_overridden.key"></a>**`key`** (string). Dotted path.

  Returns: boolean. True while an override stands, and false once the last is lifted.

- <a id="config.list" name="config.list"></a>[lua]`trx.config.list()`  
  Every setting and its current value.

  Returns: table. Maps each option's key to its value.

  Example:
  ```lua
  for key, value in pairs(trx.config.list()) do
    trx.log.info(key .. " = " .. tostring(value))
  end
  ```
