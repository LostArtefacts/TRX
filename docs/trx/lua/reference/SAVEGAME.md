---
title: Savegame
order: 28
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: tools/lint/gen/lua_docs
  The public API is declared next to its implementation, in
  src/lua/trx/savegame.lua. Edit it there.
-->

## <a id="savegame" name="savegame"></a>Savegame module

Manage save slots and saved games.

### Properties

- <a id="savegame.manual_allowed" name="savegame.manual_allowed"></a>**`trx.savegame.manual_allowed`** (boolean). Whether the current level allows manual saving. *(read-only)*

### Enums

- <a id="savegame.Pool" name="savegame.Pool"></a>[lua]`trx.savegame.Pool`

    Which set of save slots a slot belongs to.

    - `trx.savegame.Pool.NORMAL`  
        The numbered save slots.
    - `trx.savegame.Pool.QUICK`  
        The quick-save slots, counted and addressed by their on-screen order.

### Structures

- <a id="savegame.SlotNum" name="savegame.SlotNum"></a>[lua]`trx.savegame.SlotNum`

    Slot number within the pool. For the quick pool this is the on-screen
    order. Counted from 1.

- <a id="savegame.SlotInfo" name="savegame.SlotInfo"></a>[lua]`trx.savegame.SlotInfo`

    What a slot holds, as the save list shows it.

    Properties:
    - <a id="savegame.SlotInfo.can_restart" name="savegame.SlotInfo.can_restart"></a>**`can_restart`**: boolean. Whether the level can be restarted from the save.
    - <a id="savegame.SlotInfo.can_select_level" name="savegame.SlotInfo.can_select_level"></a>**`can_select_level`**: boolean. Whether the save reaches an earlier level in the game.
    - <a id="savegame.SlotInfo.counter" name="savegame.SlotInfo.counter"></a>**`counter`**: integer. The save count when this save was written.
    - <a id="savegame.SlotInfo.has_story" name="savegame.SlotInfo.has_story"></a>**`has_story`**: boolean. Whether story content runs before the saved level.
    - <a id="savegame.SlotInfo.is_quick" name="savegame.SlotInfo.is_quick"></a>**`is_quick`**: boolean. Whether the save is a quick save.
    - <a id="savegame.SlotInfo.level_num" name="savegame.SlotInfo.level_num"></a>**`level_num`**: integer. Where the level sits in the main level table.
    - <a id="savegame.SlotInfo.level_title" name="savegame.SlotInfo.level_title"></a>**`level_title`**: string. The name of the level the save was made in.

### Functions

- <a id="savegame.slot_count" name="savegame.slot_count"></a>[lua]`trx.savegame.slot_count([pool])`  
  Counts the slots in a pool.

  The quick pool counts only slots that hold a save.

  Parameters:
  - <a id="savegame.slot_count.pool" name="savegame.slot_count.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Returns: integer. The number of slots.

