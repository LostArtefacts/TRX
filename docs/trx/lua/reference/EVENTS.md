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

- [lua]`trx.events.on_room_change(callback)`  
  Happens when an item changes rooms during play, which a cutscene or the attract demo is not. `trx.rooms.Room:on_enter` and `:on_exit` are this same event, narrowed to one room.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` that changed rooms.
    - **`old_room`** (integer). 0-based number of the room it left, or -1 if it had none.
    - **`new_room`** (integer). 0-based number of the room it entered, or -1 if it left the world.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_room_change(function(item, old_room, new_room)
    trx.log.info(item.object_id .. " moved to room " .. new_room)
  end)
  ```

- [lua]`trx.events.on_trigger(callback)`  
  Happens every time a trigger is aimed at an item - a floor trigger in the level, the `/trigger` console command, or `item:trigger` from a script - of any kind, an antitrigger included. It is the raw trigger, not a state change: a floor pad fires it every frame Lara stands on it, and a partial trigger fires it too. A cutscene or the attract demo does not.

  The handler runs after the trigger has been applied, so the item already reflects it, and changes the handler makes to the item are not overwritten.

  `trx.items.Item:on_trigger` is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` the trigger was aimed at.
    - **`trigger`** (table). What the trigger carried: `type` (an `items.TriggerType`), `mask` (the code bits it set, `1` to `31`), `timer` (in seconds), and `one_shot`.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_trigger(function(item, trigger)
    if trigger.type == trx.items.TriggerType.ANTITRIGGER then
      trx.log.info(item.object_id .. " was antitriggered")
    end
  end)
  ```

- [lua]`trx.events.on_show(callback)`  
  Happens when an item becomes visible during play - drawn and in the world, taking part in collision and targeting. It is the change that fires, not the state: an item already visible does not fire it again, and only a live level does, not a cutscene or the attract demo.

  `trx.items.Item:on_show` is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` that became visible.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_show(function(item)
    trx.log.info(item.object_id .. " appeared")
  end)
  ```

- [lua]`trx.events.on_hide(callback)`  
  Happens when an item becomes hidden during play - drawn and in the world, taking part in collision and targeting. It is the change that fires, not the state: an item already hidden does not fire it again, and only a live level does, not a cutscene or the attract demo.

  `trx.items.Item:on_hide` is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` that became hidden.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_hide(function(item)
    trx.log.info(item.object_id .. " vanished")
  end)
  ```

- [lua]`trx.events.on_finish(callback)`  
  Happens when an item finishes its run during play - a trap that has sprung, a switch thrown, a one-shot object spent. It is the change that fires, once, and only a live level does, not a cutscene or the attract demo.

  `trx.items.Item:on_finish` is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` that finished.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_finish(function(item)
    trx.log.info(item.object_id .. " finished its run")
  end)
  ```

- [lua]`trx.events.on_enter_sim(callback)`  
  Happens when an item starts being simulated during play - its control routine begins running each frame. Every path that starts an item fires it: a trigger, a switch, a respawn, a cheat. A trigger also fires `on_activate`, which this does not.

  `trx.items.Item:on_enter_sim` is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` that started being simulated.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_enter_sim(function(item)
    trx.log.info(item.object_id .. " started running")
  end)
  ```

- [lua]`trx.events.on_leave_sim(callback)`  
  Happens when an item stops being simulated during play - its control routine no longer runs. It keeps its place and its state; it merely stops.

  `trx.items.Item:on_leave_sim` is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` that stopped being simulated.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_leave_sim(function(item)
    trx.log.info(item.object_id .. " stopped running")
  end)
  ```

- [lua]`trx.events.on_activate(callback)`  
  Happens when an item is activated through the lifecycle front door during play - the path a level trigger takes. Switches, respawns and cheats start an item without it, firing only `on_enter_sim`; watch that one for a start of any cause.

  `trx.items.Item:on_activate` is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` that was activated.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_activate(function(item)
    trx.log.info(item.object_id .. " was activated")
  end)
  ```

- [lua]`trx.events.on_deactivate(callback)`  
  Happens when a running item is deactivated through the lifecycle front door during play - the path an antitrigger takes. It fires only when the item was actually running.

  `trx.items.Item:on_deactivate` is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` that was deactivated.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_deactivate(function(item)
    trx.log.info(item.object_id .. " was deactivated")
  end)
  ```

- [lua]`trx.events.on_destroy(callback)`  
  Happens as an item is removed from the game during play - a creature cleared away, a pickup taken, an object that has run its course. The item can still be read from the handler, which runs before the removal completes, but a handle kept past the handler goes stale.

  `trx.items.Item:on_destroy` is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` being removed. Valid only for the duration of the handler.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_destroy(function(item)
    trx.log.info(item.object_id .. " was removed")
  end)
  ```

- [lua]`trx.events.on_enter_world(callback)`  
  Happens when an item enters the world during play - a runtime spawn, such as a creature an emitter releases or an item a script creates. The level's own items do not fire it as they load; only an arrival during a live level counts.

  `trx.items.Item:on_enter_world` is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` that entered the world.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_enter_world(function(item)
    trx.log.info(item.object_id .. " entered the world")
  end)
  ```

- [lua]`trx.events.on_leave_world(callback)`  
  Happens when an item leaves the world during play - unlinked from its room, no longer drawn or collidable. It need not be destroyed; a destroyed item leaves the world on its way out, and fires this first.

  `trx.items.Item:on_leave_world` is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** (Item). The `trx.items.Item` that left the world.

  Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

  Example:
  ```lua
  trx.events.on_leave_world(function(item)
    trx.log.info(item.object_id .. " left the world")
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
