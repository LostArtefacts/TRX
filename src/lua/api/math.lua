local raw = trxc.math
local api = trx.api

api.module("math", {
  order = 27,
  description = "Fixed-point trigonometry, matching the engine's own tables. Using these rather "
    .. "than Lua's `math` library guarantees a script places things exactly where the engine "
    .. "would. `trx.math.Angle` says what an angle is here.",
})

api.unit("math.Angle", {
  description = [[
    An angle in the engine's own units, where 65536 is a full turn rather than
    2 pi. An angle counts in cycles, so one past the end of a turn wraps round
    to name the same direction: adding a half turn to a rotation always works.
    `trx.math.DEG_1` converts from degrees.
  ]],
  spellings = { "TRX units", "angle units" },
})

api.unit("math.Distance", {
  description = [[
    A length in the units the engine measures the world in, where one sector is
    `trx.math.WALL_L`. Y grows downwards, so a greater Y is further down.
  ]],
  spellings = { "world units", "world coordinates" },
})

api.type("math.Vec3", {
  record = true,
  description = "A point or a direction in the world.",
  fields = {
    x = { type = "math.Distance", description = "The east-west axis." },
    y = { type = "math.Distance", description = "The up-down axis." },
    z = { type = "math.Distance", description = "The north-south axis." },
  },
})

api.type("math.Rot", {
  record = true,
  description = "An orientation, as three angles about the world axes.",
  fields = {
    x = { type = "math.Angle", description = "Pitch, nose up and down." },
    y = { type = "math.Angle", description = "Yaw, the direction it faces." },
    z = {
      type = "math.Angle",
      description = "Roll, the tilt about its own length.",
    },
  },
})

api.type("math.Box", {
  description = "An axis-aligned box. Whether it is placed in the world or in something's own "
    .. "frame is for whatever hands it over to say.",
  fields = {
    min_x = { type = "math.Distance", description = "West edge." },
    min_y = { type = "math.Distance", description = "Top edge." },
    min_z = { type = "math.Distance", description = "North edge." },
    max_x = { type = "math.Distance", description = "East edge." },
    max_y = { type = "math.Distance", description = "Bottom edge." },
    max_z = { type = "math.Distance", description = "South edge." },
  },
})

api.define("math.sin", {
  description = "Sine of an angle.",
  params = {
    { name = "angle", type = "math.Angle" },
  },
  returns = { type = "number", description = "A value in [-1, 1]." },
  impl = raw.sin,
})

api.define("math.cos", {
  description = "Cosine of an angle.",
  params = {
    { name = "angle", type = "math.Angle" },
  },
  returns = { type = "number", description = "A value in [-1, 1]." },
  impl = raw.cos,
})

api.define("math.atan", {
  description = "Angle of the vector (x, z).",
  params = {
    {
      name = "z",
      type = "math.Distance",
      description = "How far the vector reaches north.",
    },
    {
      name = "x",
      type = "math.Distance",
      description = "How far it reaches east.",
    },
  },
  returns = { type = "math.Angle" },
  examples = {
    [[-- face an item towards Lara
local angle = trx.math.atan(lara.pos.z - pos.z, lara.pos.x - pos.x)]],
  },
  impl = raw.atan,
})

api.const("math.DEG_1", {
  value = raw.DEG_1,
  type = "math.Angle",
  description = "One degree. Multiply by it to say an angle in degrees: `45 * trx.math.DEG_1`.",
})

api.const("math.DEG_45", {
  value = raw.DEG_45,
  type = "math.Angle",
  description = "A 45-degree turn.",
})

api.const("math.DEG_90", {
  value = raw.DEG_90,
  type = "math.Angle",
  description = "A quarter turn. A full turn is four of these.",
})

api.const("math.WALL_L", {
  value = raw.WALL_L,
  type = "math.Distance",
  description = "The size of one sector. Level geometry is laid out on this grid, so it is the "
    .. "step to take to move an item a sector over.",
})
