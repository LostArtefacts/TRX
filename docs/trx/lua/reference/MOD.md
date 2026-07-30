---
title: Mod
order: 23
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/mod.lua. Edit it there.
-->

## Mod module

The mods the game was built with, and which one is loaded.

### Properties

- **`trx.mod.list`** (table). The mods the game was built with, as a list of `trx.mod.Mod` counted from one. *(read-only)*
- **`trx.mod.current`** (Mod). The loaded mod, as a `trx.mod.Mod`. *(read-only)*

### Enums

- [lua]`trx.mod.Type`

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

- [lua]`trx.mod.Mod`

    A mod the game can run. Everything on it is read-only.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`base_mod`**: string. The mod this one builds on, or `nil` if it stands alone. *(read-only)*
    - **`engine_version`**: integer. Which Tomb Raider the mod runs on. *(read-only)*
    - **`is_available`**: boolean. Whether the mod's files are present. *(read-only)*
    - **`is_valid`**: boolean. Whether the mod can be loaded. *(read-only)*
    - **`name`**: string. The mod's identifier, as `trx.mod.switch` takes it. *(read-only)*
    - **`title`**: string. The mod's name, as shown to the player. *(read-only)*
    - **`type`**: integer. What kind of mod it is. Compare against `trx.mod.Type`. *(read-only)*

### Functions

- [lua]`trx.mod.switch(mod)`  
  Restarts the game into another mod. The switch happens once the game flow picks it up, not on the call.

  Parameters:
  - **`mod`** (any). A `trx.mod.Mod` or a mod name.

  Returns:
  - boolean. Whether the mod can be switched to. `false` leaves the game where it is.

  Example:
  ```lua
  trx.mod.switch("arabian-nights")
  ```
