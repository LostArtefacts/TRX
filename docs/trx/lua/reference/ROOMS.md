---
title: Rooms
order: 6
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/rooms.lua. Edit it there.
-->

## <a id="rooms" name="rooms"></a>Rooms module

Module for inspecting and altering the rooms of the current level.

### Indexing

Indexing the module reaches a room, and `#trx.rooms` is how many the level has. `pairs()` walks them in order, keyed by the room number.

- <a id="rooms[]" name="rooms[]"></a>**`trx.rooms[key]`** (key: [trx.rooms.Num](#rooms.Num), value: [trx.rooms.Room](#rooms.Room) or `nil`).
- **`#trx.rooms`** (integer). How many there are.

Example:
```lua
trx.log.info(#trx.rooms .. " rooms, first is " .. trx.rooms[0].num)
for num, room in pairs(trx.rooms) do
  room.cold = true
end
```

### Properties

- <a id="rooms.flip_group_count" name="rooms.flip_group_count"></a>**`trx.rooms.flip_group_count`** (integer). How many flip groups a level can hold. A room belongs to one of them, and a flip
  moves that group alone. *(read-only)*
- <a id="rooms.flipped" name="rooms.flipped"></a>**`trx.rooms.flipped`** (boolean). Whether the group that moved last is showing its flip pairs. *(read-only)*
- <a id="rooms.query" name="rooms.query"></a>**`trx.rooms.query`** ([trx.rooms.RoomQuery](#rooms.RoomQuery)). The identity query over every room in the level. Narrow it and read it. *(read-only)*

### Enums

- <a id="rooms.FlipStatus" name="rooms.FlipStatus"></a>[lua]`trx.rooms.FlipStatus`

    The values [`trx.rooms.Room.flip_status`](#rooms.Room.flip_status) can take.

    - `trx.rooms.FlipStatus.NONE` = `0`  
        This is a normal room.
    - `trx.rooms.FlipStatus.UNFLIPPED` = `1`  
        This room is currently reachable by Lara.
    - `trx.rooms.FlipStatus.FLIPPED` = `2`  
        This room is currently inactive and unreachable by Lara.

### Structures

- <a id="rooms.Num" name="rooms.Num"></a>[lua]`trx.rooms.Num`

    Room number, matching the numbers level editors show. Counted from 0.

- <a id="rooms.Room" name="rooms.Room"></a>[lua]`trx.rooms.Room`

    A room in the current level.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="rooms.Room.cold" name="rooms.Room.cold"></a>**`cold`**: boolean. Whether Lara's breath is visible in the room.
    - <a id="rooms.Room.damaging" name="rooms.Room.damaging"></a>**`damaging`**: boolean. Whether the room drains Lara's exposure meter.
    - <a id="rooms.Room.flip_status" name="rooms.Room.flip_status"></a>**`flip_status`**: [trx.rooms.FlipStatus](#rooms.FlipStatus). Current flip status. *(read-only)*
    - <a id="rooms.Room.num" name="rooms.Room.num"></a>**`num`**: [trx.rooms.Num](#rooms.Num). *(read-only)*
    - <a id="rooms.Room.swamp" name="rooms.Room.swamp"></a>**`swamp`**: boolean. Whether the room is filled with swamp water, which Lara wades through and sinks into rather than swimming.
    - <a id="rooms.Room.underwater" name="rooms.Room.underwater"></a>**`underwater`**: boolean. Whether the room is filled with water.
    - <a id="rooms.Room.wind" name="rooms.Room.wind"></a>**`wind`**: boolean. Whether the room has a breeze. Requires the player to have breeze enabled.

    Computed properties (derived, not stored on the object):
    - <a id="rooms.Room.bounds" name="rooms.Room.bounds"></a>**`bounds`**: [trx.math.Box](MATH.md#math.Box). Where the room sits in the world.
    - <a id="rooms.Room.flipped_room" name="rooms.Room.flipped_room"></a>**`flipped_room`**: [trx.rooms.Room](#rooms.Room). This room's flip pair, or `nil` if it has none.
    - <a id="rooms.Room.internal_bounds" name="rooms.Room.internal_bounds"></a>**`internal_bounds`**: [trx.math.Box](MATH.md#math.Box). As [`bounds`](#rooms.Room.bounds), but excluding the outer ring of sectors, which is solid wall.

    Methods:

    - <a id="rooms.Room.floor_height" name="rooms.Room.floor_height"></a>[lua]`room:floor_height(pos, [opts])`  
      As [`trx.rooms.floor_height`](#rooms.floor_height), looking from this room.

      Parameters:
      - <a id="rooms.Room.floor_height.pos" name="rooms.Room.floor_height.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.
      - <a id="rooms.Room.floor_height.opts" name="rooms.Room.floor_height.opts"></a>**`opts`** (table, optional). How to read the floor.

        Keys:
        - <a id="rooms.Room.floor_height.opts.fix_tilts" name="rooms.Room.floor_height.opts.fix_tilts"></a>**`fix_tilts`** (boolean, optional, default `true`). Whether a floor tilt that lies inside a wall is taken into account. `false` gives the flat height the original games read there, which is what the geometry glitches of the vanilla levels rest on.

      Returns: [trx.math.Distance](MATH.md#math.Distance) or `nil`. The height, with `nil` where there is no floor.

    - <a id="rooms.Room.is_valid" name="rooms.Room.is_valid"></a>[lua]`room:is_valid()`  
      Whether the handle still refers to a room of the level that is loaded. A level change replaces the rooms, so a handle held across one goes stale rather than naming a different room: reading or writing a field on it raises an error. Check this for a handle held across time.

      Returns: boolean. False once the level that held the room has been left.

      Example:
      ```lua
      local start_room = trx.rooms[0]
      trx.events.after_control(function()
        if start_room:is_valid() then
          trx.log.info(tostring(start_room.underwater))
        end
      end)
      ```

    - <a id="rooms.Room.on_enter" name="rooms.Room.on_enter"></a>[lua]`room:on_enter(callback, [opts])`  
      Happens when something changes rooms into this one.

      Parameters:
      - <a id="rooms.Room.on_enter.callback" name="rooms.Room.on_enter.callback"></a>**`callback`** (function). What to run when it happens.
        Called with:
        - <a id="rooms.Room.on_enter.item" name="rooms.Room.on_enter.item"></a>**`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that changed rooms.
      - <a id="rooms.Room.on_enter.opts" name="rooms.Room.on_enter.opts"></a>**`opts`** (table, optional). What to watch for.

        Keys:
        - <a id="rooms.Room.on_enter.opts.watch" name="rooms.Room.on_enter.opts.watch"></a>**`watch`** (string, optional, default `"lara"`). Either `"lara"`, which reacts to Lara alone, or `"all"`, which reacts to every item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

      Example:
      ```lua
      trx.rooms[7]:on_enter(function(item)
        trx.log.info("entered room 7")
      end)
      ```

    - <a id="rooms.Room.on_exit" name="rooms.Room.on_exit"></a>[lua]`room:on_exit(callback, [opts])`  
      Happens when something changes rooms out of this one.

      Parameters:
      - <a id="rooms.Room.on_exit.callback" name="rooms.Room.on_exit.callback"></a>**`callback`** (function). What to run when it happens.
        Called with:
        - <a id="rooms.Room.on_exit.item" name="rooms.Room.on_exit.item"></a>**`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that changed rooms.
      - <a id="rooms.Room.on_exit.opts" name="rooms.Room.on_exit.opts"></a>**`opts`** (table, optional). What to watch for.

        Keys:
        - <a id="rooms.Room.on_exit.opts.watch" name="rooms.Room.on_exit.opts.watch"></a>**`watch`** (string, optional, default `"lara"`). Either `"lara"`, which reacts to Lara alone, or `"all"`, which reacts to every item.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The attached handler.

- <a id="rooms.RoomQuery" name="rooms.RoomQuery"></a>[lua]`trx.rooms.RoomQuery`

    A [`trx.query.Query`](QUERY.md#query.Query) over the rooms of the current level, with the narrowings below on top of the ones every query has. Rooms answer to no names, so the name layer is absent.

    Methods:

    - <a id="rooms.RoomQuery.at" name="rooms.RoomQuery.at"></a>[lua]`roomquery:at(pos)`  
      The room contains a world position. Rooms overlap, so a position can be in several at once and every one of them matches, in room order. A room claims a point when the point is within its bounds, the outer ring of solid wall aside, and the column it stands in has a floor - the test the engine itself puts a position through. The hidden half of a flip pair is passed over.

      Parameters:
      - <a id="rooms.RoomQuery.at.pos" name="rooms.RoomQuery.at.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

      Example:
      ```lua
      trx.rooms.query:at(trx.lara.item.pos):first()
      ```

    - <a id="rooms.RoomQuery.dry" name="rooms.RoomQuery.dry"></a>[lua]`roomquery:dry()`  
      The room holds neither water nor swamp water.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="rooms.RoomQuery.flipped" name="rooms.RoomQuery.flipped"></a>[lua]`roomquery:flipped()`  
      The room is the half of a flip pair the level is not showing. Its geometry is still there to inspect, but nothing can be in it.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="rooms.RoomQuery.reachable" name="rooms.RoomQuery.reachable"></a>[lua]`roomquery:reachable()`  
      The room is part of the level as it stands: an ordinary room, or the half of a flip pair the level is showing. This is what a script asking about the world wants, and what [`at`](#rooms.RoomQuery.at) already applies.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

      Example:
      ```lua
      trx.rooms.query:reachable():underwater():count()
      ```

    - <a id="rooms.RoomQuery.swamp" name="rooms.RoomQuery.swamp"></a>[lua]`roomquery:swamp()`  
      The room is filled with swamp water.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="rooms.RoomQuery.underwater" name="rooms.RoomQuery.underwater"></a>[lua]`roomquery:underwater()`  
      The room is filled with water.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

### Functions

- <a id="rooms.get" name="rooms.get"></a>[lua]`trx.rooms.get(num)`  
  Retrieves a room by number.

  Parameters:
  - <a id="rooms.get.num" name="rooms.get.num"></a>**`num`** ([trx.rooms.Num](#rooms.Num)).

  Returns: [trx.rooms.Room](#rooms.Room) or `nil`. The room, or `nil` where the level has no such number.

  Example:
  ```lua
  local room = trx.rooms[14]
  room.underwater = true
  ```

- <a id="rooms.count" name="rooms.count"></a>[lua]`trx.rooms.count()`  
  Returns the number of rooms in the level. Same as `#trx.rooms`.

  Returns: integer. How many rooms the loaded level holds.

- <a id="rooms.flip_groups" name="rooms.flip_groups"></a>[lua]`trx.rooms.flip_groups(groups)`  
  Puts rooms in flip groups. A level script can then move some flip pairs while the
  rest stay where they are. Each entry names one room and the group it belongs to. Its flip pair
  joins the same group.

  Call this only from the top level of a level script. Rooms must be grouped before the level
  starts, so the game can restore flipped groups correctly when it loads a save.

  A level with no groups moves all flip pairs together. After a script names any group, each flip
  trigger moves only the group with the same number.

  Parameters:
  - <a id="rooms.flip_groups.groups" name="rooms.flip_groups.groups"></a>**`groups`** (table). Flip groups, keyed by [`trx.rooms.Num`](#rooms.Num).

  Example:
  ```lua
  trx.rooms.flip_groups({ [33] = 1, [37] = 2 })
  ```

- <a id="rooms.flip" name="rooms.flip"></a>[lua]`trx.rooms.flip([group])`  
  Flips rooms, swapping each with its flip pair. With no group given, every group
  moves.

  Parameters:
  - <a id="rooms.flip.group" name="rooms.flip.group"></a>**`group`** (integer, optional). Which flip group to act on, counted from 0. A level splits its flip pairs into
    groups and moves one at a time; a game that names no group places every room in the first.
    Omit this to act on every group.

  Example:
  ```lua
  trx.rooms.flip()
  ```

  Example:
  ```lua
  trx.rooms.flip(3)
  ```

- <a id="rooms.is_flipped" name="rooms.is_flipped"></a>[lua]`trx.rooms.is_flipped([group])`  
  Whether a group of rooms is showing its flip pairs. With no group given, answers
  for the group that moved last, which is what the world itself reads.

  Parameters:
  - <a id="rooms.is_flipped.group" name="rooms.is_flipped.group"></a>**`group`** (integer, optional). Which flip group to act on, counted from 0. A level splits its flip pairs into
    groups and moves one at a time; a game that names no group places every room in the first.
    Omit this to act on every group.

  Returns:
  - boolean. Whether that group is showing its pairs.

- <a id="rooms.flip_effect" name="rooms.flip_effect"></a>[lua]`trx.rooms.flip_effect(effect_id, [timer])`  
  Sets the active flip effect, and optionally its timer.

  Parameters:
  - <a id="rooms.flip_effect.effect_id" name="rooms.flip_effect.effect_id"></a>**`effect_id`** ([trx.catalog.flip_effects](CATALOG.md#catalog.flip_effects)). Use `-1` to clear the current effect.
  - <a id="rooms.flip_effect.timer" name="rooms.flip_effect.timer"></a>**`timer`** (integer, optional). Flip timer value.

  Example:
  ```lua
  trx.rooms.flip_effect(trx.catalog.flip_effects.floor_shake, 10)
  ```

- <a id="rooms.floor_height" name="rooms.floor_height"></a>[lua]`trx.rooms.floor_height(pos, [room_num], [opts])`  
  The height of the floor under a world position. `nil` where there is no floor at all: inside solid geometry, or off the edge of the level.

  Parameters:
  - <a id="rooms.floor_height.pos" name="rooms.floor_height.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.
  - <a id="rooms.floor_height.room_num" name="rooms.floor_height.room_num"></a>**`room_num`** ([trx.rooms.Num](#rooms.Num), optional). The search crosses portals, so a neighbouring room's floor is found too. Without it, the room is looked up from the position, which takes the first room that contains it and passes over the flipped-away ones. Where rooms overlap, name the room, or ask the room itself with [`trx.rooms.Room:floor_height`](#rooms.Room.floor_height).
  - <a id="rooms.floor_height.opts" name="rooms.floor_height.opts"></a>**`opts`** (table, optional). How to read the floor.

    Keys:
    - <a id="rooms.floor_height.opts.fix_tilts" name="rooms.floor_height.opts.fix_tilts"></a>**`fix_tilts`** (boolean, optional, default `true`). Whether a floor tilt that lies inside a wall is taken into account. `false` gives the flat height the original games read there, which is what the geometry glitches of the vanilla levels rest on.

  Returns: [trx.math.Distance](MATH.md#math.Distance) or `nil`. The height, with `nil` where there is no floor.

  Example:
  ```lua
  local floor = trx.lara.item.room:floor_height(trx.lara.item.pos)
  ```

- <a id="rooms.find_valid_pos" name="rooms.find_valid_pos"></a>[lua]`trx.rooms.find_valid_pos(pos, room_num)`  
  Nudges a position into valid room geometry, e.g. to find somewhere an item can legally be placed.

  Parameters:
  - <a id="rooms.find_valid_pos.pos" name="rooms.find_valid_pos.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). Position to search near.
  - <a id="rooms.find_valid_pos.room_num" name="rooms.find_valid_pos.room_num"></a>**`room_num`** ([trx.rooms.Num](#rooms.Num)).

  Returns:
  - [trx.math.Vec3](MATH.md#math.Vec3) or `nil`. The valid position, or `nil` if none was found nearby.
  - [trx.rooms.Num](#rooms.Num). The room the position is in.
