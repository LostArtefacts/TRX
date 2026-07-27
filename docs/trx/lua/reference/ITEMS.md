---
title: Items
order: 4
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/items.lua. Edit it there.
-->

## Items module

Module for controlling all moveables.

### Indexing

Indexing the module reaches an item, and `#trx.items` is how many the level has. Items count from zero, matching the item numbers level editors show. `pairs()` walks them in order, keyed by that number.

- **`trx.items[key]`** (Item or `nil`). 0-based index, or the item's unique name.
- **`#trx.items`** (integer). How many there are.

Example:
```lua
for num, item in pairs(trx.items) do
  trx.log.info(item.object_id)
end
```

### Properties

- **`trx.items.query`** (table). The identity query over every item in the level. Narrow it and read it - see [Query](../../QUERY.md).

  Its own narrowings, beyond the operators: `simulated`, `present`, `visible`, `finished`, `in_play`, `alive`, `targetable`, `of_object` (by object id or name) and `in_room`.

  Example: `trx.items.query:of_object("wolf"):simulated():matches()`. *(read-only)*

### Enums

- [lua]`trx.items.PickupMode`

    The values the `pickup_mode` item property can take. It selects the animation Lara plays when collecting the item.

    - `trx.items.PickupMode.NORMAL` = `0`  
        Picked up off the floor.
    - `trx.items.PickupMode.PLINTH_LOW` = `1`  
        Picked up from a low pedestal.
    - `trx.items.PickupMode.PLINTH_HIGH` = `2`  
        Picked up from a high pedestal.

- [lua]`trx.items.TriggerType`

    The kind of trigger `item:trigger` fires, matching the trigger types a level editor offers. Most are forward triggers that differ only in what trips them in a level; from a script they behave alike, and `TRIGGER` is the one to reach for.

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

### Structures

