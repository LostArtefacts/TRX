---
title: Events
order: 2
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/events.lua. Edit it there.
-->

## Events module

Lua scripts can listen for game events by attaching a handler to one of the hooks below. Attaching returns a listener id, which `trx.events.detach` takes.

A handler attached from a level script is detached automatically when the level ends; one attached from a global script lives for the whole session.

### Functions

- [lua]`trx.events.before_level_file(callback)`  
  Happens prior to loading the level file.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`level_num`** (integer). Number of the level the event fired for.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.before_level_file(function(level_num)
    -- handle pre-file-load setup
  end)
  ```

- [lua]`trx.events.after_level_file(callback)`  
  Happens after the level finishes loading, prior to loading information from a savegame.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`level_num`** (integer). Number of the level the event fired for.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

- [lua]`trx.events.before_item_setup(callback)`  
  Happens after level items exist, before they are initialized. Use this to set object or item properties that item initialization reads.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`level_num`** (integer). Number of the level the event fired for.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

- [lua]`trx.events.after_item_setup(callback)`  
  Happens after level items exist, after they are initialized.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`level_num`** (integer). Number of the level the event fired for.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

- [lua]`trx.events.after_level_state(callback)`  
  Happens after the level finishes loading, after loading information from a savegame. If the game is started normally, this duplicates `after_level_file`.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`level_num`** (integer). Number of the level the event fired for.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.after_level_state(function(level_num)
    -- handle post-savegame state restore
  end)
  ```

- [lua]`trx.events.on_game_start(callback)`  
  Happens after the level finishes loading and the game is about to start. Unlike `after_level_file` and `after_level_state`, this waits for the fade-to-black / cross-fade effects to finish, so it is the place to play sound effects and run game logic.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`level_num`** (integer). Number of the level being started.
    - **`is_save`** (boolean). Whether the level is being resumed from a savegame rather than started fresh.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

- [lua]`trx.events.on_pickup(callback)`  
  Happens just after Lara picks up an item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item_num`** (integer). 1-based index of the item that was picked up.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_pickup(function(item_num)
    trx.log.info(trx.items[item_num].object_id)
  end)
  ```

- [lua]`trx.events.before_control(callback)`  
  Happens on every logical game frame, before the main game logic runs. The handler takes no arguments.

  Parameters:
  - **`callback`** (function).

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

- [lua]`trx.events.after_control(callback)`  
  Happens on every logical game frame, after the main game logic runs. The handler takes no arguments.

  Parameters:
  - **`callback`** (function).

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

- [lua]`trx.events.detach(listener_id)`  
  Removes a previously attached handler, which stops firing immediately.

  Parameters:
  - **`listener_id`** (integer). The id attach returned.

  Returns: boolean. Whether a handler with that id was attached. `false` means it had already been detached, or the id was never handed out.

  Example:
  ```lua
  local id = trx.events.before_control(function()
    -- handle control loop event
  end)
  trx.events.detach(id)
  ```
