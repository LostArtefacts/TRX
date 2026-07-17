---
title: Rooms
order: 10
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/rooms.lua. Edit it there.
-->

## Rooms module

Module for inspecting and altering the rooms of the current level.

### Indexing

Indexing the module reaches a room, and `#trx.rooms` is how many the level has. Rooms count from zero, matching the room numbers level editors show. `pairs()` walks them in order, keyed by that number.

- **`trx.rooms[key]`** (Room or `nil`). 0-based room number.
- **`#trx.rooms`** (integer). How many there are.

Example:
```lua
trx.log.info(#trx.rooms .. " rooms, first is " .. trx.rooms[0].num)
for num, room in pairs(trx.rooms) do
  room.cold = true
end
```

### Properties

- **`trx.rooms.flipped`** (boolean). Whether the room map is currently flipped. *(read-only)*

### Enums

- [lua]`trx.rooms.FlipStatus`

    The values `room.flip_status` can take.

    - `trx.rooms.FlipStatus.NONE` = `0`  
        This is a normal room.
    - `trx.rooms.FlipStatus.UNFLIPPED` = `1`  
        This room is currently reachable by Lara.
    - `trx.rooms.FlipStatus.FLIPPED` = `2`  
        This room is currently inactive and unreachable by Lara.

### Structures

- [lua]`trx.rooms.Room`

    A room in the current level.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`cold`**: boolean. Whether Lara's breath is visible in the room.
    - **`damaging`**: boolean. Whether the room drains Lara's exposure meter.
    - **`flip_status`**: integer. Current flip status. Compare against `trx.rooms.FlipStatus`. *(read-only)*
    - **`num`**: integer. 0-based room number, matching the numbers level editors show. *(read-only)*
    - **`underwater`**: boolean. Whether the room is filled with water.
    - **`wind`**: boolean. Whether the room has a breeze. Requires the player to have breeze enabled.

    Computed properties (derived, not stored on the object):
    - **`bounds`**: table. World-coordinate bounds of the room: `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`.
    - **`flipped_room`**: Room. This room's flip pair, or `nil` if it has none.
    - **`internal_bounds`**: table. As `bounds`, but excluding the outer ring of sectors, which is solid wall.

    Methods:

    - [lua]`room:is_valid()`  
      Whether the handle still refers to a room of the level that is loaded. A level change replaces the rooms, so a handle held across one goes stale rather than naming a different room: reading or writing a field on it raises an error. Check this for a handle held across time.

      Returns: boolean.

      Example:
      ```lua
      local start_room = trx.rooms[0]
      trx.events.after_control(function()
        if start_room:is_valid() then
          trx.log.info(tostring(start_room.underwater))
        end
      end)
      ```

    - [lua]`room:on_enter(callback, [opts])`  
      Happens when something changes rooms into this one.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). The `trx.items.Item` that changed rooms.
      - **`opts`** (table, optional). Options. `watch = "lara"` (the default) reacts to Lara alone; `watch = "all"` to every item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

      Example:
      ```lua
      trx.rooms[7]:on_enter(function(item)
        trx.log.info("entered room 7")
      end)
      ```

    - [lua]`room:on_exit(callback, [opts])`  
      Happens when something changes rooms out of this one.

      Parameters:
      - **`callback`** (function).
        Called with:
        - **`item`** (Item). The `trx.items.Item` that changed rooms.
      - **`opts`** (table, optional). Options. `watch = "lara"` (the default) reacts to Lara alone; `watch = "all"` to every item.

      Returns: integer. Listener id. Pass it to `trx.events.detach` to stop listening.

### Functions

- [lua]`trx.rooms.get(num)`  
  Retrieves a room by number. Rooms count from zero, matching the room numbers level editors show.

  Parameters:
  - **`num`** (integer). 0-based room number.

  Returns: Room or `nil`.

  Example:
  ```lua
  local room = trx.rooms[14]
  room.underwater = true
  ```

- [lua]`trx.rooms.count()`  
  Returns the number of rooms in the level. Same as `#trx.rooms`.

  Returns: integer.

- [lua]`trx.rooms.flip()`  
  Flips the current room map, swapping every room with its flip pair.

- [lua]`trx.rooms.flip_effect(effect_id, [timer])`  
  Sets the active flip effect, and optionally its timer.

  Parameters:
  - **`effect_id`** (integer). Use `-1` to clear the current effect. Compare against `trx.catalog.flip_effects`.
  - **`timer`** (integer, optional). Flip timer value.

  Example:
  ```lua
  trx.rooms.flip_effect(trx.catalog.flip_effects.floor_shake, 10)
  ```

- [lua]`trx.rooms.find_valid_pos(pos, room_num)`  
  Nudges a position into valid room geometry, e.g. to find somewhere an item can legally be placed.

  Parameters:
  - **`pos`** (vec3). Position to search near.
  - **`room_num`** (integer). 0-based room to search from.

  Returns:
  - vec3 or `nil`. The valid position, or `nil` if none was found nearby.
  - integer. The 0-based room the position is in.
