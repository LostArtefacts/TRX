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

The spark pool, counted from 1. `#trx.fx.sparks.pool` is
[`MAX_COUNT`](#fx.sparks.MAX_COUNT) rather than how many sparks are alive: a slot holding
no live spark reads as `nil`.

- <a id="fx.sparks.pool[]" name="fx.sparks.pool[]"></a>**`trx.fx.sparks.pool[key]`** (key: integer, value: [trx.fx.Spark](#fx.Spark) or `nil`).
- **`#trx.fx.sparks.pool`** (integer). How many there are.

Example:
```lua
for _, spark in pairs(trx.fx.sparks.pool) do
  spark.color = trx.math.color(255, 0, 0)
end
```

### Properties

- <a id="fx.fog_color" name="fx.fog_color"></a>**`trx.fx.fog_color`** ([trx.math.Color](MATH.md#math.Color)). The color override for distance fog.
  `nil` means no override. Write `nil` to restore the level fog color. A level change clears the
  override. Savegames keep it. This controls distance fog only. Fog bulbs have their own colors
  in [`trx.fx.fog_bulbs`](#fx.fog_bulbs).
- <a id="fx.sparks.wind" name="fx.sparks.wind"></a>**`trx.fx.sparks.wind`** ([trx.fx.Wind](#fx.Wind)). The wind that carries the sparks marked [`trx.fx.Spark.is_outside`](#fx.Spark.is_outside).
  The engine works this out again every frame from the breeze setting, so a value
  written here holds for that frame alone.

### Constants

- <a id="fx.MAX_LIGHTS" name="fx.MAX_LIGHTS"></a>[lua]`trx.fx.MAX_LIGHTS` = `64` (integer)  
  How many lights a script can put up in one frame.

- <a id="fx.MAX_FOG" name="fx.MAX_FOG"></a>[lua]`trx.fx.MAX_FOG` = `10` (integer)  
  How many balls of fog can be seen at once. TR4 levels carry fog of their own, which takes its slots first, so fewer than this reach the screen where a level is already using them.

- <a id="fx.sparks.MAX_COUNT" name="fx.sparks.MAX_COUNT"></a>[lua]`trx.fx.sparks.MAX_COUNT` = `400` (integer)  
  How many sparks the pool holds. A spark spawned after that takes the slot of the one with the least life left.

### Enums

- <a id="fx.SparkType" name="fx.SparkType"></a>[lua]`trx.fx.SparkType`

    Which spark-set sprite a spark is drawn with.

    - `trx.fx.SparkType.EXPLOSION` = `0`  
        The soft round puff fire, smoke and explosions are drawn with.
    - `trx.fx.SparkType.SMALL_SPLASH` = `4`  
        A single drop of water.
    - `trx.fx.SparkType.BIG_SPLASH` = `8`  
        A sheet of water.
    - `trx.fx.SparkType.RIPPLE` = `9`  
        A ring on the water surface.
    - `trx.fx.SparkType.PARTICLE` = `10`  
        The plain speck, which is also what a footprint is drawn with.
    - `trx.fx.SparkType.SHIELD` = `11`  
        The bubble drawn around a shielded target.
    - `trx.fx.SparkType.ROPE` = `19`  
        A length of rope.
    - `trx.fx.SparkType.DRIVE` = `20`  
        The forward gear light of a vehicle.
    - `trx.fx.SparkType.REVERSE` = `21`  
        The reverse gear light of a vehicle.
    - `trx.fx.SparkType.RICOCHET` = `36`  
        The spark struck off a wall by a shot.
    - `trx.fx.SparkType.BLOOD` = `39`  
        A drop of blood.

- <a id="fx.DrawType" name="fx.DrawType"></a>[lua]`trx.fx.DrawType`

    How a sprite is laid over what is behind it.

    - `trx.fx.DrawType.OPAQUE` = `0`  
        It covers what is behind it.
    - `trx.fx.DrawType.BLEND` = `1`  
        It is mixed with what is behind it.
    - `trx.fx.DrawType.BLEND_ADD` = `2`  
        It is added to what is behind it, so it lightens.
    - `trx.fx.DrawType.BLEND_SUB` = `3`  
        It is taken from what is behind it, so it darkens.
    - `trx.fx.DrawType.REFLECTIVE_OPAQUE` = `8`  
        Opaque, and carrying the room reflection.
    - `trx.fx.DrawType.REFLECTIVE_BLEND_ADD` = `9`  
        Added, and carrying the room reflection.

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

- <a id="fx.Spark" name="fx.Spark"></a>[lua]`trx.fx.Spark`

    A spark is one particle from the spark pool: a sprite that lives for a set
    number of frames, moving, resizing and fading on its own as it does.

    TR3 and TR4 only. The earlier games carry no spark set, so nothing spawns one
    and the pool stays empty.

    A spark that runs out of life leaves its slot to the next one asked for. A
    handle held across that becomes stale, and field access raises an error.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="fx.Spark.color" name="fx.Spark.color"></a>**`color`**: [trx.math.Color](MATH.md#math.Color). The color it is drawn in now.
    - <a id="fx.Spark.draw_type" name="fx.Spark.draw_type"></a>**`draw_type`**: [trx.fx.DrawType](#fx.DrawType). How it is laid over what is behind it. *(read-only)*
    - <a id="fx.Spark.end_color" name="fx.Spark.end_color"></a>**`end_color`**: [trx.math.Color](MATH.md#math.Color). The color it fades to.
    - <a id="fx.Spark.end_height" name="fx.Spark.end_height"></a>**`end_height`**: integer. The height it grows to.
    - <a id="fx.Spark.end_width" name="fx.Spark.end_width"></a>**`end_width`**: integer. The width it grows to.
    - <a id="fx.Spark.extras" name="fx.Spark.extras"></a>**`extras`**: integer. How many further sparks it leaves behind as it ends.
    - <a id="fx.Spark.fade_speed" name="fx.Spark.fade_speed"></a>**`fade_speed`**: integer. How fast it travels from one color to the other.
    - <a id="fx.Spark.fade_to_black" name="fx.Spark.fade_to_black"></a>**`fade_to_black`**: integer. How many frames of life are left when it starts to darken toward black.
    - <a id="fx.Spark.friction" name="fx.Spark.friction"></a>**`friction`**: integer. How fast it is slowed each frame.
    - <a id="fx.Spark.gravity" name="fx.Spark.gravity"></a>**`gravity`**: integer. How much it is pulled down each frame.
    - <a id="fx.Spark.height" name="fx.Spark.height"></a>**`height`**: integer. How tall it is drawn now.
    - <a id="fx.Spark.is_blood" name="fx.Spark.is_blood"></a>**`is_blood`**: boolean. Whether it counts as blood, which the pool sheds first when it runs short of slots.
    - <a id="fx.Spark.is_green" name="fx.Spark.is_green"></a>**`is_green`**: boolean. Whether it is drawn in the green of the poison gas.
    - <a id="fx.Spark.is_outside" name="fx.Spark.is_outside"></a>**`is_outside`**: boolean. Whether the wind carries it.
    - <a id="fx.Spark.is_underwater" name="fx.Spark.is_underwater"></a>**`is_underwater`**: boolean. Whether it drifts like an underwater particle.
    - <a id="fx.Spark.life" name="fx.Spark.life"></a>**`life`**: integer. Frames of life left. It reaches zero and the spark ends.
    - <a id="fx.Spark.life_span" name="fx.Spark.life_span"></a>**`life_span`**: integer. Frames of life it started with, which the fades are measured against.
    - <a id="fx.Spark.max_y_vel" name="fx.Spark.max_y_vel"></a>**`max_y_vel`**: integer. As fast as gravity may carry it down.
    - <a id="fx.Spark.node_num" name="fx.Spark.node_num"></a>**`node_num`**: integer. Which of the sixteen body points it hangs off, where it is attached to one.
    - <a id="fx.Spark.pos" name="fx.Spark.pos"></a>**`pos`**: [trx.math.Vec3](MATH.md#math.Vec3). Where it sits, in the world, or from what it is attached to. Read [`world_pos`](#fx.Spark.world_pos) for the position it is drawn at.
    - <a id="fx.Spark.room_num" name="fx.Spark.room_num"></a>**`room_num`**: [trx.rooms.Num](ROOMS.md#rooms.Num). The room it was spawned in. *(read-only)*
    - <a id="fx.Spark.rot_add" name="fx.Spark.rot_add"></a>**`rot_add`**: integer. How far it turns each frame.
    - <a id="fx.Spark.rot_angle" name="fx.Spark.rot_angle"></a>**`rot_angle`**: integer. How far it is turned about the view, from 0 to 4095.
    - <a id="fx.Spark.rotates" name="fx.Spark.rotates"></a>**`rotates`**: boolean. Whether it turns as it lives.
    - <a id="fx.Spark.scalar" name="fx.Spark.scalar"></a>**`scalar`**: integer. How strongly the size is scaled with distance.
    - <a id="fx.Spark.scales" name="fx.Spark.scales"></a>**`scales`**: boolean. Whether it travels from its start size to its end size.
    - <a id="fx.Spark.start_color" name="fx.Spark.start_color"></a>**`start_color`**: [trx.math.Color](MATH.md#math.Color). The color it fades from.
    - <a id="fx.Spark.start_height" name="fx.Spark.start_height"></a>**`start_height`**: integer. The height it grows from.
    - <a id="fx.Spark.start_width" name="fx.Spark.start_width"></a>**`start_width`**: integer. The width it grows from.
    - <a id="fx.Spark.uses_alt_sprite" name="fx.Spark.uses_alt_sprite"></a>**`uses_alt_sprite`**: boolean. Whether it is drawn with the second sprite of its kind.
    - <a id="fx.Spark.vel" name="fx.Spark.vel"></a>**`vel`**: [trx.math.Vec3](MATH.md#math.Vec3). How far it moves each frame.
    - <a id="fx.Spark.width" name="fx.Spark.width"></a>**`width`**: integer. How wide it is drawn now.

    Computed properties (derived, not stored on the object):
    - <a id="fx.Spark.item" name="fx.Spark.item"></a>**`item`**: [trx.items.Item](ITEMS.md#items.Item). The item it is attached to, and `nil` where it hangs on nothing.
    - <a id="fx.Spark.room" name="fx.Spark.room"></a>**`room`**: [trx.rooms.Room](ROOMS.md#rooms.Room). The room it was spawned in.
    - <a id="fx.Spark.world_pos" name="fx.Spark.world_pos"></a>**`world_pos`**: [trx.math.Vec3](MATH.md#math.Vec3). Where it is drawn, which is where it sits unless it is attached to something that carries it.

    Methods:

    - <a id="fx.Spark.is_valid" name="fx.Spark.is_valid"></a>[lua]`spark:is_valid()`  
      Reports whether the handle still names a live spark.

      A spark that runs out of life leaves its slot to the next one asked for.

      Returns: boolean. False once the spark has ended.

    - <a id="fx.Spark.kill" name="fx.Spark.kill"></a>[lua]`spark:kill()`  
      Ends the spark now and frees its slot.

- <a id="fx.Wind" name="fx.Wind"></a>[lua]`trx.fx.Wind`

    How far the wind carries a spark each frame.

    Properties:
    - <a id="fx.Wind.x" name="fx.Wind.x"></a>**`x`**: [trx.math.Distance](MATH.md#math.Distance). The east-west axis.
    - <a id="fx.Wind.z" name="fx.Wind.z"></a>**`z`**: [trx.math.Distance](MATH.md#math.Distance). The north-south axis.

### Functions

- <a id="fx.sparks" name="fx.sparks"></a>[lua]`trx.fx.sparks`  
  The spark pool: the particles TR3 and TR4 draw their smoke, flames, sparks and
  splashes with.

  TR1 and TR2 carry no spark set. There the pool stays empty, and everything here
  returns nothing rather than raising.

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

- <a id="fx.explosion" name="fx.explosion"></a>[lua]`trx.fx.explosion(opts)`  
  Shows the explosion a rocket or a grenade leaves behind, without the damage.

  TR1, TR2 and TR4 draw the explosion sprite the level carries. TR3 has none, and
  gets a fireball of sparks instead. Under water the effect uses the drowned
  version, which throws a burst of bubbles and lifts a splash where the water
  ends.

  Parameters:
  - <a id="fx.explosion.opts" name="fx.explosion.opts"></a>**`opts`** (table). Where the explosion is and whether it is heard.

    Keys:
    - <a id="fx.explosion.opts.pos" name="fx.explosion.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.
    - <a id="fx.explosion.opts.sound" name="fx.explosion.opts.sound"></a>**`sound`** (boolean, optional, default `true`). Whether the explosion sound plays with it.

  Example:
  ```lua
  trx.fx.explosion({ pos = trx.lara.item.pos })
  ```

- <a id="fx.fire" name="fx.fire"></a>[lua]`trx.fx.fire(opts)`  
  Burns a fire at a point for this frame. Make the call every frame to keep the
  fire alight.

  TR4 only. The other games have no such fire, so the call does nothing there.

  Parameters:
  - <a id="fx.fire.opts" name="fx.fire.opts"></a>**`opts`** (table). Where the fire is and how it burns.

    Keys:
    - <a id="fx.fire.opts.pos" name="fx.fire.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.
    - <a id="fx.fire.opts.size" name="fx.fire.opts.size"></a>**`size`** (integer, optional, default `1`). How big it burns: `0` small, `1` medium, `2` big.
    - <a id="fx.fire.opts.fade" name="fx.fire.opts.fade"></a>**`fade`** (integer, optional, default `0`). How far it is dimmed, `0` being full strength.

  Example:
  ```lua
  trx.events.after_control(function()
    trx.fx.fire({ pos = trx.lara.item.pos, size = 2 })
  end)
  ```

- <a id="fx.splash" name="fx.splash"></a>[lua]`trx.fx.splash(item)`  
  Breaks the water at an item, the way a body falling in does. The item says
  where the splash is; the water it lands in says how big.

  Parameters:
  - <a id="fx.splash.item" name="fx.splash.item"></a>**`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item the splash rises around.

  Example:
  ```lua
  trx.fx.splash(trx.lara.item)
  ```

- <a id="fx.wade_splash" name="fx.wade_splash"></a>[lua]`trx.fx.wade_splash(item, depth)`  
  Breaks the water around an item wading through it.

  Parameters:
  - <a id="fx.wade_splash.item" name="fx.wade_splash.item"></a>**`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item doing the wading.
  - <a id="fx.wade_splash.depth" name="fx.wade_splash.depth"></a>**`depth`** ([trx.math.Distance](MATH.md#math.Distance)). How deep the water stands about it.

  Example:
  ```lua
  trx.fx.wade_splash(trx.lara.item, 512)
  ```

- <a id="fx.ripple" name="fx.ripple"></a>[lua]`trx.fx.ripple(opts)`  
  Spreads a ring on the water surface. The ring widens and fades on its own.

  Parameters:
  - <a id="fx.ripple.opts" name="fx.ripple.opts"></a>**`opts`** (table). Where the ring is and how it spreads.

    Keys:
    - <a id="fx.ripple.opts.pos" name="fx.ripple.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.
    - <a id="fx.ripple.opts.size" name="fx.ripple.opts.size"></a>**`size`** (integer, optional, default `8`). How wide it starts, from 1 to 255.
    - <a id="fx.ripple.opts.slow" name="fx.ripple.opts.slow"></a>**`slow`** (boolean, optional, default `false`). Whether it spreads at half speed.
    - <a id="fx.ripple.opts.dark" name="fx.ripple.opts.dark"></a>**`dark`** (boolean, optional, default `false`). Whether it is drawn dark rather than bright.
    - <a id="fx.ripple.opts.blood" name="fx.ripple.opts.blood"></a>**`blood`** (boolean, optional, default `false`). Whether it is drawn in the blood color.
    - <a id="fx.ripple.opts.jitter" name="fx.ripple.opts.jitter"></a>**`jitter`** (boolean, optional, default `false`). Whether the ring wavers as it spreads.

  Example:
  ```lua
  trx.fx.ripple({ pos = trx.lara.item.pos, size = 16 })
  ```

- <a id="fx.small_splash" name="fx.small_splash"></a>[lua]`trx.fx.small_splash(opts)`  
  Throws a handful of drops off the water surface.

  Parameters:
  - <a id="fx.small_splash.opts" name="fx.small_splash.opts"></a>**`opts`** (table). Where the drops are and how many of them.

    Keys:
    - <a id="fx.small_splash.opts.pos" name="fx.small_splash.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.
    - <a id="fx.small_splash.opts.count" name="fx.small_splash.opts.count"></a>**`count`** (integer, optional, default `1`). How many drops, from 1 to 255.

  Example:
  ```lua
  trx.fx.small_splash({ pos = trx.lara.item.pos, count = 8 })
  ```

- <a id="fx.underwater_blood" name="fx.underwater_blood"></a>[lua]`trx.fx.underwater_blood(opts)`  
  Spreads a cloud of blood under water, the way a hit that lands there does.

  Parameters:
  - <a id="fx.underwater_blood.opts" name="fx.underwater_blood.opts"></a>**`opts`** (table). Where the cloud is and how wide it spreads.

    Keys:
    - <a id="fx.underwater_blood.opts.pos" name="fx.underwater_blood.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.
    - <a id="fx.underwater_blood.opts.size" name="fx.underwater_blood.opts.size"></a>**`size`** (integer, optional, default `8`). How wide it spreads, from 1 to 255.
    - <a id="fx.underwater_blood.opts.dark" name="fx.underwater_blood.opts.dark"></a>**`dark`** (boolean, optional, default `false`). Whether it is drawn in the darker TR3 gold color.

  Example:
  ```lua
  trx.fx.underwater_blood({ pos = trx.lara.item.pos, size = 32 })
  ```

- <a id="fx.footprint" name="fx.footprint"></a>[lua]`trx.fx.footprint(item, left_foot)`  
  Leaves a footprint under an item, on the floor the item stands on. The floor
  material decides whether one is left at all.

  Parameters:
  - <a id="fx.footprint.item" name="fx.footprint.item"></a>**`item`** ([trx.items.Item](ITEMS.md#items.Item)). The item the print is taken from.
  - <a id="fx.footprint.left_foot" name="fx.footprint.left_foot"></a>**`left_foot`** (boolean). Whether it is the left foot rather than the right.

  Example:
  ```lua
  trx.fx.footprint(trx.lara.item, true)
  ```

- <a id="fx.knockback" name="fx.knockback"></a>[lua]`trx.fx.knockback(opts)`  
  Spreads a ring of force out from a point, as a blast does. The ring is drawn
  and widens on its own.

  Parameters:
  - <a id="fx.knockback.opts" name="fx.knockback.opts"></a>**`opts`** (table). Where the ring starts.

    Keys:
    - <a id="fx.knockback.opts.pos" name="fx.knockback.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.

  Example:
  ```lua
  trx.fx.knockback({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.spawn" name="fx.sparks.spawn"></a>[lua]`trx.fx.sparks.spawn(opts)`  
  Puts one spark in the world and hands it back, so a script can draw with the
  pool the game draws its own smoke and flames with.

  The spark lives for the frames it is given, moving, resizing and fading on its
  own, and then frees its slot. Nothing has to be called each frame to keep it up.

  Returns `nil` where the level carries no spark set, which is every TR1 and TR2
  level.

  Parameters:
  - <a id="fx.sparks.spawn.opts" name="fx.sparks.spawn.opts"></a>**`opts`** (table). What the spark is and how it behaves.

    Keys:
    - <a id="fx.sparks.spawn.opts.pos" name="fx.sparks.spawn.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.spawn.opts.vel" name="fx.sparks.spawn.opts.vel"></a>**`vel`** ([trx.math.Vec3](MATH.md#math.Vec3), optional). How far it moves each frame. Defaults to standing still.
    - <a id="fx.sparks.spawn.opts.sprite_type" name="fx.sparks.spawn.opts.sprite_type"></a>**`sprite_type`** ([trx.fx.SparkType](#fx.SparkType), optional, default [`trx.fx.SparkType.PARTICLE`](#fx.SparkType)). Which sprite it is drawn with.
    - <a id="fx.sparks.spawn.opts.life" name="fx.sparks.spawn.opts.life"></a>**`life`** (integer, optional, default `16`). How many frames it lives, from 1 to 255.
    - <a id="fx.sparks.spawn.opts.color" name="fx.sparks.spawn.opts.color"></a>**`color`** ([trx.math.Color](MATH.md#math.Color), optional). The color it starts in. Defaults to white.
    - <a id="fx.sparks.spawn.opts.end_color" name="fx.sparks.spawn.opts.end_color"></a>**`end_color`** ([trx.math.Color](MATH.md#math.Color), optional). The color it fades to. Defaults to the color it starts in, so it holds one color.
    - <a id="fx.sparks.spawn.opts.fade_speed" name="fx.sparks.spawn.opts.fade_speed"></a>**`fade_speed`** (integer, optional, default `8`). How fast it travels between the two colors.
    - <a id="fx.sparks.spawn.opts.fade_to_black" name="fx.sparks.spawn.opts.fade_to_black"></a>**`fade_to_black`** (integer, optional, default `8`). How many frames of life are left when it starts to darken toward black.
    - <a id="fx.sparks.spawn.opts.width" name="fx.sparks.spawn.opts.width"></a>**`width`** (integer, optional, default `4`). How wide it starts, from 0 to 255.
    - <a id="fx.sparks.spawn.opts.height" name="fx.sparks.spawn.opts.height"></a>**`height`** (integer, optional, default `4`). How tall it starts, from 0 to 255.
    - <a id="fx.sparks.spawn.opts.end_width" name="fx.sparks.spawn.opts.end_width"></a>**`end_width`** (integer, optional). The width it grows to, for a spark that scales. Defaults to the width it starts at.
    - <a id="fx.sparks.spawn.opts.end_height" name="fx.sparks.spawn.opts.end_height"></a>**`end_height`** (integer, optional). The height it grows to, for a spark that scales. Defaults to the height it starts at.
    - <a id="fx.sparks.spawn.opts.scales" name="fx.sparks.spawn.opts.scales"></a>**`scales`** (boolean, optional, default `false`). Whether it travels from its start size to its end size over its life.
    - <a id="fx.sparks.spawn.opts.scalar" name="fx.sparks.spawn.opts.scalar"></a>**`scalar`** (integer, optional, default `2`). How strongly the size is scaled with distance.
    - <a id="fx.sparks.spawn.opts.gravity" name="fx.sparks.spawn.opts.gravity"></a>**`gravity`** (integer, optional, default `0`). How much it is pulled down each frame.
    - <a id="fx.sparks.spawn.opts.max_y_vel" name="fx.sparks.spawn.opts.max_y_vel"></a>**`max_y_vel`** (integer, optional, default `0`). As fast as gravity may carry it down.
    - <a id="fx.sparks.spawn.opts.friction" name="fx.sparks.spawn.opts.friction"></a>**`friction`** (integer, optional, default `0`). How fast it is slowed each frame.
    - <a id="fx.sparks.spawn.opts.rot_angle" name="fx.sparks.spawn.opts.rot_angle"></a>**`rot_angle`** (integer, optional, default `0`). How far it starts turned about the view, from 0 to 4095.
    - <a id="fx.sparks.spawn.opts.rot_add" name="fx.sparks.spawn.opts.rot_add"></a>**`rot_add`** (integer, optional, default `0`). How far it turns each frame, for a spark that turns.
    - <a id="fx.sparks.spawn.opts.rotates" name="fx.sparks.spawn.opts.rotates"></a>**`rotates`** (boolean, optional, default `false`). Whether it turns as it lives.
    - <a id="fx.sparks.spawn.opts.extras" name="fx.sparks.spawn.opts.extras"></a>**`extras`** (integer, optional, default `0`). How many further sparks it leaves behind as it ends.
    - <a id="fx.sparks.spawn.opts.draw_type" name="fx.sparks.spawn.opts.draw_type"></a>**`draw_type`** ([trx.fx.DrawType](#fx.DrawType), optional, default [`trx.fx.DrawType.BLEND_ADD`](#fx.DrawType)). How it is laid over what is behind it.
    - <a id="fx.sparks.spawn.opts.is_outside" name="fx.sparks.spawn.opts.is_outside"></a>**`is_outside`** (boolean, optional, default `false`). Whether the wind carries it.
    - <a id="fx.sparks.spawn.opts.is_underwater" name="fx.sparks.spawn.opts.is_underwater"></a>**`is_underwater`** (boolean, optional, default `false`). Whether it drifts like an underwater particle.
    - <a id="fx.sparks.spawn.opts.is_green" name="fx.sparks.spawn.opts.is_green"></a>**`is_green`** (boolean, optional, default `false`). Whether it is drawn in the green of the poison gas.
    - <a id="fx.sparks.spawn.opts.uses_alt_sprite" name="fx.sparks.spawn.opts.uses_alt_sprite"></a>**`uses_alt_sprite`** (boolean, optional, default `false`). Whether it is drawn with the second sprite of its kind.

  Returns: [trx.fx.Spark](#fx.Spark) or `nil`. The spark, or `nil` where the level carries no spark set.

  Example:
  ```lua
  trx.fx.sparks.spawn({
    pos = trx.lara.item.pos,
    vel = { x = 0, y = -8, z = 0 },
    sprite_type = trx.fx.SparkType.EXPLOSION,
    life = 48,
    color = trx.math.color(255, 200, 64),
    end_color = trx.math.color(64, 0, 0),
    width = 16,
    end_width = 48,
    scales = true,
  })
  ```

- <a id="fx.sparks.explosion" name="fx.sparks.explosion"></a>[lua]`trx.fx.sparks.explosion(opts)`  
  Throws the fireball of sparks an explosion is drawn with.

  Parameters:
  - <a id="fx.sparks.explosion.opts" name="fx.sparks.explosion.opts"></a>**`opts`** (table). Where the fireball is and how strongly it bursts.

    Keys:
    - <a id="fx.sparks.explosion.opts.pos" name="fx.sparks.explosion.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.explosion.opts.extras" name="fx.sparks.explosion.opts.extras"></a>**`extras`** (integer, optional, default `3`). How many further sparks each one leaves behind as it ends, from 0 to 3.
    - <a id="fx.sparks.explosion.opts.light" name="fx.sparks.explosion.opts.light"></a>**`light`** (integer, optional, default `-2`). How the fireball lights the room around it: `-2` bright, `-1` dim, `0` not at all.
    - <a id="fx.sparks.explosion.opts.underwater" name="fx.sparks.explosion.opts.underwater"></a>**`underwater`** (boolean, optional, default `false`). Whether it uses the underwater burst.

  Example:
  ```lua
  trx.fx.sparks.explosion({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.explosion_smoke" name="fx.sparks.explosion_smoke"></a>[lua]`trx.fx.sparks.explosion_smoke(opts)`  
  Lifts the smoke an explosion leaves behind.

  Parameters:
  - <a id="fx.sparks.explosion_smoke.opts" name="fx.sparks.explosion_smoke.opts"></a>**`opts`** (table). Where the smoke is and which part of the burst it is.

    Keys:
    - <a id="fx.sparks.explosion_smoke.opts.pos" name="fx.sparks.explosion_smoke.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.explosion_smoke.opts.underwater" name="fx.sparks.explosion_smoke.opts.underwater"></a>**`underwater`** (boolean, optional, default `false`). Whether it lifts as it does under water.
    - <a id="fx.sparks.explosion_smoke.opts.ending" name="fx.sparks.explosion_smoke.opts.ending"></a>**`ending`** (boolean, optional, default `false`). Whether it is the thinner smoke that closes the burst rather than the smoke that opens it.

  Example:
  ```lua
  trx.fx.sparks.explosion_smoke({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.explosion_bubble" name="fx.sparks.explosion_bubble"></a>[lua]`trx.fx.sparks.explosion_bubble(opts)`  
  Throws the burst of bubbles an explosion under water makes.

  Parameters:
  - <a id="fx.sparks.explosion_bubble.opts" name="fx.sparks.explosion_bubble.opts"></a>**`opts`** (table). Where the bubbles rise.

    Keys:
    - <a id="fx.sparks.explosion_bubble.opts.pos" name="fx.sparks.explosion_bubble.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.

  Example:
  ```lua
  trx.fx.sparks.explosion_bubble({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.fire_flame" name="fx.sparks.fire_flame"></a>[lua]`trx.fx.sparks.fire_flame(opts)`  
  Throws one tongue of flame, the way a burning body does. Make the call every
  frame to keep a fire burning.

  No flame is thrown more than twenty sectors from Lara.

  Parameters:
  - <a id="fx.sparks.fire_flame.opts" name="fx.sparks.fire_flame.opts"></a>**`opts`** (table). Where the flame is and what color it burns.

    Keys:
    - <a id="fx.sparks.fire_flame.opts.pos" name="fx.sparks.fire_flame.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.fire_flame.opts.variant" name="fx.sparks.fire_flame.opts.variant"></a>**`variant`** (integer, optional, default `0`). Which colors it burns in: `0` orange, `2` pale, `254` green.

  Example:
  ```lua
  trx.fx.sparks.fire_flame({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.fire_smoke" name="fx.sparks.fire_smoke"></a>[lua]`trx.fx.sparks.fire_smoke(opts)`  
  Lifts one puff of the smoke a fire gives off.

  Parameters:
  - <a id="fx.sparks.fire_smoke.opts" name="fx.sparks.fire_smoke.opts"></a>**`opts`** (table). Where the smoke is and which flame color it follows.

    Keys:
    - <a id="fx.sparks.fire_smoke.opts.pos" name="fx.sparks.fire_smoke.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.fire_smoke.opts.variant" name="fx.sparks.fire_smoke.opts.variant"></a>**`variant`** (integer, optional, default `0`). Which colors it burns in: `0` orange, `2` pale, `254` green.

  Example:
  ```lua
  trx.fx.sparks.fire_smoke({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.static_flame" name="fx.sparks.static_flame"></a>[lua]`trx.fx.sparks.static_flame(opts)`  
  Throws one tongue of the flame a standing fire burns with.

  Parameters:
  - <a id="fx.sparks.static_flame.opts" name="fx.sparks.static_flame.opts"></a>**`opts`** (table). Where the flame is and how big it burns.

    Keys:
    - <a id="fx.sparks.static_flame.opts.pos" name="fx.sparks.static_flame.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.static_flame.opts.size" name="fx.sparks.static_flame.opts.size"></a>**`size`** (integer, optional, default `32`). How big it burns.

  Example:
  ```lua
  trx.fx.sparks.static_flame({ pos = trx.lara.item.pos, size = 64 })
  ```

- <a id="fx.sparks.side_flame" name="fx.sparks.side_flame"></a>[lua]`trx.fx.sparks.side_flame(opts)`  
  Throws a tongue of flame sideways, as a jet does.

  Parameters:
  - <a id="fx.sparks.side_flame.opts" name="fx.sparks.side_flame.opts"></a>**`opts`** (table). Where the jet is and which way it burns.

    Keys:
    - <a id="fx.sparks.side_flame.opts.pos" name="fx.sparks.side_flame.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.side_flame.opts.angle" name="fx.sparks.side_flame.opts.angle"></a>**`angle`** ([trx.math.Angle](MATH.md#math.Angle)). The way the flame is thrown.
    - <a id="fx.sparks.side_flame.opts.speed" name="fx.sparks.side_flame.opts.speed"></a>**`speed`** (integer, optional, default `32`). How hard it is thrown.
    - <a id="fx.sparks.side_flame.opts.pilot" name="fx.sparks.side_flame.opts.pilot"></a>**`pilot`** (boolean, optional, default `false`). Whether it is the small pilot flame rather than the jet itself.

  Example:
  ```lua
  trx.fx.sparks.side_flame({
    pos = trx.lara.item.pos,
    angle = trx.lara.item.rot.y,
  })
  ```

- <a id="fx.sparks.flamethrower_flame" name="fx.sparks.flamethrower_flame"></a>[lua]`trx.fx.sparks.flamethrower_flame(opts)`  
  Throws the flame a flamethrower leaves where its jet lands.

  Parameters:
  - <a id="fx.sparks.flamethrower_flame.opts" name="fx.sparks.flamethrower_flame.opts"></a>**`opts`** (table). Where the flame lands.

    Keys:
    - <a id="fx.sparks.flamethrower_flame.opts.pos" name="fx.sparks.flamethrower_flame.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.

  Example:
  ```lua
  trx.fx.sparks.flamethrower_flame({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.flamethrower_smoke" name="fx.sparks.flamethrower_smoke"></a>[lua]`trx.fx.sparks.flamethrower_smoke(opts)`  
  Lifts the smoke a flamethrower jet leaves behind.

  Parameters:
  - <a id="fx.sparks.flamethrower_smoke.opts" name="fx.sparks.flamethrower_smoke.opts"></a>**`opts`** (table). Where the smoke is.

    Keys:
    - <a id="fx.sparks.flamethrower_smoke.opts.pos" name="fx.sparks.flamethrower_smoke.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.flamethrower_smoke.opts.underwater" name="fx.sparks.flamethrower_smoke.opts.underwater"></a>**`underwater`** (boolean, optional, default `false`). Whether it lifts as it does under water.

  Example:
  ```lua
  trx.fx.sparks.flamethrower_smoke({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.gun_smoke" name="fx.sparks.gun_smoke"></a>[lua]`trx.fx.sparks.gun_smoke(opts)`  
  Lifts the smoke a fired weapon leaves at its muzzle.

  Parameters:
  - <a id="fx.sparks.gun_smoke.opts" name="fx.sparks.gun_smoke.opts"></a>**`opts`** (table). Where the smoke is and which weapon left it.

    Keys:
    - <a id="fx.sparks.gun_smoke.opts.pos" name="fx.sparks.gun_smoke.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.gun_smoke.opts.weapon" name="fx.sparks.gun_smoke.opts.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon it is drawn for. `UNKNOWN`, `UNARMED`, and out-of-range values raise.
    - <a id="fx.sparks.gun_smoke.opts.shade" name="fx.sparks.gun_smoke.opts.shade"></a>**`shade`** (integer, optional, default `64`). How light the smoke is, from 0 to 255.
    - <a id="fx.sparks.gun_smoke.opts.initial" name="fx.sparks.gun_smoke.opts.initial"></a>**`initial`** (boolean, optional, default `false`). Whether it is the first puff of a shot, which is denser than the ones that follow.
    - <a id="fx.sparks.gun_smoke.opts.vel" name="fx.sparks.gun_smoke.opts.vel"></a>**`vel`** ([trx.math.Vec3](MATH.md#math.Vec3), optional). Which way the smoke is pushed. Left out, it lifts straight up.

  Example:
  ```lua
  trx.fx.sparks.gun_smoke({
    pos = trx.lara.item.pos,
    weapon = trx.catalog.weapons.pistols,
    initial = true,
  })
  ```

- <a id="fx.sparks.dart_smoke" name="fx.sparks.dart_smoke"></a>[lua]`trx.fx.sparks.dart_smoke(opts)`  
  Lifts the trail of smoke a flying dart leaves.

  Parameters:
  - <a id="fx.sparks.dart_smoke.opts" name="fx.sparks.dart_smoke.opts"></a>**`opts`** (table). Where the smoke is and which way the dart flies.

    Keys:
    - <a id="fx.sparks.dart_smoke.opts.pos" name="fx.sparks.dart_smoke.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.dart_smoke.opts.vel" name="fx.sparks.dart_smoke.opts.vel"></a>**`vel`** ([trx.math.Vec3](MATH.md#math.Vec3), optional). How far it moves each frame. Defaults to standing still.
    - <a id="fx.sparks.dart_smoke.opts.hit" name="fx.sparks.dart_smoke.opts.hit"></a>**`hit`** (boolean, optional, default `false`). Whether the dart has landed, which puffs the smoke out rather than trailing it.

  Example:
  ```lua
  trx.fx.sparks.dart_smoke({ pos = trx.lara.item.pos, hit = true })
  ```

- <a id="fx.sparks.rocket_smoke" name="fx.sparks.rocket_smoke"></a>[lua]`trx.fx.sparks.rocket_smoke(opts)`  
  Lifts the trail of smoke a flying rocket leaves.

  Parameters:
  - <a id="fx.sparks.rocket_smoke.opts" name="fx.sparks.rocket_smoke.opts"></a>**`opts`** (table). Where the smoke is and how light it lifts.

    Keys:
    - <a id="fx.sparks.rocket_smoke.opts.pos" name="fx.sparks.rocket_smoke.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.rocket_smoke.opts.shade" name="fx.sparks.rocket_smoke.opts.shade"></a>**`shade`** (integer, optional, default `0`). How light the smoke turns as it fades, from 0 to 191.

  Example:
  ```lua
  trx.fx.sparks.rocket_smoke({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.flare" name="fx.sparks.flare"></a>[lua]`trx.fx.sparks.flare(opts)`  
  Throws the sparks a burning flare gives off.

  Parameters:
  - <a id="fx.sparks.flare.opts" name="fx.sparks.flare.opts"></a>**`opts`** (table). Where the sparks are and which way they fly.

    Keys:
    - <a id="fx.sparks.flare.opts.pos" name="fx.sparks.flare.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.flare.opts.vel" name="fx.sparks.flare.opts.vel"></a>**`vel`** ([trx.math.Vec3](MATH.md#math.Vec3), optional). How far it moves each frame. Defaults to standing still.
    - <a id="fx.sparks.flare.opts.smoke" name="fx.sparks.flare.opts.smoke"></a>**`smoke`** (boolean, optional, default `false`). Whether smoke is lifted with them.

  Example:
  ```lua
  trx.fx.sparks.flare({ pos = trx.lara.item.pos, smoke = true })
  ```

- <a id="fx.sparks.shotgun" name="fx.sparks.shotgun"></a>[lua]`trx.fx.sparks.shotgun(opts)`  
  Throws the sparks a shotgun blast strikes off what it hits.

  Parameters:
  - <a id="fx.sparks.shotgun.opts" name="fx.sparks.shotgun.opts"></a>**`opts`** (table). Where the sparks are and which way they fly.

    Keys:
    - <a id="fx.sparks.shotgun.opts.pos" name="fx.sparks.shotgun.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.shotgun.opts.vel" name="fx.sparks.shotgun.opts.vel"></a>**`vel`** ([trx.math.Vec3](MATH.md#math.Vec3), optional). How far it moves each frame. Defaults to standing still.

  Example:
  ```lua
  trx.fx.sparks.shotgun({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.ricochet" name="fx.sparks.ricochet"></a>[lua]`trx.fx.sparks.ricochet(opts)`  
  Strikes the sparks a shot makes where it lands on a wall.

  TR4 throws as many streaks as asked for and can lift smoke instead. TR3 throws
  one burst, and uses the count as its size.

  Parameters:
  - <a id="fx.sparks.ricochet.opts" name="fx.sparks.ricochet.opts"></a>**`opts`** (table). Where the shot lands and which way the sparks fly.

    Keys:
    - <a id="fx.sparks.ricochet.opts.pos" name="fx.sparks.ricochet.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.ricochet.opts.angle" name="fx.sparks.ricochet.opts.angle"></a>**`angle`** ([trx.math.Angle](MATH.md#math.Angle)). The way the sparks fly.
    - <a id="fx.sparks.ricochet.opts.count" name="fx.sparks.ricochet.opts.count"></a>**`count`** (integer, optional, default `3`). How many streaks, from 1 to 255.
    - <a id="fx.sparks.ricochet.opts.smoke_only" name="fx.sparks.ricochet.opts.smoke_only"></a>**`smoke_only`** (boolean, optional, default `false`). Whether smoke is lifted rather than sparks struck. TR4 only.

  Example:
  ```lua
  trx.fx.sparks.ricochet({ pos = trx.lara.item.pos, angle = 0 })
  ```

- <a id="fx.sparks.bubble" name="fx.sparks.bubble"></a>[lua]`trx.fx.sparks.bubble(opts)`  
  Lifts one bubble through the water.

  Parameters:
  - <a id="fx.sparks.bubble.opts" name="fx.sparks.bubble.opts"></a>**`opts`** (table). Where the bubble is and how big it is.

    Keys:
    - <a id="fx.sparks.bubble.opts.pos" name="fx.sparks.bubble.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.bubble.opts.size" name="fx.sparks.bubble.opts.size"></a>**`size`** (integer, optional, default `8`). How big it is at its smallest.
    - <a id="fx.sparks.bubble.opts.size_range" name="fx.sparks.bubble.opts.size_range"></a>**`size_range`** (integer, optional, default `8`). How much bigger than that it may be drawn.

  Example:
  ```lua
  trx.fx.sparks.bubble({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.breath" name="fx.sparks.breath"></a>[lua]`trx.fx.sparks.breath(opts)`  
  Puffs the cloud of breath a body gives off in the cold.

  Parameters:
  - <a id="fx.sparks.breath.opts" name="fx.sparks.breath.opts"></a>**`opts`** (table). Where the breath is and which way it drifts.

    Keys:
    - <a id="fx.sparks.breath.opts.pos" name="fx.sparks.breath.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.breath.opts.vel" name="fx.sparks.breath.opts.vel"></a>**`vel`** ([trx.math.Vec3](MATH.md#math.Vec3), optional). How far it moves each frame. Defaults to standing still.

  Example:
  ```lua
  trx.fx.sparks.breath({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.pickup_aid" name="fx.sparks.pickup_aid"></a>[lua]`trx.fx.sparks.pickup_aid(opts)`  
  Throws the twinkle that marks a pickup worth reaching.

  Parameters:
  - <a id="fx.sparks.pickup_aid.opts" name="fx.sparks.pickup_aid.opts"></a>**`opts`** (table). Where the twinkle is and which way it drifts.

    Keys:
    - <a id="fx.sparks.pickup_aid.opts.pos" name="fx.sparks.pickup_aid.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.pickup_aid.opts.vel" name="fx.sparks.pickup_aid.opts.vel"></a>**`vel`** ([trx.math.Vec3](MATH.md#math.Vec3), optional). How far it moves each frame. Defaults to standing still.

  Example:
  ```lua
  trx.fx.sparks.pickup_aid({ pos = trx.lara.item.pos })
  ```

- <a id="fx.sparks.waterfall_mist" name="fx.sparks.waterfall_mist"></a>[lua]`trx.fx.sparks.waterfall_mist(opts)`  
  Lifts the mist that stands at the foot of a waterfall.

  Parameters:
  - <a id="fx.sparks.waterfall_mist.opts" name="fx.sparks.waterfall_mist.opts"></a>**`opts`** (table). Where the mist is and which way it faces.

    Keys:
    - <a id="fx.sparks.waterfall_mist.opts.pos" name="fx.sparks.waterfall_mist.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position. Must lie inside the level.
    - <a id="fx.sparks.waterfall_mist.opts.angle" name="fx.sparks.waterfall_mist.opts.angle"></a>**`angle`** ([trx.math.Angle](MATH.md#math.Angle)). The way the waterfall faces.

  Example:
  ```lua
  trx.fx.sparks.waterfall_mist({ pos = trx.lara.item.pos, angle = 0 })
  ```
