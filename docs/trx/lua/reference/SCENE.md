---
title: Scene
order: 44
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/trx/scene.lua. Edit it there.
-->

## <a id="scene" name="scene"></a>Scene module

Outlines a script draws into the world the camera is looking at, over the level
geometry rather than over the interface.

The calls are available from [`trx.events.on_scene_paint`](EVENTS.md#events.on_scene_paint) and nowhere else, and
raise anywhere else. Nothing is remembered between frames: a shape that is to
stay on screen is drawn again every time the event fires.

A shape is placed the way an item position and a zone are, so it needs no room
and belongs to none. The outlines are drawn as wireframe, and one reaching
further from its middle than a level is wide draws at that limit instead.

### Functions

- <a id="scene.box" name="scene.box"></a>[lua]`trx.scene.box(min, max, color, [alpha])`  
  Draws the outline of a world-space box. The corners may come in any order.

  Parameters:
  - <a id="scene.box.min" name="scene.box.min"></a>**`min`** ([trx.math.Vec3](MATH.md#math.Vec3)). One corner of the box.
  - <a id="scene.box.max" name="scene.box.max"></a>**`max`** ([trx.math.Vec3](MATH.md#math.Vec3)). The opposite corner of the box.
  - <a id="scene.box.color" name="scene.box.color"></a>**`color`** ([trx.math.Color](MATH.md#math.Color)). The color of the outline.
  - <a id="scene.box.alpha" name="scene.box.alpha"></a>**`alpha`** (integer, optional, default `255`). How solid the outline is, counted 0 to 255.

  Example:
  ```lua
  trx.events.on_scene_paint(function()
    trx.scene.box(
      { x = 51200, y = -2048, z = 30720 },
      { x = 53248, y = 0, z = 32768 },
      "00ff00")
  end)
  ```

- <a id="scene.sphere" name="scene.sphere"></a>[lua]`trx.scene.sphere(centre, radius, color, [alpha])`  
  Draws the outline of a sphere.

  Parameters:
  - <a id="scene.sphere.centre" name="scene.sphere.centre"></a>**`centre`** ([trx.math.Vec3](MATH.md#math.Vec3)). Middle of the sphere.
  - <a id="scene.sphere.radius" name="scene.sphere.radius"></a>**`radius`** ([trx.math.Distance](MATH.md#math.Distance)). How far out it reaches.
  - <a id="scene.sphere.color" name="scene.sphere.color"></a>**`color`** ([trx.math.Color](MATH.md#math.Color)). The color of the outline.
  - <a id="scene.sphere.alpha" name="scene.sphere.alpha"></a>**`alpha`** (integer, optional, default `255`). How solid the outline is, counted 0 to 255.

  Example:
  ```lua
  trx.events.on_scene_paint(function()
    trx.scene.sphere(trx.lara.item.pos, 2048, "00ff00", 128)
  end)
  ```
