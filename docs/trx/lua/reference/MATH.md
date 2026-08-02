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

Fixed-point trigonometry, matching the engine's own tables.

TRX angles are 16-bit units where 65536 is a full turn, not radians. Using these rather than Lua's `math` library guarantees a script places things exactly where the engine would.

### Constants

- <a id="math.DEG_1" name="math.DEG_1"></a>[lua]`trx.math.DEG_1` = `182`  
  One degree in TRX units. Multiply by it to say an angle in degrees: `45 * trx.math.DEG_1`.

- <a id="math.DEG_45" name="math.DEG_45"></a>[lua]`trx.math.DEG_45` = `8192`  
  A 45-degree turn, in TRX units.

- <a id="math.DEG_90" name="math.DEG_90"></a>[lua]`trx.math.DEG_90` = `16384`  
  A quarter turn, in TRX units. A full turn is four of these, and wraps to zero.

- <a id="math.WALL_L" name="math.WALL_L"></a>[lua]`trx.math.WALL_L` = `1024`  
  The size of one sector in world units. Level geometry is laid out on this grid, so it is the step to take to move an item a sector over.

### Structures

- <a id="math.Box" name="math.Box"></a>[lua]`trx.math.Box`

    An axis-aligned box, in the units the engine measures the world in. Whether it is placed in world coordinates or in something's own frame is for whatever hands it over to say.

    Properties:
    - <a id="math.Box.max_x" name="math.Box.max_x"></a>**`max_x`**: integer. East edge.
    - <a id="math.Box.max_y" name="math.Box.max_y"></a>**`max_y`**: integer. Bottom edge.
    - <a id="math.Box.max_z" name="math.Box.max_z"></a>**`max_z`**: integer. South edge.
    - <a id="math.Box.min_x" name="math.Box.min_x"></a>**`min_x`**: integer. West edge.
    - <a id="math.Box.min_y" name="math.Box.min_y"></a>**`min_y`**: integer. Top edge. Y grows downwards.
    - <a id="math.Box.min_z" name="math.Box.min_z"></a>**`min_z`**: integer. North edge.

### Functions

- <a id="math.sin" name="math.sin"></a>[lua]`trx.math.sin(angle)`  
  Sine of an angle.

  Parameters:
  - <a id="math.sin.angle" name="math.sin.angle"></a>**`angle`** (integer). Angle in TRX units.

  Returns: number. A value in [-1, 1].

- <a id="math.cos" name="math.cos"></a>[lua]`trx.math.cos(angle)`  
  Cosine of an angle.

  Parameters:
  - <a id="math.cos.angle" name="math.cos.angle"></a>**`angle`** (integer). Angle in TRX units.

  Returns: number. A value in [-1, 1].

- <a id="math.atan" name="math.atan"></a>[lua]`trx.math.atan(z, x)`  
  Angle of the vector (x, z), in TRX units.

  Parameters:
  - <a id="math.atan.z" name="math.atan.z"></a>**`z`** (integer). How far the vector reaches north.
  - <a id="math.atan.x" name="math.atan.x"></a>**`x`** (integer). How far it reaches east.

  Returns: integer. In TRX units.

  Example:
  ```lua
  -- face an item towards Lara
  local angle = trx.math.atan(lara.pos.z - pos.z, lara.pos.x - pos.x)
  ```
