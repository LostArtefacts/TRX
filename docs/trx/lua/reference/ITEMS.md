---
title: Items
order: 2
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/items.lua. Edit it there.
-->

## <a id="items" name="items"></a>Items module

Module for controlling all moveables.

### Indexing

Indexing the module reaches an item, and `#trx.items` is how many the level has. `pairs()` walks them in order, keyed by the item number.

- <a id="items[]" name="items[]"></a>**`trx.items[key]`** (key: [trx.items.Num](#items.Num) or string, value: [trx.items.Item](#items.Item) or `nil`). An item's unique name reaches it as well.
- **`#trx.items`** (integer). How many there are.

Example:
```lua
for num, item in pairs(trx.items) do
  trx.log.info(item.object_id)
end
```

### Properties

- <a id="items.query" name="items.query"></a>**`trx.items.query`** ([trx.items.ItemQuery](#items.ItemQuery)). The identity query over every item in the level. Narrow it and read it. *(read-only)*

### Enums

- <a id="items.PickupMode" name="items.PickupMode"></a>[lua]`trx.items.PickupMode`

    The values the `pickup_mode` item property can take. It selects the animation Lara plays when collecting the item.

    - `trx.items.PickupMode.NORMAL` = `0`  
        Picked up off the floor.
    - `trx.items.PickupMode.PLINTH_LOW` = `1`  
        Picked up from a low pedestal.
    - `trx.items.PickupMode.PLINTH_HIGH` = `2`  
        Picked up from a high pedestal.
    - `trx.items.PickupMode.HIDDEN` = `3`  
        Hidden behind an object Lara can reach into.
    - `trx.items.PickupMode.CROWBAR` = `4`  
        Pried off the wall using a crowbar.
    - `trx.items.PickupMode.SARCOPHAGUS` = `5`  
        Hidden inside a sarcophagus.
    - `trx.items.PickupMode.PLINTH_SCION` = `6`  
        Similar to PLINTH_HIGH; invokes Lara's extra animation as in Tomb of Qualopec.

- <a id="items.ScaledSpikesMode" name="items.ScaledSpikesMode"></a>[lua]`trx.items.ScaledSpikesMode`

    The values the `scaled_spikes_mode` item property can take. It determines how spikes behave when triggered.

    - `trx.items.ScaledSpikesMode.LOOPING` = `0`  
        Spikes will extend, wait a brief period, retract, and then the loop will repeat.
    - `trx.items.ScaledSpikesMode.EXTENDED` = `1`  
        Spikes will extend and remain as-is indefinitely.
    - `trx.items.ScaledSpikesMode.ONE_SHOT` = `2`  
        Spikes will extend, wait a brief period, retract, and then stop.

- <a id="items.SwitchMode" name="items.SwitchMode"></a>[lua]`trx.items.SwitchMode`

    The values the `switch_mode` item property can take. It selects the animation Lara plays when interacting with the item.

    - `trx.items.SwitchMode.NORMAL` = `0`  
        A regular/classic wall lever.
    - `trx.items.SwitchMode.HIDDEN_REACH` = `1`  
        Lara reaches in to activate.
    - `trx.items.SwitchMode.HIDDEN_PICKUP` = `2`  
        Lara reaches in to collect a pickup.
    - `trx.items.SwitchMode.SHOVE` = `3`  
        A single-use button that requires a shove to activate.

- <a id="items.TriggerType" name="items.TriggerType"></a>[lua]`trx.items.TriggerType`

    The kind of trigger [`trx.items.Item:trigger`](#items.Item.trigger) fires, matching the trigger types a level editor offers. Most are forward triggers that differ only in what trips them in a level; from a script they behave alike, and `TRIGGER` is the one to reach for.

    - `trx.items.TriggerType.TRIGGER` = `0`  
        A plain trigger: sets the code bits and, once they are all set, starts the item.
    - `trx.items.TriggerType.HEAVY` = `1`  
        A forward trigger a heavy object trips. A falling block reads this to know it was set off by weight.
    - `trx.items.TriggerType.SWITCH` = `2`  
        Toggles the code bits, so firing it a second time takes the trigger back.
    - `trx.items.TriggerType.HEAVY_SWITCH` = `3`  
        A switch a heavy object trips.
    - `trx.items.TriggerType.ANTITRIGGER` = `4`  
        Takes the trigger back, clearing the code bits. The item is left running so it can stand itself down, which is how a door animates shut.

- <a id="items.WaterfallSound" name="items.WaterfallSound"></a>[lua]`trx.items.WaterfallSound`

    The values the `loop_sound` item property can take. It selects the
    sound a waterfall loops while it runs.

    - `trx.items.WaterfallSound.NONE` = `0`  
        The waterfall runs silently.
    - `trx.items.WaterfallSound.SAND` = `1`  
        A pouring sand loop.
    - `trx.items.WaterfallSound.WATER` = `2`  
        A running water loop.

### Structures

- <a id="items.AnimNum" name="items.AnimNum"></a>[lua]`trx.items.AnimNum`

    The animation's number within the object an item is of. Counted from 0.

- <a id="items.FrameNum" name="items.FrameNum"></a>[lua]`trx.items.FrameNum`

    The frame's number within the animation it belongs to. Counted from 0.

- <a id="items.AnimState" name="items.AnimState"></a>[lua]`trx.items.AnimState`

    An animation state, as the object's own animations number them. What a state means is the object's business: the numbers of a wolf are not the numbers of a door. Counted from 0.

- <a id="items.Num" name="items.Num"></a>[lua]`trx.items.Num`

    Item number, matching the numbers level editors show. Counted from 0.

- <a id="items.Trigger" name="items.Trigger"></a>[lua]`trx.items.Trigger`

    What a trigger carried when it fired.

    Properties:
    - <a id="items.Trigger.mask" name="items.Trigger.mask"></a>**`mask`**: integer. The code bits it set, `1` to `31`.
    - <a id="items.Trigger.one_shot" name="items.Trigger.one_shot"></a>**`one_shot`**: boolean. Whether it fires only the once.
    - <a id="items.Trigger.timer" name="items.Trigger.timer"></a>**`timer`**: [trx.game.Seconds](GAME.md#game.Seconds). How long it keeps the item going.
    - <a id="items.Trigger.type" name="items.Trigger.type"></a>**`type`**: [trx.items.TriggerType](#items.TriggerType). The kind of trigger it was.

- <a id="items.Item" name="items.Item"></a>[lua]`trx.items.Item`

    An item, also known as a moveable.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="items.Item.anim_num" name="items.Item.anim_num"></a>**`anim_num`**: [trx.items.AnimNum](#items.AnimNum).
    - <a id="items.Item.anim_state" name="items.Item.anim_state"></a>**`anim_state`**: [trx.items.AnimState](#items.AnimState). The state the item is in.
    - <a id="items.Item.collidable" name="items.Item.collidable"></a>**`collidable`**: boolean. Whether Lara can collide with this item.
    - <a id="items.Item.fall_speed" name="items.Item.fall_speed"></a>**`fall_speed`**: integer. Vertical speed.
    - <a id="items.Item.frame_num" name="items.Item.frame_num"></a>**`frame_num`**: [trx.items.FrameNum](#items.FrameNum). Negative values count back from the end.
    - <a id="items.Item.goal_anim_state" name="items.Item.goal_anim_state"></a>**`goal_anim_state`**: [trx.items.AnimState](#items.AnimState). The state the item is transitioning towards.
    - <a id="items.Item.gravity" name="items.Item.gravity"></a>**`gravity`**: boolean. Whether gravity applies to this item.
    - <a id="items.Item.hit_points" name="items.Item.hit_points"></a>**`hit_points`**: integer. Current hit points. Raising this above the maximum also raises the `max_hit_points` entry of [`properties`](#items.Item.properties).
    - <a id="items.Item.is_alive" name="items.Item.is_alive"></a>**`is_alive`**: boolean. Whether the item is a living creature with hit points remaining. *(read-only)*
    - <a id="items.Item.is_ally" name="items.Item.is_ally"></a>**`is_ally`**: boolean. Whether this item is a creature that fights on Lara's side. An ally is shown in its own colour where an enemy would be. *(read-only)*
    - <a id="items.Item.is_finished" name="items.Item.is_finished"></a>**`is_finished`**: boolean. Whether the item has finished its run - a creature that died, or a one-shot trigger that fired. It stays in the level but no longer acts.
    - <a id="items.Item.is_hostile" name="items.Item.is_hostile"></a>**`is_hostile`**: boolean. Whether this item is a creature currently hostile to Lara. *(read-only)*
    - <a id="items.Item.is_in_play" name="items.Item.is_in_play"></a>**`is_in_play`**: boolean. Whether the item is live: simulated, visible and not finished - the state a targetable enemy is in. A read-only composite of the axes. *(read-only)*
    - <a id="items.Item.is_killed" name="items.Item.is_killed"></a>**`is_killed`**: boolean. Whether the item has already been killed. *(read-only)*
    - <a id="items.Item.is_one_shot" name="items.Item.is_one_shot"></a>**`is_one_shot`**: boolean. Whether the item's trigger has been spent and will never fire again.
    - <a id="items.Item.is_present" name="items.Item.is_present"></a>**`is_present`**: boolean. Whether the item is in the world at all: linked in its room, so drawn and collidable in principle. Managed by the engine. *(read-only)*
    - <a id="items.Item.is_reversed" name="items.Item.is_reversed"></a>**`is_reversed`**: boolean. Whether the item's trigger is inverted, so it runs until triggered rather than once triggered. This is how a level ships something already on.
    - <a id="items.Item.is_simulated" name="items.Item.is_simulated"></a>**`is_simulated`**: boolean. Whether the item's control routine runs each frame. Call [`activate`](#items.Item.activate) to start it. *(read-only)*
    - <a id="items.Item.is_targetable" name="items.Item.is_targetable"></a>**`is_targetable`**: boolean. Whether Lara's auto-aim can lock onto the item right now. *(read-only)*
    - <a id="items.Item.is_triggered" name="items.Item.is_triggered"></a>**`is_triggered`**: boolean. Whether the item's trigger currently says go. This is what a door, a switch or an alarm reads to decide whether to act; a creature ignores it and goes by whether it is running.
      It is a verdict on [`trigger_mask`](#items.Item.trigger_mask), [`timer`](#items.Item.timer) and [`is_reversed`](#items.Item.is_reversed) together, not a field of its own. *(read-only)*
    - <a id="items.Item.is_visible" name="items.Item.is_visible"></a>**`is_visible`**: boolean. Whether the item is drawn. It can be present in the world but not visible, like an ambush enemy waiting to appear.
    - <a id="items.Item.max_hit_points" name="items.Item.max_hit_points"></a>**`max_hit_points`**: integer. Maximum hit points. Set the `max_hit_points` entry of [`properties`](#items.Item.properties) to change it. *(read-only)*
    - <a id="items.Item.mesh_bits" name="items.Item.mesh_bits"></a>**`mesh_bits`**: integer. Bitmask of which of the item's meshes are drawn.
    - <a id="items.Item.name" name="items.Item.name"></a>**`name`**: string. Unique item name, or `nil`. Assigning a name already in use raises an error.
    - <a id="items.Item.num" name="items.Item.num"></a>**`num`**: [trx.items.Num](#items.Num). An item handed over by a query can say where it lives. *(read-only)*
    - <a id="items.Item.object_id" name="items.Item.object_id"></a>**`object_id`**: [trx.catalog.objects](CATALOG.md#catalog.objects). The item's object type. *(read-only)*
    - <a id="items.Item.pos" name="items.Item.pos"></a>**`pos`**: [trx.math.Vec3](MATH.md#math.Vec3). World position. Updating this also updates [`room`](#items.Item.room) and [`room_num`](#items.Item.room_num).
    - <a id="items.Item.room_num" name="items.Item.room_num"></a>**`room_num`**: [trx.rooms.Num](ROOMS.md#rooms.Num). The room containing this item. Set [`pos`](#items.Item.pos) to move the item between rooms. *(read-only)*
    - <a id="items.Item.rot" name="items.Item.rot"></a>**`rot`**: [trx.math.Rot](MATH.md#math.Rot). Orientation.
    - <a id="items.Item.speed" name="items.Item.speed"></a>**`speed`**: integer. Forward speed.
    - <a id="items.Item.timer" name="items.Item.timer"></a>**`timer`**: [trx.game.Frames](GAME.md#game.Frames). How long the item's trigger keeps it going. `0` runs it until something takes the trigger back; `-1` means it has run out; anything else counts down. [`trigger`](#items.Item.trigger) takes its own timer as a [`trx.game.Seconds`](GAME.md#game.Seconds).
    - <a id="items.Item.touch_bits" name="items.Item.touch_bits"></a>**`touch_bits`**: integer. Bitmask of which of the item's meshes Lara is touching. *(read-only)*
    - <a id="items.Item.trigger_mask" name="items.Item.trigger_mask"></a>**`trigger_mask`**: integer. The five code bits, counted the way a level editor counts them: `1` to `31`. The trigger only says go once every bit is set, which is how a level makes several triggers agree before anything happens. A lone trigger carries all of them.
    - <a id="items.Item.was_hit" name="items.Item.was_hit"></a>**`was_hit`**: boolean. Whether the item was hit during the current frame. *(read-only)*

    Computed properties (derived, not stored on the object):
    - <a id="items.Item.bounds" name="items.Item.bounds"></a>**`bounds`**: [trx.math.Box](MATH.md#math.Box). The item's bounding box for the frame it is on. The numbers are in the item's own frame, so they say how far the model reaches around [`pos`](#items.Item.pos) before [`rot`](#items.Item.rot) turns it, and they change as the item animates.
    - <a id="items.Item.properties" name="items.Item.properties"></a>**`properties`**: table. Typed, object-specific item properties. Writing here overrides the object's default for this item only; reads fall back to the object. Iterable with `pairs()`. See [Objects](../../OBJECTS.md).
    - <a id="items.Item.room" name="items.Item.room"></a>**`room`**: [trx.rooms.Room](ROOMS.md#rooms.Room). The room containing this item.

    Methods:

    - <a id="items.Item.activate" name="items.Item.activate"></a>[lua]`item:activate()`  
      Brings the item to life, exactly as tripping a trigger on it would: its control routine starts running, and a creature also gets its AI, without which it would stand there and ignore Lara.

      Objects with no control routine cannot be activated, and an item that is already active is left alone.

    - <a id="items.Item.deactivate" name="items.Item.deactivate"></a>[lua]`item:deactivate()`  
      Stops the item: its control routine no longer runs, and a creature loses its AI and stands down. The item stays where it is and keeps its hit points, so this is not a way of getting rid of it - use [`destroy`](#items.Item.destroy) for that.

      A trigger can still bring it back, and so can [`activate`](#items.Item.activate).

    - <a id="items.Item.destroy" name="items.Item.destroy"></a>[lua]`item:destroy()`  
      Removes the item from the game. Any other handle to it becomes stale.

    - <a id="items.Item.die" name="items.Item.die"></a>[lua]`item:die([explode])`  
      Runs the object's creature death handling: the corpse stays, and [`explode`](#items.Item.die.explode) bursts its meshes as a rocket or grenade would. For creatures; [`destroy`](#items.Item.destroy) simply removes any item from the game.

      Parameters:
      - <a id="items.Item.die.explode" name="items.Item.die.explode"></a>**`explode`** (boolean, optional, default `false`). Whether to burst the meshes as it dies.

    - <a id="items.Item.distance_to" name="items.Item.distance_to"></a>[lua]`item:distance_to(pos)`  
      Distance from this item to a world position.

      Parameters:
      - <a id="items.Item.distance_to.pos" name="items.Item.distance_to.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.

      Returns: [trx.math.Distance](MATH.md#math.Distance). Measured between the two positions.

    - <a id="items.Item.get_property" name="items.Item.get_property"></a>[lua]`item:get_property(name)`  
      Reads an object property, falling back to the object's default. Prefer `item.properties.<name>`.

      Parameters:
      - <a id="items.Item.get_property.name" name="items.Item.get_property.name"></a>**`name`** (string). Which property, as the object declares it.

      Returns: any or `nil`. The value, of the type the property is declared with.

    - <a id="items.Item.get_property_names" name="items.Item.get_property_names"></a>[lua]`item:get_property_names()`  
      Names of every property this item's object declares.

      Returns: a list of string.

    - <a id="items.Item.is_valid" name="items.Item.is_valid"></a>[lua]`item:is_valid()`  
      Whether the handle still refers to a live item. Reading or writing a field on a stale handle raises an error rather than silently operating on an unrelated item, so check this for a handle held across time.

      Returns: boolean. False once the item it named is gone.

      Example:
      ```lua
      local wolf = trx.items.query:of_object(trx.catalog.objects.wolf):first()
      trx.events.after_control(function()
        if wolf:is_valid() and wolf.hit_points <= 0 then
          trx.log.info("the wolf is down")
        end
      end)
      ```

    - <a id="items.Item.on_activate" name="items.Item.on_activate"></a>[lua]`item:on_activate(callback)`  
      Happens when this item is activated through the lifecycle front door during play. [`trx.events.on_activate`](EVENTS.md#events.on_activate), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_activate.callback" name="items.Item.on_activate.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_activate.item" name="items.Item.on_activate.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_activate(function(item)
        trx.log.info("the item was activated")
      end)
      ```

    - <a id="items.Item.on_deactivate" name="items.Item.on_deactivate"></a>[lua]`item:on_deactivate(callback)`  
      Happens when this item is deactivated through the lifecycle front door during play. [`trx.events.on_deactivate`](EVENTS.md#events.on_deactivate), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_deactivate.callback" name="items.Item.on_deactivate.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_deactivate.item" name="items.Item.on_deactivate.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_deactivate(function(item)
        trx.log.info("the item was deactivated")
      end)
      ```

    - <a id="items.Item.on_destroy" name="items.Item.on_destroy"></a>[lua]`item:on_destroy(callback)`  
      Happens as this item is removed from the game during play. It can still be read from the handler, but not after. [`trx.events.on_destroy`](EVENTS.md#events.on_destroy), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_destroy.callback" name="items.Item.on_destroy.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_destroy.item" name="items.Item.on_destroy.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_destroy(function(item)
        trx.log.info("the item was removed")
      end)
      ```

    - <a id="items.Item.on_enter_sim" name="items.Item.on_enter_sim"></a>[lua]`item:on_enter_sim(callback)`  
      Happens when this item starts being simulated during play. [`trx.events.on_enter_sim`](EVENTS.md#events.on_enter_sim), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_enter_sim.callback" name="items.Item.on_enter_sim.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_enter_sim.item" name="items.Item.on_enter_sim.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_enter_sim(function(item)
        trx.log.info("the item started running")
      end)
      ```

    - <a id="items.Item.on_enter_world" name="items.Item.on_enter_world"></a>[lua]`item:on_enter_world(callback)`  
      Happens when this item enters the world during play, such as a runtime spawn. [`trx.events.on_enter_world`](EVENTS.md#events.on_enter_world), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_enter_world.callback" name="items.Item.on_enter_world.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_enter_world.item" name="items.Item.on_enter_world.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_enter_world(function(item)
        trx.log.info("the item entered the world")
      end)
      ```

    - <a id="items.Item.on_finish" name="items.Item.on_finish"></a>[lua]`item:on_finish(callback)`  
      Happens when this item finishes its run during play. [`trx.events.on_finish`](EVENTS.md#events.on_finish), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_finish.callback" name="items.Item.on_finish.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_finish.item" name="items.Item.on_finish.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_finish(function(item)
        trx.log.info("the item finished its run")
      end)
      ```

    - <a id="items.Item.on_hide" name="items.Item.on_hide"></a>[lua]`item:on_hide(callback)`  
      Happens when this item becomes hidden during play. [`trx.events.on_hide`](EVENTS.md#events.on_hide), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_hide.callback" name="items.Item.on_hide.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_hide.item" name="items.Item.on_hide.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_hide(function(item)
        trx.log.info("the item vanished")
      end)
      ```

    - <a id="items.Item.on_hit" name="items.Item.on_hit"></a>[lua]`item:on_hit(callback)`  
      Happens when this item takes damage. [`trx.events.on_hit`](EVENTS.md#events.on_hit), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_hit.callback" name="items.Item.on_hit.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_hit.item" name="items.Item.on_hit.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.
        - <a id="items.Item.on_hit.damage" name="items.Item.on_hit.damage"></a>**`damage`** (integer). Hit points taken, before clamping to zero.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_hit(function(item, damage)
        trx.log.info("the item lost " .. damage .. " hit points")
      end)
      ```

    - <a id="items.Item.on_kill" name="items.Item.on_kill"></a>[lua]`item:on_kill(callback)`  
      Happens when damage takes this item's hit points to zero. [`trx.events.on_kill`](EVENTS.md#events.on_kill), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_kill.callback" name="items.Item.on_kill.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_kill.item" name="items.Item.on_kill.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_kill(function(item)
        trx.log.info("the item is down")
      end)
      ```

    - <a id="items.Item.on_leave_sim" name="items.Item.on_leave_sim"></a>[lua]`item:on_leave_sim(callback)`  
      Happens when this item stops being simulated during play. [`trx.events.on_leave_sim`](EVENTS.md#events.on_leave_sim), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_leave_sim.callback" name="items.Item.on_leave_sim.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_leave_sim.item" name="items.Item.on_leave_sim.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_leave_sim(function(item)
        trx.log.info("the item stopped running")
      end)
      ```

    - <a id="items.Item.on_leave_world" name="items.Item.on_leave_world"></a>[lua]`item:on_leave_world(callback)`  
      Happens when this item leaves the world during play. [`trx.events.on_leave_world`](EVENTS.md#events.on_leave_world), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_leave_world.callback" name="items.Item.on_leave_world.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_leave_world.item" name="items.Item.on_leave_world.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_leave_world(function(item)
        trx.log.info("the item left the world")
      end)
      ```

    - <a id="items.Item.on_show" name="items.Item.on_show"></a>[lua]`item:on_show(callback)`  
      Happens when this item becomes visible during play. [`trx.events.on_show`](EVENTS.md#events.on_show), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_show.callback" name="items.Item.on_show.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_show.item" name="items.Item.on_show.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_show(function(item)
        trx.log.info("the item appeared")
      end)
      ```

    - <a id="items.Item.on_trigger" name="items.Item.on_trigger"></a>[lua]`item:on_trigger(callback)`  
      Happens every time a trigger is aimed at this item, of any kind. [`trx.events.on_trigger`](EVENTS.md#events.on_trigger), narrowed to this item.

      Parameters:
      - <a id="items.Item.on_trigger.callback" name="items.Item.on_trigger.callback"></a>**`callback`** (function). What to run when it happens to this item.
        Called with:
        - <a id="items.Item.on_trigger.item" name="items.Item.on_trigger.item"></a>**`item`** ([trx.items.Item](#items.Item)). This item.
        - <a id="items.Item.on_trigger.trigger" name="items.Item.on_trigger.trigger"></a>**`trigger`** ([trx.items.Trigger](#items.Trigger)). What the trigger carried.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.items[12]:on_trigger(function(item, trigger)
        trx.log.info("triggered with mask " .. trigger.mask)
      end)
      ```

    - <a id="items.Item.set_property" name="items.Item.set_property"></a>[lua]`item:set_property(name, value)`  
      Overrides an object property for this item. Prefer `item.properties.<name> = ...`.

      Parameters:
      - <a id="items.Item.set_property.name" name="items.Item.set_property.name"></a>**`name`** (string). Which property, as the object declares it.
      - <a id="items.Item.set_property.value" name="items.Item.set_property.value"></a>**`value`** (any). What to write, of the type the property is declared with.

    - <a id="items.Item.shatter" name="items.Item.shatter"></a>[lua]`item:shatter([damage])`  
      Bursts the item's meshes into flying debris, the visual [`die`](#items.Item.die) produces with [`die.explode`](#items.Item.die.explode), on its own. It does not kill or remove the item.

      Parameters:
      - <a id="items.Item.shatter.damage" name="items.Item.shatter.damage"></a>**`damage`** (integer, optional, default `0`). Splash damage dealt to nearby items.

    - <a id="items.Item.take_damage" name="items.Item.take_damage"></a>[lua]`item:take_damage(damage)`  
      Hurts the item the way a weapon does, and reports through
      [`trx.events.on_hit`](EVENTS.md#events.on_hit), and [`trx.events.on_kill`](EVENTS.md#events.on_kill) where the blow takes the
      last hit point. Writing [`hit_points`](#items.Item.hit_points) reports neither.
      The kill counts as the environment's rather than Lara's.

      Parameters:
      - <a id="items.Item.take_damage.damage" name="items.Item.take_damage.damage"></a>**`damage`** (integer). Hit points to take.

      Example:
      ```lua
      local lara = trx.lara.item
      lara:take_damage(lara.hit_points)
      ```

    - <a id="items.Item.trigger" name="items.Item.trigger"></a>[lua]`item:trigger([opts])`  
      Fires a trigger at the item, exactly as a floor trigger in the level would: sets the code bits, and once they are all set, starts the item running.

      This is the one to reach for on anything a level would trigger - a door, a switch, an alarm - because those read their trigger before they act, and merely activating one leaves it running but doing nothing. Pass `type = trx.items.TriggerType.ANTITRIGGER` to take the trigger back instead.

      Parameters:
      - <a id="items.Item.trigger.opts" name="items.Item.trigger.opts"></a>**`opts`** (table, optional). What the trigger carries.

        Keys:
        - <a id="items.Item.trigger.opts.type" name="items.Item.trigger.opts.type"></a>**`type`** ([trx.items.TriggerType](#items.TriggerType), optional). A plain `TRIGGER` by default.
        - <a id="items.Item.trigger.opts.mask" name="items.Item.trigger.opts.mask"></a>**`mask`** (integer, optional). Which of the five code bits to set, `1` to `31`, all of them by default. Pass fewer to act as one of several triggers a puzzle is waiting on.
        - <a id="items.Item.trigger.opts.timer" name="items.Item.trigger.opts.timer"></a>**`timer`** ([trx.game.Seconds](GAME.md#game.Seconds), optional, default `0`). How long it should keep the item going. `0` means until something takes the trigger back. A timer of exactly `1` is a single frame, not a second, matching the level format.
        - <a id="items.Item.trigger.opts.one_shot" name="items.Item.trigger.opts.one_shot"></a>**`one_shot`** (boolean, optional). Never let it fire again.

      Example:
      ```lua
      trx.items[12]:trigger()
      ```

      Example:
      ```lua
      trx.items[12]:trigger({ timer = 3, one_shot = true })
      ```

      Example:
      ```lua
      trx.items[12]:trigger({ type = trx.items.TriggerType.ANTITRIGGER })
      ```

- <a id="items.ItemQuery" name="items.ItemQuery"></a>[lua]`trx.items.ItemQuery`

    A [`trx.query.Query`](QUERY.md#query.Query) over the items a level holds, with the narrowings below on top of the ones every query has. Items answer to no names of their own, so [`of_object`](#items.ItemQuery.of_object) is how a name reaches them.

    Methods:

    - <a id="items.ItemQuery.alive" name="items.ItemQuery.alive"></a>[lua]`itemquery:alive()`  
      The item still has hit points.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="items.ItemQuery.finished" name="items.ItemQuery.finished"></a>[lua]`itemquery:finished()`  
      The item has run its course.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="items.ItemQuery.in_box" name="items.ItemQuery.in_box"></a>[lua]`itemquery:in_box(min, max)`  
      The item stands inside a world-space box. The corners may come in any order.

      An item is tested by its position, the point it stands at, rather than by the box it fills. Position is all this asks after, so the rest of the query says what else the item must be: `trx.items.query:in_box(min, max):present()` asks for the ones that are in the world as well.

      Parameters:
      - <a id="items.ItemQuery.in_box.min" name="items.ItemQuery.in_box.min"></a>**`min`** ([trx.math.Vec3](MATH.md#math.Vec3)). One corner of the box.
      - <a id="items.ItemQuery.in_box.max" name="items.ItemQuery.in_box.max"></a>**`max`** ([trx.math.Vec3](MATH.md#math.Vec3)). The opposite corner.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

      Example:
      ```lua
      local guards = trx.items.query
        :in_box({ x = 51200, y = -2048, z = 30720 }, { x = 53248, y = 0, z = 32768 })
        :present()
        :matches()
      ```

    - <a id="items.ItemQuery.in_play" name="items.ItemQuery.in_play"></a>[lua]`itemquery:in_play()`  
      The item is part of the game rather than set aside.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="items.ItemQuery.in_room" name="items.ItemQuery.in_room"></a>[lua]`itemquery:in_room(room_num)`  
      The item is in the given room.

      Parameters:
      - <a id="items.ItemQuery.in_room.room_num" name="items.ItemQuery.in_room.room_num"></a>**`room_num`** ([trx.rooms.Num](ROOMS.md#rooms.Num)).

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="items.ItemQuery.in_sphere" name="items.ItemQuery.in_sphere"></a>[lua]`itemquery:in_sphere(centre, radius)`  
      The item stands within a radius of a point. As with [`in_box`](#items.ItemQuery.in_box), the item's position is the whole of the test.

      Parameters:
      - <a id="items.ItemQuery.in_sphere.centre" name="items.ItemQuery.in_sphere.centre"></a>**`centre`** ([trx.math.Vec3](MATH.md#math.Vec3)). Middle of the sphere.
      - <a id="items.ItemQuery.in_sphere.radius" name="items.ItemQuery.in_sphere.radius"></a>**`radius`** ([trx.math.Distance](MATH.md#math.Distance)). How far out it reaches.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="items.ItemQuery.of_object" name="items.ItemQuery.of_object"></a>[lua]`itemquery:of_object(key)`  
      The item is of the given object, named the way a player would name it or by its id.

      Parameters:
      - <a id="items.ItemQuery.of_object.key" name="items.ItemQuery.of_object.key"></a>**`key`** (any). Object id, or a name [`trx.objects.query`](OBJECTS.md#objects.query) resolves.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

      Example:
      ```lua
      trx.items.query:of_object("wolf"):simulated():matches()
      ```

    - <a id="items.ItemQuery.present" name="items.ItemQuery.present"></a>[lua]`itemquery:present()`  
      The item is in the world, whether or not anything is simulating it.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="items.ItemQuery.simulated" name="items.ItemQuery.simulated"></a>[lua]`itemquery:simulated()`  
      The item is being simulated: its control routine runs every frame.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="items.ItemQuery.targetable" name="items.ItemQuery.targetable"></a>[lua]`itemquery:targetable()`  
      Lara's guns can lock onto the item.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="items.ItemQuery.visible" name="items.ItemQuery.visible"></a>[lua]`itemquery:visible()`  
      The item is drawn.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

### Functions

- <a id="items.get" name="items.get"></a>[lua]`trx.items.get(key)`  
  Retrieves an item by number or by name.

  Parameters:
  - <a id="items.get.key" name="items.get.key"></a>**`key`** ([trx.items.Num](#items.Num)). An item's unique name reaches it as well.

  Returns: [trx.items.Item](#items.Item) or `nil`. The item, or `nil` where nothing answers to the key.

  Example:
  ```lua
  local item = trx.items[0]
  item.name = "lara"
  local lara = trx.items["lara"]
  ```

- <a id="items.spawn" name="items.spawn"></a>[lua]`trx.items.spawn(object_id, pos, [angle_y], [opts])`  
  Creates a new item of the given object type at the given position.

  Parameters:
  - <a id="items.spawn.object_id" name="items.spawn.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). Object type to spawn.
  - <a id="items.spawn.pos" name="items.spawn.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
  - <a id="items.spawn.angle_y" name="items.spawn.angle_y"></a>**`angle_y`** ([trx.math.Angle](MATH.md#math.Angle), optional, default `0`). Facing angle.
  - <a id="items.spawn.opts" name="items.spawn.opts"></a>**`opts`** (table, optional). How to spawn it.

    Keys:
    - <a id="items.spawn.opts.activate" name="items.spawn.opts.activate"></a>**`activate`** (boolean, optional). Bring the item to life, enabling AI for creatures.

  Returns: [trx.items.Item](#items.Item) or `nil`. `nil` if the item pool is exhausted.

  Example:
  ```lua
  local wolf = trx.items.spawn(
    trx.catalog.objects.wolf, trx.lara.item.pos, 0, { activate = true })
  ```

- <a id="items.count" name="items.count"></a>[lua]`trx.items.count()`  
  Returns the total number of allocated items. Same as `#trx.items`.

  Returns: integer. How many slots the level holds, live or not.
