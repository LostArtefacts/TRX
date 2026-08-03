---
title: Math
order: 27
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/math.lua. Edit it there.
-->

## <a id="math" name="math"></a>Math module

Fixed-point trigonometry, matching the engine's own tables. Using these rather than Lua's `math` library guarantees a script places things exactly where the engine would. [`trx.math.Angle`](#math.Angle) says what an angle is here.

### Constants

- <a id="math.DEG_1" name="math.DEG_1"></a>[lua]`trx.math.DEG_1` = `182` ([trx.math.Angle](#math.Angle))  
  One degree. Multiply by it to say an angle in degrees: `45 * trx.math.DEG_1`.

- <a id="math.DEG_45" name="math.DEG_45"></a>[lua]`trx.math.DEG_45` = `8192` ([trx.math.Angle](#math.Angle))  
  A 45-degree turn.

- <a id="math.DEG_90" name="math.DEG_90"></a>[lua]`trx.math.DEG_90` = `16384` ([trx.math.Angle](#math.Angle))  
  A quarter turn. A full turn is four of these.

- <a id="math.WALL_L" name="math.WALL_L"></a>[lua]`trx.math.WALL_L` = `1024` ([trx.math.Distance](#math.Distance))  
  The size of one sector. Level geometry is laid out on this grid, so it is the step to take to move an item a sector over.

### Structures

- <a id="math.Angle" name="math.Angle"></a>[lua]`trx.math.Angle` (integer)

    An angle in the engine's own units, where 65536 is a full turn rather than
    2 pi. An angle counts in cycles, so one past the end of a turn wraps round
    to name the same direction: adding a half turn to a rotation always works.
    [`trx.math.DEG_1`](#math.DEG_1) converts from degrees.

- <a id="math.Distance" name="math.Distance"></a>[lua]`trx.math.Distance` (integer)

    A length in the units the engine measures the world in, where one sector is
    [`trx.math.WALL_L`](#math.WALL_L). Y grows downwards, so a greater Y is further down.

- <a id="math.Vec3" name="math.Vec3"></a>[lua]`trx.math.Vec3`

    A point or a direction in the world.

    Properties:
    - <a id="math.Vec3.x" name="math.Vec3.x"></a>**`x`**: [trx.math.Distance](#math.Distance). The east-west axis.
    - <a id="math.Vec3.y" name="math.Vec3.y"></a>**`y`**: [trx.math.Distance](#math.Distance). The up-down axis.
    - <a id="math.Vec3.z" name="math.Vec3.z"></a>**`z`**: [trx.math.Distance](#math.Distance). The north-south axis.

- <a id="math.Rot" name="math.Rot"></a>[lua]`trx.math.Rot`

    An orientation, as three angles about the world axes.

    Properties:
    - <a id="math.Rot.x" name="math.Rot.x"></a>**`x`**: [trx.math.Angle](#math.Angle). Pitch, nose up and down.
    - <a id="math.Rot.y" name="math.Rot.y"></a>**`y`**: [trx.math.Angle](#math.Angle). Yaw, the direction it faces.
    - <a id="math.Rot.z" name="math.Rot.z"></a>**`z`**: [trx.math.Angle](#math.Angle). Roll, the tilt about its own length.

- <a id="math.Box" name="math.Box"></a>[lua]`trx.math.Box`

    An axis-aligned box. Whether it is placed in the world or in something's own frame is for whatever hands it over to say.

    Properties:
    - <a id="math.Box.max_x" name="math.Box.max_x"></a>**`max_x`**: [trx.math.Distance](#math.Distance). East edge.
    - <a id="math.Box.max_y" name="math.Box.max_y"></a>**`max_y`**: [trx.math.Distance](#math.Distance). Bottom edge.
    - <a id="math.Box.max_z" name="math.Box.max_z"></a>**`max_z`**: [trx.math.Distance](#math.Distance). South edge.
    - <a id="math.Box.min_x" name="math.Box.min_x"></a>**`min_x`**: [trx.math.Distance](#math.Distance). West edge.
    - <a id="math.Box.min_y" name="math.Box.min_y"></a>**`min_y`**: [trx.math.Distance](#math.Distance). Top edge.
    - <a id="math.Box.min_z" name="math.Box.min_z"></a>**`min_z`**: [trx.math.Distance](#math.Distance). North edge.

### Functions

- <a id="math.sin" name="math.sin"></a>[lua]`trx.math.sin(angle)`  
  Sine of an angle.

  Parameters:
  - <a id="math.sin.angle" name="math.sin.angle"></a>**`angle`** ([trx.math.Angle](#math.Angle)).

  Returns: number. A value in [-1, 1].

- <a id="math.cos" name="math.cos"></a>[lua]`trx.math.cos(angle)`  
  Cosine of an angle.

  Parameters:
  - <a id="math.cos.angle" name="math.cos.angle"></a>**`angle`** ([trx.math.Angle](#math.Angle)).

  Returns: number. A value in [-1, 1].

- <a id="math.atan" name="math.atan"></a>[lua]`trx.math.atan(z, x)`  
  Angle of the vector (x, z).

  Parameters:
  - <a id="math.atan.z" name="math.atan.z"></a>**`z`** ([trx.math.Distance](#math.Distance)). How far the vector reaches north.
  - <a id="math.atan.x" name="math.atan.x"></a>**`x`** ([trx.math.Distance](#math.Distance)). How far it reaches east.

  Returns: [trx.math.Angle](#math.Angle).

  Example:
  ```lua
  -- face an item towards Lara
  local angle = trx.math.atan(lara.pos.z - pos.z, lara.pos.x - pos.x)
  ```
