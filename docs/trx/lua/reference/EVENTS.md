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
    - **`item_num`** (integer). 0-based index of the item that was picked up.

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

- [lua]`trx.events.on_flip_effect(effect_num, callback)`  
  Claims a flip effect number and happens whenever a level runs it, whether from a floor trigger or an animation command. Place an ordinary flipeffect trigger in a level editor - pad, heavy, switch and antitrigger all work - pick an unused effect number, and handle it here from the level's script.

  A claimed number belongs to the script for the rest of the level: its stock engine effect does not run, even if the handler is later detached. Unclaimed numbers are unaffected.

  Unlike the other hooks, this happens at effect execution time, in the middle of a game frame.

  Parameters:
  - **`effect_num`** (integer). The flip effect number to claim, as the level editor numbers them. This is not the id space of `trx.rooms.flip_effect`, which takes `trx.catalog.flip_effects` names.
  - **`callback`** (function).
    Called with:
    - **`timer`** (integer). A floor trigger's timer field, free for the level to use as a parameter. 0 for an animation command, which carries no timer.
    - **`item_num`** (integer). 0-based index of the item that ran the effect: Lara for a pad trigger, the activating object for a heavy trigger, the animating item for an animation command.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_flip_effect(62, function(timer, item_num)
    trx.log.info("flipeffect 62 ran with timer " .. timer)
  end)
  ```

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