- [lua]`trx.items.Item`

    An item, also known as a moveable.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`anim`**: integer. Object-relative animation number, 0-indexed.
    - **`anim_state`**: integer. Current animation state.
    - **`collidable`**: boolean. Whether Lara can collide with this item.
    - **`fall_speed`**: integer. Vertical speed.
    - **`frame`**: integer. Object-relative frame number, 0-indexed. Negative values count back from the end.
    - **`goal_anim_state`**: integer. Animation state the item is transitioning towards.
    - **`gravity`**: boolean. Whether gravity applies to this item.
    - **`hit_points`**: integer. Current hit points. Raising this above the maximum also raises `properties.max_hit_points`.
    - **`index`**: integer. The index `trx.items[i]` takes, counted from 0. An item handed over by a query can say where it lives. *(read-only)*
    - **`is_alive`**: boolean. Whether the item is a living creature with hit points remaining. *(read-only)*
    - **`is_finished`**: boolean. Whether the item has finished its run - a creature that died, or a one-shot trigger that fired. It stays in the level but no longer acts.
    - **`is_hostile`**: boolean. Whether this item is a creature currently hostile to Lara. *(read-only)*
    - **`is_in_play`**: boolean. Whether the item is live: simulated, visible and not finished - the state a targetable enemy is in. A read-only composite of the axes. *(read-only)*
    - **`is_killed`**: boolean. Whether the item has already been killed. *(read-only)*
    - **`is_one_shot`**: boolean. Whether the item's trigger has been spent and will never fire again.
    - **`is_present`**: boolean. Whether the item is in the world at all: linked in its room, so drawn and collidable in principle. Managed by the engine. *(read-only)*
    - **`is_reversed`**: boolean. Whether the item's trigger is inverted, so it runs until triggered rather than once triggered. This is how a level ships something already on.
    - **`is_simulated`**: boolean. Whether the item's control routine runs each frame. Call `activate()` to start it. *(read-only)*
    - **`is_targetable`**: boolean. Whether Lara's auto-aim can lock onto the item right now. *(read-only)*
    - **`is_triggered`**: boolean. Whether the item's trigger currently says go. This is what a door, a switch or an alarm reads to decide whether to act; a creature ignores it and goes by whether it is running.

      It is a verdict on `trigger_mask`, `timer` and `is_reversed` together, not a field of its own. *(read-only)*
    - **`is_visible`**: boolean. Whether the item is drawn. It can be present in the world but not visible, like an ambush enemy waiting to appear.
    - **`max_hit_points`**: integer. Maximum hit points. Set `properties.max_hit_points` to change it. *(read-only)*
    - **`mesh_bits`**: integer. Bitmask of which of the item's meshes are drawn.
    - **`name`**: string. Unique item name, or `nil`. Assigning a name already in use raises an error.
    - **`object_id`**: integer. The item's object type. Compare against `trx.catalog.objects`. *(read-only)*
    - **`pos`**: vec3. World position. Updating this also updates `room` and `room_num`.
    - **`room_num`**: integer. 0-based number of the room containing this item. Set `pos` to move the item between rooms. *(read-only)*
    - **`rot`**: vec3. Orientation, in the units `trx.math` counts angles in. An angle counts in cycles, so one past the end of the turn wraps round to name the same direction rather than raising: adding a half turn to a rotation always works.
    - **`speed`**: integer. Forward speed.
    - **`timer`**: integer. How long the item's trigger keeps it going, in game frames. `0` runs it until something takes the trigger back; `-1` means it has run out; anything else counts down. This is the raw frame count - `trigger()` takes its timer in seconds instead.
    - **`touch_bits`**: integer. Bitmask of which of the item's meshes Lara is touching. *(read-only)*
    - **`trigger_mask`**: integer. The five code bits, counted the way a level editor counts them: `1` to `31`. The trigger only says go once every bit is set, which is how a level makes several triggers agree before anything happens. A lone trigger carries all of them.
    - **`was_hit`**: boolean. Whether the item was hit during the current frame. *(read-only)*

    Computed properties (derived, not stored on the object):
    - **`properties`**: table. Typed, object-specific item properties. Writing here overrides the object's default for this item only; reads fall back to the object. Iterable with `pairs()`. See [Objects](../../OBJECTS.md).
    - **`room`**: Room. The `trx.rooms.Room` containing this item.

    Methods:

    - [lua]`item:activate()`  
      Brings the item to life, exactly as tripping a trigger on it would: its control routine starts running, and a creature also gets its AI, without which it would stand there and ignore Lara.

      Objects with no control routine cannot be activated, and an item that is already active is left alone.

    - [lua]`item:deactivate()`  
      Stops the item: its control routine no longer runs, and a creature loses its AI and stands down. The item stays where it is and keeps its hit points, so this is not a way of getting rid of it - use `destroy()` for that.

      A trigger can still bring it back, and so can `activate()`.

    - [lua]`item:destroy()`  
      Removes the item from the game. Any other handle to it becomes stale.

    - [lua]`item:die([explode])`  
      Runs the object's creature death handling: the corpse stays, and `explode` bursts its meshes as a rocket or grenade would. For creatures; `destroy()` simply removes any item from the game.

      Parameters:
      - **`explode`** (boolean, optional, default `False`). Whether to burst the meshes as it dies.

    - [lua]`item:distance_to(pos)`  
      Distance from this item to a world position.

      Parameters:
      - **`pos`** (vec3). World position.

      Returns: integer.

    - [lua]`item:get_property(name)`  
      Reads an object property, falling back to the object's default. Prefer `item.properties.<name>`.

      Parameters:
      - **`name`** (string).

      Returns: any or `nil`.

    - [lua]`item:get_property_names()`  
      Names of every property this item's object declares.

      Returns: table.

    - [lua]`item:is_valid()`  
      Whether the handle still refers to a live item. Reading or writing a field on a stale handle raises an error rather than silently operating on an unrelated item, so check this for a handle held across time.

      Returns: boolean.

      Example:
      ```lua
      local wolf = trx.items.query:of_object(trx.catalog.objects.wolf):first()
      trx.events.after_control(function()
        if wolf:is_valid() and wolf.hit_points <= 0 then
          trx.log.info("the wolf is down")
        end
      end)
      ```

    - [lua]`item:on_activate(callback)`  
      Happens when this item is activated through the lifecycle front door during play. `trx.events.on_activate`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_activate(function(item)
        trx.log.info("the item was activated")
      end)
      ```

    - [lua]`item:on_deactivate(callback)`  
      Happens when this item is deactivated through the lifecycle front door during play. `trx.events.on_deactivate`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_deactivate(function(item)
        trx.log.info("the item was deactivated")
      end)
      ```

    - [lua]`item:on_destroy(callback)`  
      Happens as this item is removed from the game during play. It can still be read from the handler, but not after. `trx.events.on_destroy`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_destroy(function(item)
        trx.log.info("the item was removed")
      end)
      ```

    - [lua]`item:on_enter_sim(callback)`  
      Happens when this item starts being simulated during play. `trx.events.on_enter_sim`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_enter_sim(function(item)
        trx.log.info("the item started running")
      end)
      ```

    - [lua]`item:on_enter_world(callback)`  
      Happens when this item enters the world during play, such as a runtime spawn. `trx.events.on_enter_world`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_enter_world(function(item)
        trx.log.info("the item entered the world")
      end)
      ```

    - [lua]`item:on_finish(callback)`  
      Happens when this item finishes its run during play. `trx.events.on_finish`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_finish(function(item)
        trx.log.info("the item finished its run")
      end)
      ```

    - [lua]`item:on_hide(callback)`  
      Happens when this item becomes hidden during play. `trx.events.on_hide`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_hide(function(item)
        trx.log.info("the item vanished")
      end)
      ```

    - [lua]`item:on_hit(callback)`  
      Happens when this item takes damage. `trx.events.on_hit`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.
        - **`damage`** (integer). Hit points taken, before clamping to zero.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_hit(function(item, damage)
        trx.log.info("the item lost " .. damage .. " hit points")
      end)
      ```

    - [lua]`item:on_kill(callback)`  
      Happens when damage takes this item's hit points to zero. `trx.events.on_kill`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_kill(function(item)
        trx.log.info("the item is down")
      end)
      ```

    - [lua]`item:on_leave_sim(callback)`  
      Happens when this item stops being simulated during play. `trx.events.on_leave_sim`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_leave_sim(function(item)
        trx.log.info("the item stopped running")
      end)
      ```

    - [lua]`item:on_leave_world(callback)`  
      Happens when this item leaves the world during play. `trx.events.on_leave_world`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_leave_world(function(item)
        trx.log.info("the item left the world")
      end)
      ```

    - [lua]`item:on_show(callback)`  
      Happens when this item becomes visible during play. `trx.events.on_show`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_show(function(item)
        trx.log.info("the item appeared")
      end)
      ```

    - [lua]`item:on_trigger(callback)`  
      Happens every time a trigger is aimed at this item, of any kind. `trx.events.on_trigger`, narrowed to this item.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). This item.
        - **`trigger`** (table). What the trigger carried: `type`, `mask`, `timer` and `one_shot`. See `trx.events.on_trigger`.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.items[12]:on_trigger(function(item, trigger)
        trx.log.info("triggered with mask " .. trigger.mask)
      end)
      ```

    - [lua]`item:set_property(name, value)`  
      Overrides an object property for this item. Prefer `item.properties.<name> = ...`.

      Parameters:
      - **`name`** (string).
      - **`value`** (any).

    - [lua]`item:shatter([damage])`  
      Bursts the item's meshes into flying debris, the visual `die(true)` produces, on its own. It does not kill or remove the item.

      Parameters:
      - **`damage`** (integer, optional, default `0`). Splash damage dealt to nearby items.

    - [lua]`item:take_damage(damage)`  
      Hurts the item the way a weapon does, and reports through `on_hit`, and
      `on_kill` where the blow takes the last hit point. Writing `hit_points` reports neither. The kill
      counts as the environment's rather than Lara's.

      Parameters:
      - **`damage`** (integer). Hit points to take.

      Example:
      ```lua
      local lara = trx.lara.item
      lara:take_damage(lara.hit_points)
      ```

    - [lua]`item:trigger([opts])`  
      Fires a trigger at the item, exactly as a floor trigger in the level would: sets the code bits, and once they are all set, starts the item running.

      This is the one to reach for on anything a level would trigger - a door, a switch, an alarm - because those read their trigger before they act, and merely `activate()`-ing one leaves it running but doing nothing. Pass `type = trx.items.TriggerType.ANTITRIGGER` to take the trigger back instead.

      Parameters:
      - **`opts`** (table, optional). `type`: which `items.TriggerType` to fire; a plain `TRIGGER` by default.

        `mask`: which of the five code bits to set, `1` to `31`, all of them by default. Pass fewer to act as one of several triggers a puzzle is waiting on.

        `timer`: how long it should keep the item going, in seconds. `0`, the default, means until something takes the trigger back. A timer of exactly `1` is a single frame, not a second, matching the level format.

        `one_shot`: never let it fire again.

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

### Functions

- [lua]`trx.items.get(key)`  
  Retrieves an item by index or by name. Items count from zero, matching the item numbers level editors show.

  Parameters:
  - **`key`** (any). 0-based index, or the item's unique name.

  Returns: Item or `nil`.

  Example:
  ```lua
  local item = trx.items[0]
  item.name = "lara"
  local lara = trx.items["lara"]
  ```

- [lua]`trx.items.spawn(object_id, pos, [angle_y], [opts])`  
  Creates a new item of the given object type at the given position.

  Parameters:
  - **`object_id`** (integer). Object type to spawn. Compare against `trx.catalog.objects`.
  - **`pos`** (vec3). World position. Must lie inside the level.
  - **`angle_y`** (integer, optional, default `0`). Facing angle.
  - **`opts`** (table, optional). `activate`: bring the item to life, enabling AI for creatures.

  Returns: Item or `nil`. `nil` if the item pool is exhausted.

  Example:
  ```lua
  local wolf = trx.items.spawn(
    trx.catalog.objects.wolf, trx.lara.item.pos, 0, { activate = true })
  ```

- [lua]`trx.items.count()`  
  Returns the total number of allocated items. Same as `#trx.items`.

  Returns: integer.
