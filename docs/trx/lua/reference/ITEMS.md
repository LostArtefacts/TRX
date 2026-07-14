---
title: Items
order: 4
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  data/scripting/items.lua. Edit it there.
-->

## Items module

Module for controlling all moveables.

### Enums

- [lua]`trx.items.Status`

    The values `item.status` can take.

    - `trx.items.Status.INACTIVE` = `0`  
        In the level, but not yet triggered. Its control routine does not run.
    - `trx.items.Status.ACTIVE` = `1`  
        Triggered: its control routine runs every frame.
    - `trx.items.Status.DEACTIVATED` = `2`  
        Ran and finished - a creature that died, or a one-shot trigger that fired. It stays in the level, but no longer runs.
    - `trx.items.Status.INVISIBLE` = `3`  
        Neither drawn nor collidable, as a pickup Lara has already collected is.

- [lua]`trx.items.PickupMode`

    The values the `pickup_mode` item property can take. It selects the animation Lara plays when collecting the item.

    - `trx.items.PickupMode.NORMAL` = `0`  
        Picked up off the floor.
    - `trx.items.PickupMode.PLINTH_LOW` = `1`  
        Picked up from a low pedestal.
    - `trx.items.PickupMode.PLINTH_HIGH` = `2`  
        Picked up from a high pedestal.

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
    - **`flags`**: integer. Trigger-related flag bits. Read-only: writing them directly would let a script set `IF_KILLED` without unlinking the item, wedging engine state. Use `kill()` instead. *(read-only)*
    - **`frame`**: integer. Object-relative frame number, 0-indexed. Negative values count back from the end.
    - **`goal_anim_state`**: integer. Animation state the item is transitioning towards.
    - **`gravity`**: boolean. Whether gravity applies to this item.
    - **`hit_points`**: integer. Current hit points. Raising this above the maximum also raises `properties.max_hit_points`.
    - **`is_active`**: boolean. Whether the item's control routine is running. Call `activate()` to start it. *(read-only)*
    - **`is_alive`**: boolean. Whether the item is a living creature with hit points remaining. *(read-only)*
    - **`is_hostile`**: boolean. Whether this item is a creature currently hostile to Lara. *(read-only)*
    - **`is_killed`**: boolean. Whether the item has already been killed. *(read-only)*
    - **`max_hit_points`**: integer. Maximum hit points. Set `properties.max_hit_points` to change it. *(read-only)*
    - **`mesh_bits`**: integer. Bitmask of which of the item's meshes are drawn.
    - **`name`**: string. Unique item name, or `nil`. Assigning a name already in use raises an error.
    - **`object_id`**: integer. The item's object type. Compare against `trx.catalog.objects`. *(read-only)*
    - **`pos`**: vec3. World position. Updating this also updates `room` and `room_num`.
    - **`room_num`**: integer. 1-based number of the room containing this item. Set `pos` to move the item between rooms. *(read-only)*
    - **`rot`**: vec3. Orientation.
    - **`speed`**: integer. Forward speed.
    - **`status`**: integer. Item status. Use `activate()` and `kill()` to change it, so the item's active-list membership stays in sync. Compare against `trx.items.Status`. *(read-only)*
    - **`timer`**: integer. Trigger-related timer value.
    - **`touch_bits`**: integer. Bitmask of which of the item's meshes Lara is touching. *(read-only)*
    - **`was_hit`**: boolean. Whether the item was hit during the current frame. *(read-only)*

    Computed properties (derived, not stored on the object):
    - **`properties`**: table. Typed, object-specific item properties. Writing here overrides the object's default for this item only; reads fall back to the object. Iterable with `pairs()`. See [Objects](../../OBJECTS.md).
    - **`room`**: Room. The `trx.rooms.Room` containing this item.

    Methods:

    - [lua]`item:activate()`  
      Adds the item to the active list and starts its control routine. Objects with no control routine cannot be activated.

    - [lua]`item:distance_to(pos)`  
      Distance from this item to a world position.

      Parameters:
      - **`pos`** (vec3). World position.

      Returns: integer.

    - [lua]`item:explode()`  
      Runs the object's death handling with an explosion, as a rocket or a grenade would. Unlike `kill()`, which simply removes the item from the game.

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
      local wolf = trx.items.first({ object_id = trx.catalog.objects.wolf })
      trx.events.after_control(function()
        if wolf:is_valid() and wolf.hit_points <= 0 then
          trx.log.info("the wolf is down")
        end
      end)
      ```

    - [lua]`item:kill()`  
      Removes the item from the game. Any other handle to it becomes stale.

    - [lua]`item:set_property(name, value)`  
      Overrides an object property for this item. Prefer `item.properties.<name> = ...`.

      Parameters:
      - **`name`** (string).
      - **`value`** (any).

### Functions

- [lua]`trx.items.get(key)`  
  Retrieves an item by 1-based index or by name.

  Parameters:
  - **`key`** (any). 1-based index, or the item's unique name.

  Returns: Item or `nil`.

  Example:
  ```lua
  local item = trx.items[1]
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

- [lua]`trx.items.find([query])`  
  Finds all items matching the query.

  Parameters:
  - **`query`** (table, optional). Supported keys: `object_id`, `room_num`. Unknown keys are ignored and logged. Omit it for no matches.

  Returns: table. List of `Item`.

  Example:
  ```lua
  local wolves = trx.items.find({ object_id = trx.catalog.objects.wolf })
  ```

- [lua]`trx.items.first([query])`  
  Finds the first item matching the query.

  Parameters:
  - **`query`** (table, optional). Supported keys: `object_id`, `room_num`. Omit it for no match.

  Returns: Item or `nil`.

  Example:
  ```lua
  local natla = trx.items.first({ object_id = trx.catalog.objects.natla })
  ```
