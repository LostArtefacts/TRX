---
title: Math
order: 31
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

    An axis-aligned box. Whether it is placed in the world or in something's own frame is for the call that hands it over to say.

    Properties:
    - <a id="math.Box.max_x" name="math.Box.max_x"></a>**`max_x`**: [trx.math.Distance](#math.Distance). East edge.
    - <a id="math.Box.max_y" name="math.Box.max_y"></a>**`max_y`**: [trx.math.Distance](#math.Distance). Bottom edge.
    - <a id="math.Box.max_z" name="math.Box.max_z"></a>**`max_z`**: [trx.math.Distance](#math.Distance). North edge.
    - <a id="math.Box.min_x" name="math.Box.min_x"></a>**`min_x`**: [trx.math.Distance](#math.Distance). West edge.
    - <a id="math.Box.min_y" name="math.Box.min_y"></a>**`min_y`**: [trx.math.Distance](#math.Distance). Top edge.
    - <a id="math.Box.min_z" name="math.Box.min_z"></a>**`min_z`**: [trx.math.Distance](#math.Distance). South edge.

- <a id="math.Color" name="math.Color"></a>[lua]`trx.math.Color`

    A color, as three channels counted 0 to 255.

    Assigning one takes either a color or the hex text a color is written as, so
    `"33e5ff"` and `{ r = 51, g = 229, b = 255 }` say the same thing. A channel may
    also be written on its own, and a color read off something the engine owns
    writes that change straight back to it.

    Some colors the engine keeps are stored as fractions rather than bytes, and
    those carry more precision than the hex text shows: a channel of one may read
    back as `191.25`.

    Properties:
    - <a id="math.Color.b" name="math.Color.b"></a>**`b`**: number. The blue channel.
    - <a id="math.Color.g" name="math.Color.g"></a>**`g`**: number. The green channel.
    - <a id="math.Color.hex" name="math.Color.hex"></a>**`hex`**: string. The color as six hex digits, which is how a setting and a data file spell one. Writing it takes a leading `#` as well.
    - <a id="math.Color.r" name="math.Color.r"></a>**`r`**: number. The red channel.

    Operators:
    - **`color .. color`**. A color joins text as its hex, whichever side of the `..` it is on.
    - **`color == color`**. Two colors are equal when their channels are.
    - **`tostring(color)`**. The color as its hex text.

### Functions

- <a id="math.color" name="math.color"></a>[lua]`trx.math.color(value, [g], [b])`  
  Builds a color, out of three channels or out of hex text. The color it hands back belongs to the caller: assign it somewhere for the engine to take it.

  Parameters:
  - <a id="math.color.value" name="math.color.value"></a>**`value`** (string or number). The hex text, or the red channel.
  - <a id="math.color.g" name="math.color.g"></a>**`g`** (number, optional). The green channel, where the first argument was the red one.
  - <a id="math.color.b" name="math.color.b"></a>**`b`** (number, optional). The blue channel.

  Returns: [trx.math.Color](#math.Color).

  Example:
  ```lua
  local gold = trx.math.color("ffbf20")
  local teal = trx.math.color(51, 229, 255)
  ```

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

- <a id="math.round_to_sector" name="math.round_to_sector"></a>[lua]`trx.math.round_to_sector(value)`  
  Snaps a position back to the corner of the sector it stands in, the way the
  level's own geometry is laid out. A whole position keeps its height: a sector
  is a column, and rounding it is about the ground plan rather than how far up
  the position sits. A single coordinate rounds on its own, which is what an axis
  at a time needs.

  The corner is always the one to the west and the south, on both sides of the
  origin, so two positions in the same sector always answer with the same corner.

  Parameters:
  - <a id="math.round_to_sector.value" name="math.round_to_sector.value"></a>**`value`** ([trx.math.Vec3](#math.Vec3) or [trx.math.Distance](#math.Distance)). A world position, or one coordinate of one.

  Returns: [trx.math.Vec3](#math.Vec3) or [trx.math.Distance](#math.Distance). The corner of the sector, in whichever of the two came in.

  Example:
  ```lua
  -- a zone over the sector Lara stands on, a sector tall.
  -- y grows downwards, so the ceiling of the box is the lesser y.
  local corner = trx.math.round_to_sector(trx.lara.item.pos)
  trx.zones.box(corner, {
    x = corner.x + trx.math.WALL_L,
    y = corner.y - trx.math.WALL_L,
    z = corner.z + trx.math.WALL_L,
  })
  ```
