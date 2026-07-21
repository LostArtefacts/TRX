---
title: Savegame
order: 24
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/savegame.lua. Edit it there.
-->

## Savegame module

The save slots, and starting or reading a saved game.

### Enums

- [lua]`trx.savegame.Pool`

    Which set of save slots a slot belongs to.

    - `trx.savegame.Pool.NORMAL` = `0`  
        The numbered save slots.
    - `trx.savegame.Pool.QUICK` = `1`  
        The quick-save slots, counted and addressed by their on-screen order.

### Functions

- [lua]`trx.savegame.slot_count([pool])`  
  How many slots a pool has. The quick pool counts only the slots that hold a save, which is how it is shown and addressed.

  Parameters:
  - **`pool`** (integer, optional). Which set of slots to look in. Defaults to `NORMAL`. Compare against `trx.savegame.Pool`.

  Returns:
  - integer. The number of slots.

- [lua]`trx.savegame.is_free(index, [pool])`  
  Whether a slot holds no save.

  Parameters:
  - **`index`** (integer). 1-based slot number. For the quick pool this is the on-screen order.
  - **`pool`** (integer, optional). Which set of slots to look in. Defaults to `NORMAL`. Compare against `trx.savegame.Pool`.

  Returns:
  - boolean. Whether the slot is empty.

- [lua]`trx.savegame.load(index, [pool])`  
  Starts the saved game in a slot. The load happens once the game flow picks it up, not on the call.

  Parameters:
  - **`index`** (integer). 1-based slot number. For the quick pool this is the on-screen order.
  - **`pool`** (integer, optional). Which set of slots to look in. Defaults to `NORMAL`. Compare against `trx.savegame.Pool`.

  Example:
  ```lua
  trx.savegame.load(1)
  ```
