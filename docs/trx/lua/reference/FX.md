---
title: Fx
order: 17
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/fx.lua. Edit it there.
-->

## <a id="fx" name="fx"></a>Fx module

What a script puts in front of the player: things seen rather than things the
game holds.

### Indexing

The level fog bulbs, counted from 1. `#trx.fx.fog_bulbs` is the count.
`pairs()` walks them in order. A level shows at most twenty. The player can turn them off.

- <a id="fx.fog_bulbs[]" name="fx.fog_bulbs[]"></a>**`trx.fx.fog_bulbs[key]`** (key: integer, value: [trx.fx.FogBulb](#fx.FogBulb) or `nil`).
- **`#trx.fx.fog_bulbs`** (integer). How many there are.

Example:
```lua
for _, bulb in pairs(trx.fx.fog_bulbs) do
  bulb.color = trx.math.color(245, 200, 60)
end
```

### Properties

- <a id="fx.fog_color" name="fx.fog_color"></a>**`trx.fx.fog_color`** ([trx.math.Color](MATH.md#math.Color)). The color override for distance fog.
  `nil` means no override. Write `nil` to restore the level fog color. A level change clears the
  override. Savegames keep it. This controls distance fog only. Fog bulbs have their own colors
  in [`trx.fx.fog_bulbs`](#fx.fog_bulbs).

### Constants

- <a id="fx.MAX_LIGHTS" name="fx.MAX_LIGHTS"></a>[lua]`trx.fx.MAX_LIGHTS` = `64` (integer)  
  How many lights a script can put up in one frame.

- <a id="fx.MAX_FOG" name="fx.MAX_FOG"></a>[lua]`trx.fx.MAX_FOG` = `10` (integer)  
  How many balls of fog can be seen at once. TR4 levels carry fog of their own, which takes its slots first, so fewer than this reach the screen where a level is already using them.

### Structures

- <a id="fx.FogBulb" name="fx.FogBulb"></a>[lua]`trx.fx.FogBulb`

    A level fog bulb is a ball of fog drawn inside a room.

    TR4 stores fog bulbs as room lights. A script can change their color and density.
    The level sets their position and radius. A bulb follows the fog color until a script
    gives it a color of its own.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="fx.FogBulb.color" name="fx.FogBulb.color"></a>**`color`**: [trx.math.Color](MATH.md#math.Color). The color a script gave the bulb.
      `nil` means none was given, and the bulb is drawn in the fog color in force. Write `nil` to hand
      a bulb back to that color.
    - <a id="fx.FogBulb.density" name="fx.FogBulb.density"></a>**`density`**: integer. Fog density, from `0` for none to `255`. A value outside this range raises an error.
    - <a id="fx.FogBulb.pos" name="fx.FogBulb.pos"></a>**`pos`**: [trx.math.Vec3](MATH.md#math.Vec3). Center of the fog bulb. *(read-only)*
    - <a id="fx.FogBulb.radius" name="fx.FogBulb.radius"></a>**`radius`**: [trx.math.Distance](MATH.md#math.Distance). How far the fog reaches from that position. *(read-only)*

    Computed properties (derived, not stored on the object):
    - <a id="fx.FogBulb.room" name="fx.FogBulb.room"></a>**`room`**: [trx.rooms.Room](ROOMS.md#rooms.Room). The room the bulb sits in.

    Methods:

    - <a id="fx.FogBulb.is_valid" name="fx.FogBulb.is_valid"></a>[lua]`fogbulb:is_valid()`  
      Reports whether the handle still names a bulb in the loaded level.

      A level change replaces all bulbs. A handle held across one becomes stale, and field access
      raises an error.

      Returns: boolean. False after the level that held the bulb is left.

### Functions

- <a id="fx.emit_light" name="fx.emit_light"></a>[lua]`trx.fx.emit_light(opts)`  
  Lights the world around a point for this frame. Make the call every frame to
  keep the light up, and at a new position each time to move it.

  A frame shows only [`trx.fx.MAX_LIGHTS`](#fx.MAX_LIGHTS) lights. A light asked for past that
  takes the place of the one furthest from the camera, so the nearest are the
  ones seen. No error is raised.

  TR1 and TR2 light in brightness alone, so there the light is as bright as its
  brightest channel and comes out white.

  Parameters:
  - <a id="fx.emit_light.opts" name="fx.emit_light.opts"></a>**`opts`** (table). Where the light is and what it looks like.

    Keys:
    - <a id="fx.emit_light.opts.pos" name="fx.emit_light.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.
    - <a id="fx.emit_light.opts.radius" name="fx.emit_light.opts.radius"></a>**`radius`** ([trx.math.Distance](MATH.md#math.Distance), optional, default `3072`). How far it reaches, at least an eighth of a sector. Rounded down to the eighth of a sector the engine measures a dynamic light in, and carried about a quarter further than asked for in TR4, which falls a light off more gently.
    - <a id="fx.emit_light.opts.color" name="fx.emit_light.opts.color"></a>**`color`** (table, optional). Its color, each channel 0 to 255. Defaults to white.

  Example:
  ```lua
  trx.events.after_control(function()
    trx.fx.emit_light({
      pos = trx.lara.item.pos,
      radius = 2048,
      color = { r = 0, g = 255, b = 192 },
    })
  end)
  ```

- <a id="fx.emit_fog" name="fx.emit_fog"></a>[lua]`trx.fx.emit_fog(opts)`  
  Fills a ball of air with fog for this frame, which the player sees through
  rather than on. Where a light brightens what it falls on, this hangs in the
  space itself. Make the call every frame to keep the fog up.

  A frame shows only [`trx.fx.MAX_FOG`](#fx.MAX_FOG) balls of fog. A ball asked for past that
  takes the place of the one furthest from the camera, so the nearest are the
  ones seen. No error is raised.

  Parameters:
  - <a id="fx.emit_fog.opts" name="fx.emit_fog.opts"></a>**`opts`** (table). Where the fog is and what it looks like.

    Keys:
    - <a id="fx.emit_fog.opts.pos" name="fx.emit_fog.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.
    - <a id="fx.emit_fog.opts.radius" name="fx.emit_fog.opts.radius"></a>**`radius`** ([trx.math.Distance](MATH.md#math.Distance), optional, default `2048`). How far the fog reaches, at least one unit.
    - <a id="fx.emit_fog.opts.density" name="fx.emit_fog.opts.density"></a>**`density`** (integer, optional, default `128`). How thick it is, from 0 for nothing to 255.
    - <a id="fx.emit_fog.opts.color" name="fx.emit_fog.opts.color"></a>**`color`** (table, optional). Its color, each channel 0 to 255. Defaults to white.

  Example:
  ```lua
  trx.events.after_control(function()
    trx.fx.emit_fog({
      pos = { x = 32768, y = -1024, z = 45056 },
      radius = 3072,
      density = 64,
      color = { r = 128, g = 160, b = 192 },
    })
  end)
  ```

- <a id="fx.blood" name="fx.blood"></a>[lua]`trx.fx.blood(opts)`  
  Throws a spray of blood into the world at a point, the way a blow that lands
  does. The drops then fall on their own.

  TR3 and TR4 throw drops that fall and darken as they go, and in TR4 a hit under
  water spreads as a cloud instead. TR1 and TR2 have one blood sprite that drifts
  up.

  Unlike the rest of the module, this has a bearing on what the game decides. The
  engine places the drops from the control random stream, and in TR1 and TR2 the
  spray takes a slot from the effect pool a save holds.

  Parameters:
  - <a id="fx.blood.opts" name="fx.blood.opts"></a>**`opts`** (table). Where the blood is and how much of it.

    Keys:
    - <a id="fx.blood.opts.pos" name="fx.blood.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.blood.opts.angle" name="fx.blood.opts.angle"></a>**`angle`** ([trx.math.Angle](MATH.md#math.Angle), optional). The way the drops fly. Left out, TR4 throws them every way, which is what its own hits do where nothing aims them; the other games read it as straight ahead.
    - <a id="fx.blood.opts.strength" name="fx.blood.opts.strength"></a>**`strength`** (integer, optional, default `5`). How heavy the hit reads, from 1 to 255. TR3 and TR4 count it in drops, TR4 measures the width of a cloud under water with it, and TR1 and TR2 have one drifting sprite whose speed it sets.

  Example:
  ```lua
  trx.events.on_hit(function(item, damage)
    trx.fx.blood({ pos = item.pos, angle = item.rot.y, strength = damage })
  end)
  ```

- <a id="fx.blood_bath" name="fx.blood_bath"></a>[lua]`trx.fx.blood_bath(opts)`  
  Throws several sprays of blood about a point, the way a trap that kills does.
  Each one lands anywhere in the half-sector box around the point, so the blood
  covers a body rather than a spot.

  Each spray costs what [`trx.fx.blood`](#fx.blood) costs, in random draws and in effect
  slots.

  Parameters:
  - <a id="fx.blood_bath.opts" name="fx.blood_bath.opts"></a>**`opts`** (table). Where the blood is, how much of it, and how many sprays.

    Keys:
    - <a id="fx.blood_bath.opts.pos" name="fx.blood_bath.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.blood_bath.opts.angle" name="fx.blood_bath.opts.angle"></a>**`angle`** ([trx.math.Angle](MATH.md#math.Angle), optional). The way the drops fly. Left out, TR4 throws them every way, which is what its own hits do where nothing aims them; the other games read it as straight ahead.
    - <a id="fx.blood_bath.opts.strength" name="fx.blood_bath.opts.strength"></a>**`strength`** (integer, optional, default `5`). How heavy the hit reads, from 1 to 255. TR3 and TR4 count it in drops, TR4 measures the width of a cloud under water with it, and TR1 and TR2 have one drifting sprite whose speed it sets.
    - <a id="fx.blood_bath.opts.count" name="fx.blood_bath.opts.count"></a>**`count`** (integer, optional, default `5`). How many sprays, from 1 to 255.

  Example:
  ```lua
  trx.fx.blood_bath({ pos = trx.lara.item.pos, count = 10 })
  ```
