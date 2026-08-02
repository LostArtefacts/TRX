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

## Math module

Fixed-point trigonometry, matching the engine's own tables.

TRX angles are 16-bit units where 65536 is a full turn, not radians. Using these rather than Lua's `math` library guarantees a script places things exactly where the engine would.

### Constants

- <a name="math.DEG_1"></a>[lua]`trx.math.DEG_1` = `182`  
  One degree in TRX units. Multiply by it to say an angle in degrees: `45 * trx.math.DEG_1`.

- <a name="math.DEG_45"></a>[lua]`trx.math.DEG_45` = `8192`  
  A 45-degree turn, in TRX units.

- <a name="math.DEG_90"></a>[lua]`trx.math.DEG_90` = `16384`  
  A quarter turn, in TRX units. A full turn is four of these, and wraps to zero.

- <a name="math.WALL_L"></a>[lua]`trx.math.WALL_L` = `1024`  
  The size of one sector in world units. Level geometry is laid out on this grid, so it is the step to take to move an item a sector over.

### Functions

- <a name="math.sin"></a>[lua]`trx.math.sin(angle)`  
  Sine of an angle.

  Parameters:
  - **`angle`** (integer). Angle in TRX units.

  Returns: number. A value in [-1, 1].

- <a name="math.cos"></a>[lua]`trx.math.cos(angle)`  
  Cosine of an angle.

  Parameters:
  - **`angle`** (integer). Angle in TRX units.

  Returns: number. A value in [-1, 1].

- <a name="math.atan"></a>[lua]`trx.math.atan(z, x)`  
  Angle of the vector (x, z), in TRX units.

  Parameters:
  - **`z`** (integer).
  - **`x`** (integer).

  Returns: integer.

  Example:
  ```lua
  -- face an item towards Lara
  local angle = trx.math.atan(lara.pos.z - pos.z, lara.pos.x - pos.x)
  ```
