local raw = trxc.ui
local api = trx.api

require("trx.ui")

-------------------------------------------------------------------------------
-- The primitives
--
-- The drawing primitives the engine exposes to Lua, plus reservations for
-- things Lua lays out itself. Higher-level widget behavior lives in Lua.
--
-- Widgets use these directly. Scripts should only do so when they also reserve
-- the space they draw into, otherwise regions cannot include it in layout.
-------------------------------------------------------------------------------

api.namespace("ui.primitive", {
  description = [[
Low-level drawing calls and layout reservations.

Use `trx.ui.widgets` for normal UI. Use these primitives only when building a
custom widget. Primitive drawing does not affect region layout unless code
reserves space first.

Drawing calls are available only during `trx.events.on_ui_paint`. They report
an error at any other time.]],
})

api.define("ui.primitive.reserve", {
  description = [[
Reserves space in a region and returns a slot for it.

The reservation is stacked with the engine UI in that region. Reserve space
during `trx.events.on_ui_draw`, then read the assigned box during
`trx.events.on_ui_paint`.

A slot is valid only for the scene that created it.]],
  params = {
    {
      name = "region",
      type = "ui.Region",
      description = "Which region to keep room in.",
    },
    {
      name = "w",
      type = "number",
      description = "How wide, in canvas units.",
    },
    {
      name = "h",
      type = "number",
      description = "How tall, in canvas units.",
    },
  },
  returns = { type = "integer", description = "The slot." },
  impl = raw.reserve,
})

api.define("ui.primitive.slot_box", {
  description = "Returns the box assigned to a reservation by the last layout.",
  params = {
    {
      name = "slot",
      type = "integer",
      description = "The reservation slot.",
    },
  },
  returns = {
    {
      type = "number",
      description = "The left edge, or `nil` when the slot is no longer valid.",
    },
    { type = "number", description = "The top edge." },
    { type = "number", description = "The width." },
    { type = "number", description = "The height." },
  },
  impl = raw.slot_box,
})

api.define("ui.primitive.measure_text", {
  description = "Measures one line of text. Available at any time.",
  params = {
    { name = "text", type = "string", description = "What to measure." },
    {
      name = "scale",
      type = "number",
      optional = true,
      description = "Multiplies the text size. `1.0` by default.",
    },
  },
  returns = {
    { type = "number", description = "The width, in canvas units." },
    { type = "number", description = "The height, in canvas units." },
  },
  impl = raw.measure_text,
})

api.define("ui.primitive.text", {
  description = "Draws one line of text on the canvas.",
  params = {
    { name = "text", type = "string", description = "What to draw." },
    { name = "x", type = "number", description = "The left edge." },
    { name = "y", type = "number", description = "The top edge." },
    {
      name = "scale",
      type = "number",
      optional = true,
      description = "Multiplies the text size.",
    },
    {
      name = "z",
      type = "integer",
      optional = true,
      description = "The draw order.",
    },
  },
  impl = raw.draw_text,
})

api.define("ui.primitive.to_screen", {
  description = [[
Converts a canvas length to screen pixels.

The canvas is a fixed 640x480 grid, and the screen size depends on the player
settings and window. Use this with `trx.ui.primitive.to_canvas` when geometry
must align to whole screen pixels, such as an even border.]],
  params = {
    { name = "length", type = "number", description = "A canvas length." },
  },
  returns = {
    type = "number",
    description = "The same length in screen pixels.",
  },
  impl = raw.to_screen,
})

api.define("ui.primitive.to_canvas", {
  description = [[
Converts a screen-pixel length to canvas units.

Use this with `trx.ui.primitive.to_screen` when geometry must align to whole
screen pixels.]],
  params = {
    {
      name = "pixels",
      type = "number",
      description = "A length in screen pixels.",
    },
  },
  returns = {
    type = "number",
    description = "The same length in canvas units.",
  },
  impl = raw.to_canvas,
})

