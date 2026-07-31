local raw = trxc.math
local api = trx.api

api.module("math", {
  order = 27,
  description = "Fixed-point trigonometry, matching the engine's own tables.\n\n"
    .. "TRX angles are 16-bit units where 65536 is a full turn, not radians. Using these rather "
    .. "than Lua's `math` library guarantees a script places things exactly where the engine "
    .. "would.",
})

api.define("math.sin", {
  description = "Sine of an angle.",
  params = {
    { name = "angle", type = "integer", description = "Angle in TRX units." },
  },
  returns = { type = "number", description = "A value in [-1, 1]." },
  impl = raw.sin,
})

api.define("math.cos", {
  description = "Cosine of an angle.",
  params = {
    { name = "angle", type = "integer", description = "Angle in TRX units." },
  },
  returns = { type = "number", description = "A value in [-1, 1]." },
  impl = raw.cos,
})

api.define("math.atan", {
  description = "Angle of the vector (x, z), in TRX units.",
  params = {
    { name = "z", type = "integer" },
    { name = "x", type = "integer" },
  },
  returns = { type = "integer" },
  examples = {
    [[-- face an item towards Lara
local angle = trx.math.atan(lara.pos.z - pos.z, lara.pos.x - pos.x)]],
  },
  impl = raw.atan,
})

api.const("math.DEG_1", {
  value = raw.DEG_1,
  description = "One degree in TRX units. Multiply by it to say an angle in degrees: `45 * trx.math.DEG_1`.",
})

api.const("math.DEG_45", {
  value = raw.DEG_45,
  description = "A 45-degree turn, in TRX units.",
})

api.const("math.DEG_90", {
  value = raw.DEG_90,
  description = "A quarter turn, in TRX units. A full turn is four of these, and wraps to zero.",
})

api.const("math.WALL_L", {
  value = raw.WALL_L,
  description = "The size of one sector in world units. Level geometry is laid out on this grid, "
    .. "so it is the step to take to move an item a sector over.",
})
