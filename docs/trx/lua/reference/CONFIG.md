---
title: Config
order: 20
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/config.lua. Edit it there.
-->

## Config module

Module for reading and changing engine settings.

These are the player's settings, not the level's. `set` writes to them and keeps the change: it is remembered across saves and relaunches, exactly as if the player had made it themselves. A level that wants to tint the water or pull the fog in wants `override` instead, which lasts as long as the script keeps it and leaves the player's own value untouched underneath.

### Functions

- [lua]`trx.config.get(key)`  
  Reads a setting. The value comes back as the type the option is declared with, so a boolean option reads as a boolean. Colors and enums read as strings.

  Parameters:
  - **`key`** (string). Dotted path, e.g. `visuals.water_color`.

  Returns: any. Raises if no option has that key.

  Example:
  ```lua
  if trx.config.get("audio.enable_music") then
    trx.music.play(trx.catalog.music.SECRET)
  end
  ```

- [lua]`trx.config.describe(key)`  
  What shape a setting has: how a value is entered and shown, beyond the type `get` reads back.

  Parameters:
  - **`key`** (string). Dotted path.

  Returns: table. `kind` is one of `boolean`, `integer`, `number`, `color`, `enum`, `dynamic_enum` or `string`. `percent` marks a number stored 0-1 but entered and shown as a 0-100 percentage. For the enum kinds, `values` lists what the setting accepts.

  Example:
  ```lua
  for _, value in ipairs(trx.config.describe("visuals.shadow_type").values) do
    trx.log.info(value)
  end
  ```

- [lua]`trx.config.format_value(key)`  
  The current value as the console prints it: `1` or `0` for a boolean, two decimals for a plain number, a 0-100 percentage where the option is one, and enum values with dashes for underscores.

  Parameters:
  - **`key`** (string). Dotted path.

  Returns: string.

  Example:
  ```lua
  trx.console.log(trx.config.format_value("visuals.fov"))
  ```

- [lua]`trx.config.accepted_values(key)`  
  What a setting accepts, as text for an error message: `on, off` for a boolean, a marker like `[integer]` for the number kinds, or the value names for the enum kinds, with dashes for underscores.

  Parameters:
  - **`key`** (string). Dotted path.

  Returns: string or `nil`. `nil` for the kinds with nothing to list, such as a color.

- [lua]`trx.config.set(key, value, [force])`  
  Changes the player's setting, and keeps the change. Raises if the key is unknown or the value will not parse.

  The old value is not kept anywhere: the new one becomes the active setting as if the player had chosen it, and is remembered across saves and relaunches. Prefer `override` for anything a level wants only while it is running.

  Parameters:
  - **`key`** (string). Dotted path.
  - **`value`** (any). A boolean, a number, or a string, matching the option's type. A color is a 6-digit hex string. An enum value is taken in either spelling: underscores or the dashes the console shows.
  - **`force`** (boolean, optional). Write through a setting the game flow enforces.

- [lua]`trx.config.reset(key, [force])`  
  Puts a setting back to its default, and keeps the change, as `set` does.

  Parameters:
  - **`key`** (string). Dotted path.
  - **`force`** (boolean, optional). As for `set`.

  Returns: boolean. `false` when a script or the game flow is holding the setting (see `is_overridden`).

- [lua]`trx.config.override(key, value)`  
  Changes a setting for as long as the script keeps the override, without touching the player's own value.

  The player's value sits underneath and comes back on `restore`. Nothing is written to disk. Overrides stack, so one can be pushed over another; each `restore` lifts one off. A setting the game flow enforces cannot be overridden.

  Parameters:
  - **`key`** (string). Dotted path.
  - **`value`** (any). As for `set`.

  Example:
  ```lua
  trx.config.override("visuals.water_color", "0080ff")
  -- ... and when the level is done with it:
  trx.config.restore("visuals.water_color")
  ```

- [lua]`trx.config.restore(key)`  
  Lifts one override off a setting, putting back whatever was underneath it.

  Parameters:
  - **`key`** (string). Dotted path.

  Returns: boolean. `false` if the setting was not overridden.

- [lua]`trx.config.is_overridden(key)`  
  Whether a script or the game flow is currently holding this setting away from the player's own value.

  Parameters:
  - **`key`** (string). Dotted path.

  Returns: boolean.

- [lua]`trx.config.list()`  
  Every setting and its current value.

  Returns: table. Maps each option's key to its value.

  Example:
  ```lua
  for key, value in pairs(trx.config.list()) do
    trx.log.info(key .. " = " .. tostring(value))
  end
  ```
