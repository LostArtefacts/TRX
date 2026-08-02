---
title: Events
order: 1
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/events.lua. Edit it there.
-->

## <a id="events" name="events"></a>Events module

Lua scripts can listen for game events by attaching a handler to one of the hooks below. Attaching returns a listener id, which [`trx.events.detach`](#events.detach) takes.

A handler attached from a level script is detached automatically when the level ends; one attached from a global script lives for the whole session.

An event that carries a default the script may take over says so in its description; a handler answers such an event by returning true, and the default then stands down. Every other event ignores what its handlers return.

### Structures

- <a id="events.Listener" name="events.Listener"></a>[lua]`trx.events.Listener`

    An attached handler. Every hook hands one back, and holding it is what makes the handler detachable later. A listener is spent once detached, and a level change spends every one a level script attached.

    Properties:
    - <a id="events.Listener.id" name="events.Listener.id"></a>**`id`**: integer. The number the engine keys the handler by. Two listeners of the same handler carry the same one; it is never handed out twice within a session. *(read-only)*

    Methods:

    - <a id="events.Listener.detach" name="events.Listener.detach"></a>[lua]`listener:detach()`  
      Stops the handler, which fires no more from here on. [`trx.events.detach`](#events.detach) does the same to a listener held elsewhere.

      Returns: boolean. Whether the handler was still attached.

### Functions

- <a id="events.on_game_start" name="events.on_game_start"></a>[lua]`trx.events.on_game_start(callback)`  
  Happens as a level starts running, before its first frame is drawn. By then
  the level file is loaded, its items are set up and any savegame state has
  been applied, so this is where a script sets object properties, declares
  allies, changes room state and plays sound effects. Every kind of level
  fires it: a played level, a cutscene and the attract demo alike. The title
  screen has [`trx.events.on_title_start`](#events.on_title_start) instead.

  Which level is starting is [`trx.game.current_level`](GAME.md#game.current_level), whose [`trx.game.Level.num`](GAME.md#game.Level.num) and [`trx.game.Level.type`](GAME.md#game.Level.type)
  say where it counts and what kind it is. A level script already knows both,
  which is why the handler is not handed them.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`is_save`** (boolean). Whether the level is being resumed from a savegame rather than started fresh. A cutscene and a demo are never resumed, and always report false.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_game_start(function(is_save)
    trx.log.info(trx.game.current_level.title .. " is up")
  end)
  ```

- <a id="events.on_title_start" name="events.on_title_start"></a>[lua]`trx.events.on_title_start(callback)`  
  Happens when the title screen's scene starts playing behind the menu, once
  its level is loaded and its items are set up. The handler takes no
  arguments. [`trx.events.on_game_start`](#events.on_game_start) does not fire for the title level.

  Parameters:
  - **`callback`** (function).

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_title_start(function()
    trx.log.info("the menu is up")
  end)
  ```

- <a id="events.on_pickup" name="events.on_pickup"></a>[lua]`trx.events.on_pickup(callback)`  
  Happens just after Lara picks up an item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item_num`** ([trx.items.Num](ITEMS.md#items.Num)). The item that was picked up.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_pickup(function(item_num)
    trx.log.info(trx.items[item_num].object_id)
  end)
  ```

- <a id="events.before_control" name="events.before_control"></a>[lua]`trx.events.before_control(callback)`  
  Happens on every logical game frame, before the main game logic runs. The handler takes no arguments.

  Parameters:
  - **`callback`** (function).

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

- <a id="events.after_control" name="events.after_control"></a>[lua]`trx.events.after_control(callback)`  
  Happens on every logical game frame, after the main game logic runs. The handler takes no arguments.

  Parameters:
  - **`callback`** (function).

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

- <a id="events.on_flip_effect" name="events.on_flip_effect"></a>[lua]`trx.events.on_flip_effect(effect_num, callback)`  
  Claims a flip effect number and happens whenever a level runs it, whether from a floor trigger or an animation command. Place an ordinary flipeffect trigger in a level editor - pad, heavy, switch and antitrigger all work - pick an unused effect number, and handle it here from the level's script.

  A claimed number belongs to the script for the rest of the level: its stock engine effect does not run, even if the handler is later detached. Unclaimed numbers are unaffected.

  Unlike the other hooks, this happens at effect execution time, in the middle of a game frame.

  Parameters:
  - **`effect_num`** (integer). The flip effect number to claim, as the level editor numbers them. This is not the id space of [`trx.rooms.flip_effect`](ROOMS.md#rooms.flip_effect), which takes [`trx.catalog.flip_effects`](CATALOG.md#catalog.flip_effects) names. Counted from 0.
  - **`callback`** (function).
    Called with:
    - **`timer`** (integer). A floor trigger's timer field, free for the level to use as a parameter. 0 for an animation command, which carries no timer.
    - **`item_num`** ([trx.items.Num](ITEMS.md#items.Num)). The item that ran the effect: Lara for a pad trigger, the activating object for a heavy trigger, the animating item for an animation command.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_flip_effect(62, function(timer, item_num)
    trx.log.info("flipeffect 62 ran with timer " .. timer)
  end)
  ```

- <a id="events.on_room_change" name="events.on_room_change"></a>[lua]`trx.events.on_room_change(callback)`  
  Happens when an item changes rooms during play, which a cutscene or the attract demo is not. [`trx.rooms.Room:on_enter`](ROOMS.md#rooms.Room.on_enter) and [`trx.rooms.Room:on_exit`](ROOMS.md#rooms.Room.on_exit) are this same event, narrowed to one room.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that changed rooms.
    - **`old_room_num`** ([trx.rooms.Num](ROOMS.md#rooms.Num)). -1 if it had none.
    - **`new_room_num`** ([trx.rooms.Num](ROOMS.md#rooms.Num)). -1 if it left the world.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_room_change(function(item, old_room_num, new_room_num)
    trx.log.info(item.object_id .. " moved to room " .. new_room_num)
  end)
  ```

- <a id="events.on_trigger" name="events.on_trigger"></a>[lua]`trx.events.on_trigger(callback)`  
  Happens every time a trigger is aimed at an item - a floor trigger in the level, the `/trigger` console command, or [`trx.items.Item:trigger`](ITEMS.md#items.Item.trigger) from a script - of any kind, an antitrigger included. It is the raw trigger, not a state change: a floor pad fires it every frame Lara stands on it, and a partial trigger fires it too. A cutscene or the attract demo does not.

  The handler runs after the trigger has been applied, so the item already reflects it, and changes the handler makes to the item are not overwritten.

  [`trx.items.Item:on_trigger`](ITEMS.md#items.Item.on_trigger) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item the trigger was aimed at.
    - **`trigger`** (table). What the trigger carried: `type` (a [`trx.items.TriggerType`](ITEMS.md#items.TriggerType)), `mask` (the code bits it set, `1` to `31`), `timer` (in seconds), and `one_shot`.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_trigger(function(item, trigger)
    if trigger.type == trx.items.TriggerType.ANTITRIGGER then
      trx.log.info(item.object_id .. " was antitriggered")
    end
  end)
  ```

- <a id="events.on_show" name="events.on_show"></a>[lua]`trx.events.on_show(callback)`  
  Happens when an item becomes visible during play - drawn and in the world, taking part in collision and targeting. It is the change that fires, not the state: an item already visible does not fire it again, and only a live level does, not a cutscene or the attract demo.

  [`trx.items.Item:on_show`](ITEMS.md#items.Item.on_show) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that became visible.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_show(function(item)
    trx.log.info(item.object_id .. " appeared")
  end)
  ```

- <a id="events.on_hide" name="events.on_hide"></a>[lua]`trx.events.on_hide(callback)`  
  Happens when an item becomes hidden during play - drawn and in the world, taking part in collision and targeting. It is the change that fires, not the state: an item already hidden does not fire it again, and only a live level does, not a cutscene or the attract demo.

  [`trx.items.Item:on_hide`](ITEMS.md#items.Item.on_hide) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that became hidden.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_hide(function(item)
    trx.log.info(item.object_id .. " vanished")
  end)
  ```

- <a id="events.on_finish" name="events.on_finish"></a>[lua]`trx.events.on_finish(callback)`  
  Happens when an item finishes its run during play - a trap that has sprung, a switch thrown, a one-shot object spent. It is the change that fires, once, and only a live level does, not a cutscene or the attract demo.

  [`trx.items.Item:on_finish`](ITEMS.md#items.Item.on_finish) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that finished.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_finish(function(item)
    trx.log.info(item.object_id .. " finished its run")
  end)
  ```

- <a id="events.on_enter_sim" name="events.on_enter_sim"></a>[lua]`trx.events.on_enter_sim(callback)`  
  Happens when an item starts being simulated during play - its control routine begins running each frame. Every path that starts an item fires it: a trigger, a switch, a respawn, a cheat. A trigger also fires [`trx.events.on_activate`](#events.on_activate), which this does not.

  [`trx.items.Item:on_enter_sim`](ITEMS.md#items.Item.on_enter_sim) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that started being simulated.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_enter_sim(function(item)
    trx.log.info(item.object_id .. " started running")
  end)
  ```

- <a id="events.on_leave_sim" name="events.on_leave_sim"></a>[lua]`trx.events.on_leave_sim(callback)`  
  Happens when an item stops being simulated during play - its control routine no longer runs. It keeps its place and its state; it merely stops.

  [`trx.items.Item:on_leave_sim`](ITEMS.md#items.Item.on_leave_sim) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that stopped being simulated.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_leave_sim(function(item)
    trx.log.info(item.object_id .. " stopped running")
  end)
  ```

- <a id="events.on_activate" name="events.on_activate"></a>[lua]`trx.events.on_activate(callback)`  
  Happens when an item is activated through the lifecycle front door during play - the path a level trigger takes. Switches, respawns and cheats start an item without it, firing only [`trx.events.on_enter_sim`](#events.on_enter_sim); watch that one for a start of any cause.

  [`trx.items.Item:on_activate`](ITEMS.md#items.Item.on_activate) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that was activated.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_activate(function(item)
    trx.log.info(item.object_id .. " was activated")
  end)
  ```

- <a id="events.on_deactivate" name="events.on_deactivate"></a>[lua]`trx.events.on_deactivate(callback)`  
  Happens when a running item is deactivated through the lifecycle front door during play - the path an antitrigger takes. It fires only when the item was actually running.

  [`trx.items.Item:on_deactivate`](ITEMS.md#items.Item.on_deactivate) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that was deactivated.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_deactivate(function(item)
    trx.log.info(item.object_id .. " was deactivated")
  end)
  ```

- <a id="events.on_destroy" name="events.on_destroy"></a>[lua]`trx.events.on_destroy(callback)`  
  Happens as an item is removed from the game during play - a creature cleared away, a pickup taken, an object that has run its course. The item can still be read from the handler, which runs before the removal completes, but a handle kept past the handler goes stale.

  [`trx.items.Item:on_destroy`](ITEMS.md#items.Item.on_destroy) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item being removed. Valid only for the duration of the handler.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_destroy(function(item)
    trx.log.info(item.object_id .. " was removed")
  end)
  ```

- <a id="events.on_enter_world" name="events.on_enter_world"></a>[lua]`trx.events.on_enter_world(callback)`  
  Happens when an item enters the world during play - a runtime spawn, such as a creature an emitter releases or an item a script creates. The level's own items do not fire it as they load; only an arrival during a live level counts.

  [`trx.items.Item:on_enter_world`](ITEMS.md#items.Item.on_enter_world) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that entered the world.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_enter_world(function(item)
    trx.log.info(item.object_id .. " entered the world")
  end)
  ```

- <a id="events.on_leave_world" name="events.on_leave_world"></a>[lua]`trx.events.on_leave_world(callback)`  
  Happens when an item leaves the world during play - unlinked from its room, no longer drawn or collidable. It need not be destroyed; a destroyed item leaves the world on its way out, and fires this first.

  [`trx.items.Item:on_leave_world`](ITEMS.md#items.Item.on_leave_world) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that left the world.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_leave_world(function(item)
    trx.log.info(item.object_id .. " left the world")
  end)
  ```

- <a id="events.on_hit" name="events.on_hit"></a>[lua]`trx.events.on_hit(callback)`  
  Happens when an item takes damage, Lara included. It is the raw damage that
  fires, before the item's hit points are clamped, so a fatal blow reports the whole amount the
  attacker dealt. A death that does not go through damage - a script writing
  [`trx.items.Item.hit_points`](ITEMS.md#items.Item.hit_points), or [`trx.items.Item:destroy`](ITEMS.md#items.Item.destroy) - does not report.

  [`trx.items.Item:on_hit`](ITEMS.md#items.Item.on_hit) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that took the damage.
    - **`damage`** (integer). Hit points taken, before clamping to zero.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_hit(function(item, damage)
    trx.log.info(item.object_id .. " lost " .. damage .. " hit points")
  end)
  ```

- <a id="events.on_kill" name="events.on_kill"></a>[lua]`trx.events.on_kill(callback)`  
  Happens when damage takes an item's hit points to zero, Lara included. It is the
  same blow [`trx.events.on_hit`](#events.on_hit) reports, which fires first. A death that does not go through damage
  - a script writing [`trx.items.Item.hit_points`](ITEMS.md#items.Item.hit_points), or [`trx.items.Item:destroy`](ITEMS.md#items.Item.destroy) - does not report.

  Some bosses fall and get back up: Willard is knocked out, Natla plays dead before her second
  stage, and the dragon lies still until Lara takes the dagger. Each stage brings their hit points
  to zero, so they report once per stage rather than once per boss, and the dragon reports once
  more for the dagger that ends it.

  [`trx.items.Item:on_kill`](ITEMS.md#items.Item.on_kill) is this same event, narrowed to one item.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that was brought down.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_kill(function(item)
    trx.log.info(item.object_id .. " is down")
  end)
  ```

- <a id="events.on_cutscene_trigger" name="events.on_cutscene_trigger"></a>[lua]`trx.events.on_cutscene_trigger(callback)`  
  Happens when a cutscene trigger fires, before the engine acts on it. A
  handler answers the trigger by returning true - having played a cutscene of
  its own, run something else, or decided nothing should run. If no handler
  answers, the engine plays the cutscene the trigger names.

  A trigger Lara stands on fires every frame, so this happens only for a
  cutscene that has not run yet and while none is playing. Asking counts as
  running it, whatever came of it, so the same handler is not asked again on
  the next frame. Clear the mark with [`trx.cutscenes.set_played`](CUTSCENES.md#cutscenes.set_played) to hear
  about one again.

  The number a trigger names need not be one the game has a cutscene for -
  TR4 uses 32 to ask for a full-motion video. Those reach a handler too, and
  the engine has nothing of its own to do about them.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`cutscene_num`** ([trx.cutscenes.Num](CUTSCENES.md#cutscenes.Num)). The number the trigger names, which the game need not have a cutscene for.

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  -- only in the throne room; a flyby stands in for it elsewhere
  trx.events.on_cutscene_trigger(function(cutscene_num)
    if cutscene_num ~= 27 then
      return false
    end
    if trx.lara.item.room_num == 55 then
      trx.cutscenes.play(27)
    else
      trx.camera.play_flyby(3)
    end
    return true
  end)
  ```

- <a id="events.on_cutscene_start" name="events.on_cutscene_start"></a>[lua]`trx.events.on_cutscene_start(callback)`  
  Happens when a TR4 cutscene's first frame is about to show, after the fade out.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`cutscene_num`** ([trx.cutscenes.Num](CUTSCENES.md#cutscenes.Num)).

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

- <a id="events.on_cutscene_end" name="events.on_cutscene_end"></a>[lua]`trx.events.on_cutscene_end(callback)`  
  Happens once a TR4 cutscene has finished and the scene it interrupted is back. This is where a script decides what follows.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`cutscene_num`** ([trx.cutscenes.Num](CUTSCENES.md#cutscenes.Num)).

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_cutscene_end(function(cutscene_num)
    trx.log.info("cutscene " .. cutscene_num .. " finished")
  end)
  ```

- <a id="events.on_flyby_end" name="events.on_flyby_end"></a>[lua]`trx.events.on_flyby_end(callback)`  
  Happens when a flyby sequence reaches its last camera and hands the view back.
  A sequence that a cutscene or the player interrupts does not fire it.

  Parameters:
  - **`callback`** (function).
    Called with:
    - **`sequence_num`** ([trx.camera.SequenceNum](CAMERA.md#camera.SequenceNum)).

  Returns: [trx.events.Listener](#events.Listener). The attached handler.

  Example:
  ```lua
  trx.events.on_flyby_end(function(sequence_num)
    trx.camera.play_flyby(sequence_num)
  end)
  ```

- <a id="events.detach" name="events.detach"></a>[lua]`trx.events.detach(listener)`  
  Removes a previously attached handler, which stops firing immediately. [`trx.events.Listener:detach`](#events.Listener.detach) does the same to one held in hand.

  Parameters:
  - **`listener`** ([trx.events.Listener](#events.Listener)).

  Returns: boolean. Whether the handler was still attached. `false` means it had already been detached, or the level it belonged to has ended.

  Example:
  ```lua
  local listener = trx.events.before_control(function()
    -- handle control loop event
  end)
  trx.events.detach(listener)
  ```
