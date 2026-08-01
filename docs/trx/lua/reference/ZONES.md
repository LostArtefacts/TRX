---
title: Zones
order: 15
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/zones.lua. Edit it there.
-->

## <a id="zones" name="zones"></a>Zones module

A zone is a piece of the level worth keeping an eye on. Mark out a box or a
sphere in world space, or a single sector the way a floor trigger covers one,
and the zone reports when something steps into it, when it leaves, and for as
long as it stays.

A zone watches Lara and nobody else, unless it is made with `watch = "items"`,
and then it watches everything the level holds - enemies, pickups, anything
with a position. Its hooks hand over whatever set it off. To listen in one
place rather than zone by zone, [`trx.events.on_zone_enter`](EVENTS.md#events.on_zone_enter) and its siblings
hear about all of them.

A flyby camera passing through a zone is its own pair of hooks,
[`trx.zones.Zone:on_flyby_enter`](#zones.Zone.on_flyby_enter) and [`trx.zones.Zone:on_flyby_exit`](#zones.Zone.on_flyby_exit), since a
camera is not an item and there is nothing to hand a handler but the zone.
Every zone answers for a flyby, whatever it watches, and a tile answers for one
in the room the tile belongs to. The camera is checked only while a sequence is
playing.

Every frame, each zone settles who is inside before anything goes out, and the
exits come before the enters - so a script following Lara from one zone into
the next hears her leave the first before she enters the second. Something
destroyed while it is inside counts as leaving.

A box and a sphere take no notice of rooms, so where two rooms sit one above
the other over the same ground, a box tall enough to reach both catches what
stands in either. Only a tile belongs to a room. A flipmap changes the geometry
under a zone but moves nothing and renumbers nothing, so no zone sees anything
come or go.

Zones belong to the level that made them. A level change clears them, and the
level script makes them again the same way it attaches its handlers. A zone made
outside a level script - from a global script, or from the console - goes with
the next level change as well, and nothing makes it again. A global script that
wants a zone in every level makes it from a handler that runs once the level has
loaded. They are not written to savegames either, so Lara standing in a zone when
a game is loaded enters it again.

### Indexing

Indexing the module reaches a zone, by the order the zones were made or by the name one was made with. `#trx.zones` is how many there are, and `pairs()` walks them in that order.

- <a id="zones[]" name="zones[]"></a>**`trx.zones[key]`** (key: [trx.zones.Num](#zones.Num) or string, value: [trx.zones.Zone](#zones.Zone) or `nil`). Where the zone sits, or the name it was made with. A script holds the zone itself, or the name it gave it, rather than the number.
- **`#trx.zones`** (integer). How many there are.

Example:
```lua
trx.log.info(#trx.zones .. " zones, the first is a " .. trx.zones[1].type)
for _, zone in pairs(trx.zones) do
  zone:disable()
end
```

### Structures

- <a id="zones.Num" name="zones.Num"></a>[lua]`trx.zones.Num`

    Where a zone sits among the level's, counted in the order they were made. An earlier zone being removed shifts the rest along. Counted from 1.

- <a id="zones.Zone" name="zones.Zone"></a>[lua]`trx.zones.Zone`

    A script-defined trigger region. Reading a field of one that has been removed, or that a level change took away, raises rather than answering for a zone that is no longer there.

    Properties:
    - <a id="zones.Zone.centre" name="zones.Zone.centre"></a>**`centre`**: [trx.math.Vec3](MATH.md#math.Vec3). The middle of a sphere, and `nil` for a box or a tile. *(read-only)*
    - <a id="zones.Zone.enabled" name="zones.Zone.enabled"></a>**`enabled`**: boolean. Whether the zone is tested. Disabling it suspends the hooks without forgetting who is inside, so an item that leaves while it is off is reported as leaving when it comes back on. One destroyed while it is off is forgotten instead, and nothing is reported for it.
    - <a id="zones.Zone.max" name="zones.Zone.max"></a>**`max`**: [trx.math.Vec3](MATH.md#math.Vec3). The upper corner of a box or a tile, and `nil` for a sphere. *(read-only)*
    - <a id="zones.Zone.min" name="zones.Zone.min"></a>**`min`**: [trx.math.Vec3](MATH.md#math.Vec3). The lower corner of a box or a tile, and `nil` for a sphere. *(read-only)*
    - <a id="zones.Zone.name" name="zones.Zone.name"></a>**`name`**: string. The name the zone was made with, and `nil` for one made without. `trx.zones[name]` finds it again. *(read-only)*
    - <a id="zones.Zone.num" name="zones.Zone.num"></a>**`num`**: [trx.zones.Num](#zones.Num). Where the zone sits now. *(read-only)*
    - <a id="zones.Zone.radius" name="zones.Zone.radius"></a>**`radius`**: [trx.math.Distance](MATH.md#math.Distance). How far a sphere reaches, and `nil` for a box or a tile. *(read-only)*
    - <a id="zones.Zone.room_num" name="zones.Zone.room_num"></a>**`room_num`**: [trx.rooms.Num](ROOMS.md#rooms.Num). The room a tile belongs to, which is what keeps the same sector column in the room above or below from setting it off. Settled when the zone is made and fixed from then on, a flipmap included. `nil` for a box or a sphere. *(read-only)*
    - <a id="zones.Zone.type" name="zones.Zone.type"></a>**`type`**: string. The shape the zone was made with: `"box"`, `"sphere"` or `"tile"`. *(read-only)*
    - <a id="zones.Zone.watch" name="zones.Zone.watch"></a>**`watch`**: string. What sets the zone off: `"lara"` or `"items"`. *(read-only)*

    Methods:

    - <a id="zones.Zone.clear_occupants" name="zones.Zone.clear_occupants"></a>[lua]`zone:clear_occupants()`  
      Forgets who is inside, so anything still there enters again on the next frame. This is how a zone is made to fire a second time for an item that never left it. A flyby passing through is forgotten with the rest.

    - <a id="zones.Zone.contains_item" name="zones.Zone.contains_item"></a>[lua]`zone:contains_item(item)`  
      Whether the item is inside the region: the same test the zone makes every frame, so this answers whether the item counts as an occupant. An item is tested by the point it stands at, and one the world does not hold is nowhere.

      Parameters:
      - <a id="zones.Zone.contains_item.item" name="zones.Zone.contains_item.item"></a>**`item`** ([trx.items.Item](ITEMS.md#items.Item)).

      Returns: boolean. Whether the item counts as an occupant.

      Example:
      ```lua
      if plate:contains_item(trx.lara.item) then
        trx.log.info("she is standing on it")
      end
      ```

    - <a id="zones.Zone.contains_point" name="zones.Zone.contains_point"></a>[lua]`zone:contains_point(pos)`  
      Whether a world position lies inside the region. A plain test: no hooks are involved, and a disabled zone answers as readily as any other. A tile answers on position alone, so a point in the room stacked above it counts here where an item standing there would not.

      Parameters:
      - <a id="zones.Zone.contains_point.pos" name="zones.Zone.contains_point.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.

      Returns: boolean. Whether the point is inside.

    - <a id="zones.Zone.disable" name="zones.Zone.disable"></a>[lua]`zone:disable()`  
      Stops testing the zone, without forgetting who is inside, other than an occupant destroyed meanwhile. The same as `zone.enabled = false`.

    - <a id="zones.Zone.enable" name="zones.Zone.enable"></a>[lua]`zone:enable()`  
      Starts testing the zone again. The same as `zone.enabled = true`.

    - <a id="zones.Zone.is_valid" name="zones.Zone.is_valid"></a>[lua]`zone:is_valid()`  
      Whether the zone still exists. [`remove`](#zones.Zone.remove) and a level change both leave a handle stale.

      Returns: boolean. Whether the zone is still there.

    - <a id="zones.Zone.occupants" name="zones.Zone.occupants"></a>[lua]`zone:occupants()`  
      The items inside the zone as of the last frame it was tested, in item order. A disabled zone hands back who was inside when it was disabled.

      Returns: a list of [trx.items.Item](ITEMS.md#items.Item).

    - <a id="zones.Zone.on_enter" name="zones.Zone.on_enter"></a>[lua]`zone:on_enter(callback)`  
      Happens when something enters the zone.

      Parameters:
      - <a id="zones.Zone.on_enter.callback" name="zones.Zone.on_enter.callback"></a>**`callback`** (function). What to run when it happens.
        Called with:
        - <a id="zones.Zone.on_enter.item" name="zones.Zone.on_enter.item"></a>**`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that entered.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The listener. [`remove`](#zones.Zone.remove) detaches what a zone carries as well.

      Example:
      ```lua
      local door = trx.zones.box(
        { x = 51200, y = -2048, z = 30720 },
        { x = 53248, y = 0, z = 32768 })
      door:on_enter(function(item)
        trx.log.info("someone stepped in")
      end)
      ```

    - <a id="zones.Zone.on_exit" name="zones.Zone.on_exit"></a>[lua]`zone:on_exit(callback)`  
      Happens when something leaves the zone, and when something inside it is destroyed.

      Parameters:
      - <a id="zones.Zone.on_exit.callback" name="zones.Zone.on_exit.callback"></a>**`callback`** (function). What to run when it happens.
        Called with:
        - <a id="zones.Zone.on_exit.item" name="zones.Zone.on_exit.item"></a>**`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that left.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The listener. [`remove`](#zones.Zone.remove) detaches what a zone carries as well.

    - <a id="zones.Zone.on_flyby_enter" name="zones.Zone.on_flyby_enter"></a>[lua]`zone:on_flyby_enter(callback)`  
      Happens when a flyby camera enters the zone. A flyby is not an item and sets off no other hook, so a handler takes nothing: the zone is the whole of what happened.

      Parameters:
      - <a id="zones.Zone.on_flyby_enter.callback" name="zones.Zone.on_flyby_enter.callback"></a>**`callback`** (function). What to run when it happens.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The listener. [`remove`](#zones.Zone.remove) detaches what a zone carries as well.

      Example:
      ```lua
      local hall = trx.zones.box(
        { x = 51200, y = -2048, z = 30720 },
        { x = 53248, y = 0, z = 32768 })
      hall:on_flyby_enter(function()
        trx.music.play(trx.catalog.music.main_theme)
      end)
      ```

    - <a id="zones.Zone.on_flyby_exit" name="zones.Zone.on_flyby_exit"></a>[lua]`zone:on_flyby_exit(callback)`  
      Happens when a flyby camera leaves the zone, and when the sequence ends while the camera is still inside one.

      Parameters:
      - <a id="zones.Zone.on_flyby_exit.callback" name="zones.Zone.on_flyby_exit.callback"></a>**`callback`** (function). What to run when it happens.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The listener. [`remove`](#zones.Zone.remove) detaches what a zone carries as well.

    - <a id="zones.Zone.on_tick" name="zones.Zone.on_tick"></a>[lua]`zone:on_tick(callback)`  
      Happens on every logical frame something is inside the zone, including the frame it enters.

      Parameters:
      - <a id="zones.Zone.on_tick.callback" name="zones.Zone.on_tick.callback"></a>**`callback`** (function). What to run when it happens.
        Called with:
        - <a id="zones.Zone.on_tick.item" name="zones.Zone.on_tick.item"></a>**`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item that is inside.

      Returns: [trx.events.Listener](EVENTS.md#events.Listener). The listener. [`remove`](#zones.Zone.remove) detaches what a zone carries as well.

    - <a id="zones.Zone.remove" name="zones.Zone.remove"></a>[lua]`zone:remove()`  
      Removes the zone and detaches the hooks attached to it. Its handle goes stale: [`is_valid`](#zones.Zone.is_valid) says so, and reading a field raises.

### Functions

- <a id="zones.box" name="zones.box"></a>[lua]`trx.zones.box(min, max, [opts])`  
  Creates a zone from a world-space box. The corners may come in any order. Rooms play no part: what stands inside the box is inside it, whichever room holds it.

  Parameters:
  - <a id="zones.box.min" name="zones.box.min"></a>**`min`** ([trx.math.Vec3](MATH.md#math.Vec3)). One corner of the box.
  - <a id="zones.box.max" name="zones.box.max"></a>**`max`** ([trx.math.Vec3](MATH.md#math.Vec3)). The opposite corner of the box.
  - <a id="zones.box.opts" name="zones.box.opts"></a>**`opts`** (table, optional). How the zone is watched, and what it is called.

    Keys:
    - <a id="zones.box.opts.watch" name="zones.box.opts.watch"></a>**`watch`** (string, optional, default `"lara"`). What sets the zone off: `"lara"` tests Lara alone, and `"items"` tests every item the level holds.
    - <a id="zones.box.opts.name" name="zones.box.opts.name"></a>**`name`** (string, optional). A name `trx.zones[name]` finds the zone by. Raises where the level already has a zone of that name.

  Returns: [trx.zones.Zone](#zones.Zone). The zone.

  Example:
  ```lua
  local arena = trx.zones.box(
    { x = 51200, y = -2048, z = 30720 },
    { x = 53248, y = 0, z = 32768 },
    { watch = "items", name = "arena" })
  ```

- <a id="zones.sphere" name="zones.sphere"></a>[lua]`trx.zones.sphere(centre, radius, [opts])`  
  Creates a zone from a point and a radius. Rooms play no part, as they do not for [`trx.zones.box`](#zones.box).

  Parameters:
  - <a id="zones.sphere.centre" name="zones.sphere.centre"></a>**`centre`** ([trx.math.Vec3](MATH.md#math.Vec3)). Middle of the sphere.
  - <a id="zones.sphere.radius" name="zones.sphere.radius"></a>**`radius`** ([trx.math.Distance](MATH.md#math.Distance)). How far out it reaches.
  - <a id="zones.sphere.opts" name="zones.sphere.opts"></a>**`opts`** (table, optional). How the zone is watched, and what it is called.

    Keys:
    - <a id="zones.sphere.opts.watch" name="zones.sphere.opts.watch"></a>**`watch`** (string, optional, default `"lara"`). What sets the zone off: `"lara"` tests Lara alone, and `"items"` tests every item the level holds.
    - <a id="zones.sphere.opts.name" name="zones.sphere.opts.name"></a>**`name`** (string, optional). A name `trx.zones[name]` finds the zone by. Raises where the level already has a zone of that name.

  Returns: [trx.zones.Zone](#zones.Zone). The zone.

  Example:
  ```lua
  local bell = trx.zones.sphere(trx.lara.item.pos, 2048, { watch = "items" })
  ```

- <a id="zones.tile" name="zones.tile"></a>[lua]`trx.zones.tile(pos, [opts])`  
  Creates a zone from the sector under a position, in the room holding that position, at any height - the way a floor trigger occupies a sector. The same sector column in the room above or below does not set it off. Where rooms overlap and several of them hold the position, the zone takes the first, which is the lowest-numbered room rather than the nearest floor; [`trx.zones.Zone.room_num`](#zones.Zone.room_num) says which one it settled on.

  Parameters:
  - <a id="zones.tile.pos" name="zones.tile.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). A world position inside the sector.
  - <a id="zones.tile.opts" name="zones.tile.opts"></a>**`opts`** (table, optional). How the zone is watched, and what it is called.

    Keys:
    - <a id="zones.tile.opts.watch" name="zones.tile.opts.watch"></a>**`watch`** (string, optional, default `"lara"`). What sets the zone off: `"lara"` tests Lara alone, and `"items"` tests every item the level holds.
    - <a id="zones.tile.opts.name" name="zones.tile.opts.name"></a>**`name`** (string, optional). A name `trx.zones[name]` finds the zone by. Raises where the level already has a zone of that name.

  Returns: [trx.zones.Zone](#zones.Zone) or `nil`. The zone, or `nil` when the position lies outside the level.

  Example:
  ```lua
  local plate = trx.zones.tile(trx.lara.item.pos)
  plate:on_enter(function(item)
    trx.log.info("stepped on the plate")
  end)
  ```

- <a id="zones.get" name="zones.get"></a>[lua]`trx.zones.get(key)`  
  Retrieves a zone by its place in the module or by the name it was made with. The same as indexing the module.

  Parameters:
  - <a id="zones.get.key" name="zones.get.key"></a>**`key`** ([trx.zones.Num](#zones.Num) or string). Where the zone sits, or the name it was made with.

  Returns: [trx.zones.Zone](#zones.Zone) or `nil`.

- <a id="zones.count" name="zones.count"></a>[lua]`trx.zones.count()`  
  How many zones the level has. The same as `#trx.zones`.

  Returns: integer. How many zones the level holds.
