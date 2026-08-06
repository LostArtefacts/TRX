local raw = trxc.fx
local api = trx.api

require("trx.math")

api.module("fx", {
  order = 17,
  description = [[
What a script puts in front of the player: things seen rather than things the
game holds.

Nothing here takes a place in a save or has a bearing on what anything decides.

Every call here lasts the frame it is made in. A light that stays is the same
call made every frame, which is also how one follows something that moves;
there is nothing to remove.

A frame shows only so much: see `trx.fx.MAX_LIGHTS` and `trx.fx.MAX_FOG`. Once
a frame is full, what is asked for takes the place of the one furthest from the
camera, so the nearest are the ones seen. Neither raises an error.]],
})

local pos_field =
  { name = "pos", type = "math.Vec3", description = "World position." }

local color_field = {
  name = "color",
  type = "table",
  optional = true,
  description = "Its color, each channel 0 to 255. Defaults to white.",
  fields = {
    { name = "r", type = "integer", description = "Red." },
    { name = "g", type = "integer", description = "Green." },
    { name = "b", type = "integer", description = "Blue." },
  },
}

local WHITE = { r = 255, g = 255, b = 255 }

api.define("fx.emit_light", {
  description = [[
Lights the world around a point for this frame.

TR1 and TR2 light in brightness alone, so there the light is as bright as its
brightest channel and comes out white.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the light is and what it looks like.",
      fields = {
        pos_field,
        {
          name = "radius",
          type = "math.Distance",
          optional = true,
          default = 3072,
          description = "How far it reaches, at least an eighth of a sector. "
            .. "Rounded down to the eighth of a sector the engine measures a "
            .. "dynamic light in, and carried about a quarter further than "
            .. "asked for in TR4, which falls a light off more gently.",
        },
        color_field,
      },
    },
  },
  examples = {
    [[trx.events.after_control(function()
  trx.fx.emit_light({
    pos = trx.lara.item.pos,
    radius = 2048,
    color = { r = 0, g = 255, b = 192 },
  })
end)]],
  },
  impl = function(opts)
    local color = opts.color or WHITE
    raw.emit_light(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      color.r,
      color.g,
      color.b,
      -- The engine counts a dynamic light's reach in eighths of a sector.
      math.max(1, (opts.radius or 3072) // 128)
    )
  end,
})

api.define("fx.emit_fog", {
  description = [[
Fills a ball of air with fog for this frame, which the player sees through
rather than on. Where a light brightens what it falls on, this hangs in the
space itself.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the fog is and what it looks like.",
      fields = {
        pos_field,
        {
          name = "radius",
          type = "math.Distance",
          optional = true,
          default = 2048,
          description = "How far the fog reaches, at least one unit.",
        },
        {
          name = "density",
          type = "integer",
          optional = true,
          default = 128,
          description = "How thick it is, from 0 for nothing to 255.",
        },
        color_field,
      },
    },
  },
  examples = {
    [[trx.events.after_control(function()
  trx.fx.emit_fog({
    pos = { x = 32768, y = -1024, z = 45056 },
    radius = 3072,
    density = 64,
    color = { r = 128, g = 160, b = 192 },
  })
end)]],
  },
  impl = function(opts)
    local color = opts.color or WHITE
    raw.emit_fog(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      color.r,
      color.g,
      color.b,
      opts.radius or 2048,
      opts.density or 128
    )
  end,
})

api.const("fx.MAX_LIGHTS", {
  value = raw.MAX_LIGHTS,
  type = "integer",
  description = "How many lights a script can put up in one frame.",
})

api.const("fx.MAX_FOG", {
  value = raw.MAX_FOG,
  type = "integer",
  description = "How many balls of fog can be seen at once. TR4 levels carry "
    .. "fog of their own, which takes its slots first, so fewer than this "
    .. "reach the screen where a level is already using them.",
})