api.define("ui.primitive.quad", {
  description = "Draws a rectangle of one color.",
  params = {
    { name = "x", type = "number", description = "The left edge." },
    { name = "y", type = "number", description = "The top edge." },
    {
      name = "z",
      type = "integer",
      description = "The draw order.",
    },
    { name = "w", type = "number", description = "The width." },
    { name = "h", type = "number", description = "The height." },
    {
      name = "color",
      type = "math.Color",
      description = "What color to fill it with.",
    },
  },
  impl = raw.flat_quad,
})

api.define("ui.primitive.gradient_quad", {
  description = "Draws a rectangle whose corners each carry a color.",
  params = {
    { name = "x", type = "number", description = "The left edge." },
    { name = "y", type = "number", description = "The top edge." },
    {
      name = "z",
      type = "integer",
      description = "What to draw in front of.",
    },
    { name = "w", type = "number", description = "The width." },
    { name = "h", type = "number", description = "The height." },
    { name = "tl", type = "math.Color", description = "The top-left color." },
    { name = "tr", type = "math.Color", description = "The top-right color." },
    {
      name = "bl",
      type = "math.Color",
      description = "The bottom-left color.",
    },
    {
      name = "br",
      type = "math.Color",
      description = "The bottom-right color.",
    },
  },
  impl = raw.gradient_quad,
})

-- The sprite primitives share their leading parameters and differ only in how
-- many colors they take.
local function sprite_params(...)
  local params = {
    {
      name = "object",
      type = "catalog.objects",
      description = "The sprite object to draw from.",
    },
    {
      name = "sprite_num",
      type = "integer",
      description = "Which sprite of the object to draw, counted from 0.",
    },
    { name = "x", type = "number", description = "The left edge." },
    { name = "y", type = "number", description = "The top edge." },
    { name = "z", type = "integer", description = "The draw order." },
    {
      name = "scale",
      type = "number",
      description = "Multiplies the sprite size. At 1 the sprite draws at its "
        .. "own size on the canvas.",
    },
  }
  for _, color in ipairs({ ... }) do
    params[#params + 1] = {
      name = color[1],
      type = "math.Color",
      description = color[2],
    }
  end
  return params
end

api.define("ui.primitive.sprite_count", {
  description = [[
Reports how many sprites an object has.

An object the level did not load has none, so this answers whether there is
anything to draw before `trx.ui.primitive.sprite` is asked to draw it.]],
  params = {
    {
      name = "object",
      type = "catalog.objects",
      description = "The sprite object to count.",
    },
  },
  returns = { type = "integer", description = "How many sprites it has." },
  impl = raw.sprite_count,
})

api.define("ui.primitive.sprite_bounds", {
  description = [[
Reports the edges of one sprite of an object, in canvas units at a scale of one.

The edges sit around the point the sprite is drawn at, so both left and top are
usually negative. Multiply them by the scale the sprite is drawn at.

Raises where the level did not load the object, so check
`trx.objects.get(object).loaded` first.]],
  params = {
    {
      name = "object",
      type = "catalog.objects",
      description = "The sprite object to read from.",
    },
    {
      name = "sprite_num",
      type = "integer",
      description = "Which sprite of the object to read, counted from 0.",
    },
  },
  returns = {
    { type = "number", description = "The left edge." },
    { type = "number", description = "The top edge." },
    { type = "number", description = "The right edge." },
    { type = "number", description = "The bottom edge." },
  },
  impl = raw.sprite_bounds,
})

api.define("ui.primitive.sprite", {
  description = [[
Draws one sprite of an object on the canvas.

Raises where the level did not load the object, so check
`trx.objects.get(object).loaded` first.]],
  params = sprite_params({ "color", "What color to tint it with." }),
  examples = {
    [[trx.ui.primitive.sprite(
  trx.catalog.objects.assault_digits, 3, 100, 20, 0, 1,
  trx.math.color("ffffff"))]],
  },
  impl = raw.sprite,
})

api.define("ui.primitive.gradient_sprite", {
  description = [[
Draws one sprite of an object, with a color at each corner.

Raises where the level did not load the object, so check
`trx.objects.get(object).loaded` first.]],
  params = sprite_params(
    { "tl", "The top-left color." },
    { "tr", "The top-right color." },
    { "bl", "The bottom-left color." },
    { "br", "The bottom-right color." }
  ),
  impl = raw.gradient_sprite,
})