- <a id="savegame.is_free" name="savegame.is_free"></a>[lua]`trx.savegame.is_free(slot_num, [pool])`  
  Whether a slot holds no save.

  Parameters:
  - <a id="savegame.is_free.slot_num" name="savegame.is_free.slot_num"></a>**`slot_num`** ([trx.savegame.SlotNum](#savegame.SlotNum)).
  - <a id="savegame.is_free.pool" name="savegame.is_free.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Returns: boolean. Whether the slot is empty.

- <a id="savegame.load" name="savegame.load"></a>[lua]`trx.savegame.load(slot_num, [pool])`  
  Starts the saved game in a slot.

  The game flow loads it after this call returns. Raises when the slot holds
  no save.

  Parameters:
  - <a id="savegame.load.slot_num" name="savegame.load.slot_num"></a>**`slot_num`** ([trx.savegame.SlotNum](#savegame.SlotNum)).
  - <a id="savegame.load.pool" name="savegame.load.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Example:
  ```lua
  trx.savegame.load(1)
  ```

- <a id="savegame.save" name="savegame.save"></a>[lua]`trx.savegame.save([slot_num], [pool])`  
  Writes a saved game to a slot.

  A quick save without a slot number uses the next slot in the rotation.
  Otherwise, it uses the named slot.

  Parameters:
  - <a id="savegame.save.slot_num" name="savegame.save.slot_num"></a>**`slot_num`** ([trx.savegame.SlotNum](#savegame.SlotNum), optional). The quick pool uses the next slot in its rotation when it is omitted.
  - <a id="savegame.save.pool" name="savegame.save.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Returns: boolean. Whether the save was written. `false` means that the quick pool had no slot.

  Example:
  ```lua
  trx.savegame.save(1)
  ```

- <a id="savegame.info" name="savegame.info"></a>[lua]`trx.savegame.info(slot_num, [pool])`  
  Returns the slot contents, or `nil` if the slot is empty.

  Parameters:
  - <a id="savegame.info.slot_num" name="savegame.info.slot_num"></a>**`slot_num`** ([trx.savegame.SlotNum](#savegame.SlotNum)).
  - <a id="savegame.info.pool" name="savegame.info.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Returns: [trx.savegame.SlotInfo](#savegame.SlotInfo) or `nil`. What the slot holds.

  Example:
  ```lua
  local info = trx.savegame.info(1)
  if info ~= nil then
    trx.log.info(info.level_title)
  end
  ```

- <a id="savegame.delete" name="savegame.delete"></a>[lua]`trx.savegame.delete(slot_num, [pool])`  
  Removes the save from a slot and deletes its file.

  Parameters:
  - <a id="savegame.delete.slot_num" name="savegame.delete.slot_num"></a>**`slot_num`** ([trx.savegame.SlotNum](#savegame.SlotNum)).
  - <a id="savegame.delete.pool" name="savegame.delete.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Returns: boolean. Whether a save was removed.

- <a id="savegame.total_count" name="savegame.total_count"></a>[lua]`trx.savegame.total_count()`  
  Counts the saves in every pool.

  Returns: integer. The number of saves.

- <a id="savegame.restart_available" name="savegame.restart_available"></a>[lua]`trx.savegame.restart_available([slot_num], [pool])`  
  Reports whether the saved level can be restarted.

  With no slot, this uses the save the game is running from. A game that is
  not running from a save can always restart.

  Parameters:
  - <a id="savegame.restart_available.slot_num" name="savegame.restart_available.slot_num"></a>**`slot_num`** ([trx.savegame.SlotNum](#savegame.SlotNum), optional). The slot to ask about. The running save answers when it is omitted.
  - <a id="savegame.restart_available.pool" name="savegame.restart_available.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Returns: boolean. Whether the level can be restarted.

- <a id="savegame.reached_levels" name="savegame.reached_levels"></a>[lua]`trx.savegame.reached_levels(slot_num, [pool])`  
  Returns the levels up to the saved one without starting it.

  Parameters:
  - <a id="savegame.reached_levels.slot_num" name="savegame.reached_levels.slot_num"></a>**`slot_num`** ([trx.savegame.SlotNum](#savegame.SlotNum)).
  - <a id="savegame.reached_levels.pool" name="savegame.reached_levels.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

  Returns: a list of [trx.game.LevelNum](GAME.md#game.LevelNum) or `nil`. The levels, or `nil` when the slot holds no save that can be read.

- <a id="savegame.play_story" name="savegame.play_story"></a>[lua]`trx.savegame.play_story(slot_num, [pool])`  
  Plays the story content that runs before the saved level.

  Raises when the slot holds no save, or when no story runs before it.

  Parameters:
  - <a id="savegame.play_story.slot_num" name="savegame.play_story.slot_num"></a>**`slot_num`** ([trx.savegame.SlotNum](#savegame.SlotNum)).
  - <a id="savegame.play_story.pool" name="savegame.play_story.pool"></a>**`pool`** ([trx.savegame.Pool](#savegame.Pool), optional). Which set of slots to look in. Defaults to `NORMAL`.

- <a id="savegame.recent_slot" name="savegame.recent_slot"></a>[lua]`trx.savegame.recent_slot()`  
  Returns the slot where a save list should open.

  This is the slot that the game last loaded or saved. If there is no such
  slot, it is the most recently written save, and then the first numbered
  slot.

  Returns:
  - [trx.savegame.SlotNum](#savegame.SlotNum) or `nil`. The slot number, or `nil` where the game keeps no slots.
  - [trx.savegame.Pool](#savegame.Pool). Which pool it belongs to.
