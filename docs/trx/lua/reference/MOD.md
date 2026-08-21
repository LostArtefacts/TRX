---
title: Mod
order: 28
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/mod.lua. Edit it there.
-->

## <a id="mod" name="mod"></a>Mod module

The mods the game was built with, and which one is loaded.

### Properties

- <a id="mod.list" name="mod.list"></a>**`trx.mod.list`** (a list of [trx.mod.Mod](#mod.Mod)). The mods the game was built with, counted from one. *(read-only)*
- <a id="mod.current" name="mod.current"></a>**`trx.mod.current`** ([trx.mod.Mod](#mod.Mod)). The loaded mod. *(read-only)*

### Enums

- <a id="mod.Type" name="mod.Type"></a>[lua]`trx.mod.Type`

    What kind of mod it is.

    - `trx.mod.Type.BASE_GAME` = `0`  
        The base game.
    - `trx.mod.Type.EXPANSION_PACK` = `1`  
        An expansion pack.
    - `trx.mod.Type.MISC` = `2`  
        A miscellaneous mod.
    - `trx.mod.Type.DIRECT_LEVEL` = `3`  
        A single level loaded on its own.
    - `trx.mod.Type.CUSTOM` = `4`  
        A custom mod.

### Structures

- <a id="mod.Mod" name="mod.Mod"></a>[lua]`trx.mod.Mod`

    A mod the game can run. Everything on it is read-only.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="mod.Mod.base_mod" name="mod.Mod.base_mod"></a>**`base_mod`**: string. The mod this one builds on, or `nil` if it stands alone. *(read-only)*
    - <a id="mod.Mod.engine_version" name="mod.Mod.engine_version"></a>**`engine_version`**: integer. Which Tomb Raider the mod runs on. *(read-only)*
    - <a id="mod.Mod.is_available" name="mod.Mod.is_available"></a>**`is_available`**: boolean. Whether the mod's files are present. *(read-only)*
    - <a id="mod.Mod.is_valid" name="mod.Mod.is_valid"></a>**`is_valid`**: boolean. Whether the mod can be loaded. *(read-only)*
    - <a id="mod.Mod.name" name="mod.Mod.name"></a>**`name`**: string. The mod's identifier, as [`trx.mod.switch`](#mod.switch) takes it. *(read-only)*
    - <a id="mod.Mod.title" name="mod.Mod.title"></a>**`title`**: string. The mod's name, as shown to the player. *(read-only)*
    - <a id="mod.Mod.type" name="mod.Mod.type"></a>**`type`**: [trx.mod.Type](#mod.Type). What kind of mod it is. *(read-only)*

### Functions

- <a id="mod.switch" name="mod.switch"></a>[lua]`trx.mod.switch(mod)`  
  Restarts the game into another mod. The switch happens once the game flow picks it up, not on the call.

  Parameters:
  - <a id="mod.switch.mod" name="mod.switch.mod"></a>**`mod`** (any). A [`trx.mod.Mod`](#mod.Mod) or a mod name.

  Returns:
  - boolean. Whether the mod can be switched to. `false` leaves the game where it is.

  Example:
  ```lua
  trx.mod.switch("arabian-nights")
  ```
