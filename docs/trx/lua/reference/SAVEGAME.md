---
title: Savegame
order: 27
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/savegame.lua. Edit it there.
-->

## <a id="savegame" name="savegame"></a>Savegame module

The save slots, and starting or reading a saved game.

### Enums

- <a id="savegame.Pool" name="savegame.Pool"></a>[lua]`trx.savegame.Pool`

    Which set of save slots a slot belongs to.

    - `trx.savegame.Pool.NORMAL` = `0`  
        The numbered save slots.
    - `trx.savegame.Pool.QUICK` = `1`  
        The quick-save slots, counted and addressed by their on-screen order.

### Structures

- <a id="savegame.SlotNum" name="savegame.SlotNum"></a>[lua]`trx.savegame.SlotNum`

    Slot number within the pool. For the quick pool this is the on-screen order. Counted from 1.

### Functions

- <a id="savegame.slot_count" name="savegame.slot_count"></a>[lua]`trx.savegame.slot_count([pool])`  
  How many slots a pool has. The quick pool counts only the slots that hold a save, which is how it is shown and addressed.

  Parameters:
  - <a id="savegame.slot_count.pool" name="savegame.slot_count.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Returns:
  - integer. The number of slots.

- <a id="savegame.is_free" name="savegame.is_free"></a>[lua]`trx.savegame.is_free(slot_num, [pool])`  
  Whether a slot holds no save.

  Parameters:
  - <a id="savegame.is_free.slot_num" name="savegame.is_free.slot_num"></a>**`slot_num`** ([trx.savegame.SlotNum](#savegame.SlotNum)).
  - <a id="savegame.is_free.pool" name="savegame.is_free.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Returns:
  - boolean. Whether the slot is empty.

- <a id="savegame.load" name="savegame.load"></a>[lua]`trx.savegame.load(slot_num, [pool])`  
  Starts the saved game in a slot. The load happens once the game flow picks it up, not on the call.

  Parameters:
  - <a id="savegame.load.slot_num" name="savegame.load.slot_num"></a>**`slot_num`** ([trx.savegame.SlotNum](#savegame.SlotNum)).
  - <a id="savegame.load.pool" name="savegame.load.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Example:
  ```lua
  trx.savegame.load(1)
  ```

- <a id="savegame.save" name="savegame.save"></a>[lua]`trx.savegame.save([slot_num], [pool])`  
  Writes a saved game to a slot. A quick save with no slot number goes to the next slot in the rotation; with one, it saves to the slot named.

  Parameters:
  - <a id="savegame.save.slot_num" name="savegame.save.slot_num"></a>**`slot_num`** ([trx.savegame.SlotNum](#savegame.SlotNum), optional). The quick pool uses the next slot in its rotation when it is omitted.
  - <a id="savegame.save.pool" name="savegame.save.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Returns:
  - boolean. Whether the save was written. `false` means the quick pool had no slot.

  Example:
  ```lua
  trx.savegame.save(1)
  ```
