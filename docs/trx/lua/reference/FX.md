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

Nothing here takes a place in a save or has a bearing on what anything decides.

Every call here lasts the frame it is made in. A light that stays is the same
call made every frame, which is also how one follows something that moves;
there is nothing to remove.

A frame shows only so much: see [`trx.fx.MAX_LIGHTS`](#fx.MAX_LIGHTS) and [`trx.fx.MAX_FOG`](#fx.MAX_FOG). Once
a frame is full, what is asked for takes the place of the one furthest from the
camera, so the nearest are the ones seen. Neither raises an error.

### Constants

- <a id="fx.MAX_LIGHTS" name="fx.MAX_LIGHTS"></a>[lua]`trx.fx.MAX_LIGHTS` = `64` (integer)  
  How many lights a script can put up in one frame.

- <a id="fx.MAX_FOG" name="fx.MAX_FOG"></a>[lua]`trx.fx.MAX_FOG` = `10` (integer)  
  How many balls of fog can be seen at once. TR4 levels carry fog of their own, which takes its slots first, so fewer than this reach the screen where a level is already using them.

### Functions

- <a id="fx.emit_light" name="fx.emit_light"></a>[lua]`trx.fx.emit_light(opts)`  
  Lights the world around a point for this frame.

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
  space itself.

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
