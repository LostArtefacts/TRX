local raw = trxc.fx
local api = trx.api

require("trx.math")
require("trx.rooms")

api.module("fx", {
  order = 17,
  description = [[
What a script puts in front of the player: things seen rather than things the
game holds.]],
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
Lights the world around a point for this frame. Make the call every frame to
keep the light up, and at a new position each time to move it.

A frame shows only `trx.fx.MAX_LIGHTS` lights. A light asked for past that
takes the place of the one furthest from the camera, so the nearest are the
ones seen. No error is raised.

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
space itself. Make the call every frame to keep the fog up.

A frame shows only `trx.fx.MAX_FOG` balls of fog. A ball asked for past that
takes the place of the one furthest from the camera, so the nearest are the
ones seen. No error is raised.]],
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

api.type("fx.FogBulb", {
  backing = "FOG_BULB",
  description = [[A level fog bulb is a ball of fog drawn inside a room.

TR4 stores fog bulbs as room lights. A script can change their color and density.
The level sets their position and radius. A bulb follows the fog color until a script
gives it a color of its own.]],
  fields = {
    color = {
      type = "math.Color",
      nullable = true,
      description = [[The color a script gave the bulb.

`nil` means none was given, and the bulb is drawn in the fog color in force. Write `nil` to hand
a bulb back to that color.]],
    },
    density = {
      from = "density",
      type = "integer",
      description = "Fog density, from `0` for none to `255`. A value outside this "
        .. "range raises an error.",
    },
    pos = {
      from = "pos",
      type = "math.Vec3",
      writable = false,
      description = "Center of the fog bulb.",
    },
    radius = {
      from = "radius",
      type = "math.Distance",
      writable = false,
      description = "How far the fog reaches from that position.",
    },
  },

  extensions = {
    room = {
      type = "rooms.Room",
      description = "The room the bulb sits in.",
      impl = function(bulb)
        local num = raw.get_fog_bulb_room(bulb)
        return num and trx.rooms[num] or nil
      end,
    },
  },

  methods = {
    is_valid = {
      returns = {
        type = "boolean",
        description = "False after the level that held the bulb is left.",
      },
      description = [[Reports whether the handle still names a bulb in the loaded level.

A level change replaces all bulbs. A handle held across one becomes stale, and field access
raises an error.]],
    },
  },
})

api.container("fx.fog_bulbs", {
  description = [[The level fog bulbs, counted from 1. `#trx.fx.fog_bulbs` is the count.
`pairs()` walks them in order. A level shows at most twenty. The player can turn them off.]],
  key = { type = "integer" },
  value = { type = "fx.FogBulb", nullable = true },
  examples = {
    [[for _, bulb in pairs(trx.fx.fog_bulbs) do
  bulb.color = trx.math.color(245, 200, 60)
end]],
  },
  get = function(idx)
    return raw.get_fog_bulb(idx - 1)
  end,
  count = raw.fog_bulb_count,
})

api.property("fx.fog_color", {
  type = "math.Color",
  nullable = true,
  description = [[The color override for distance fog.

`nil` means no override. Write `nil` to restore the level fog color. A level change clears the
override. Savegames keep it. This controls distance fog only. Fog bulbs have their own colors
in `trx.fx.fog_bulbs`.]],
  get = raw.get_fog_color,
  set = raw.set_fog_color,
})

local blood_pos_field = {
  name = "pos",
  type = "math.Vec3",
  description = "World position. Must lie inside the level.",
}

local blood_angle_field = {
  name = "angle",
  type = "math.Angle",
  optional = true,
  description = "The way the drops fly. Left out, TR4 throws them every way, "
    .. "which is what its own hits do where nothing aims them; the other "
    .. "games read it as straight ahead.",
}

local blood_strength_field = {
  name = "strength",
  type = "integer",
  optional = true,
  default = 5,
  description = "How heavy the hit reads, from 1 to 255. TR3 and TR4 count "
    .. "it in drops, TR4 measures the width of a cloud under water with it, "
    .. "and TR1 and TR2 have one drifting sprite whose speed it sets.",
}

api.define("fx.blood", {
  description = [[
Throws a spray of blood into the world at a point, the way a blow that lands
does. The drops then fall on their own.

TR3 and TR4 throw drops that fall and darken as they go, and in TR4 a hit under
water spreads as a cloud instead. TR1 and TR2 have one blood sprite that drifts
up.

Unlike the rest of the module, this has a bearing on what the game decides. The
engine places the drops from the control random stream, and in TR1 and TR2 the
spray takes a slot from the effect pool a save holds.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the blood is and how much of it.",
      fields = {
        blood_pos_field,
        blood_angle_field,
        blood_strength_field,
      },
    },
  },
  examples = {
    [[trx.events.on_hit(function(item, damage)
  trx.fx.blood({ pos = item.pos, angle = item.rot.y, strength = damage })
end)]],
  },
  impl = function(opts)
    raw.blood(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.strength or 5,
      opts.angle or -1
    )
  end,
})

api.define("fx.blood_bath", {
  description = [[
Throws several sprays of blood about a point, the way a trap that kills does.
Each one lands anywhere in the half-sector box around the point, so the blood
covers a body rather than a spot.

Each spray costs what `trx.fx.blood` costs, in random draws and in effect
slots.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the blood is, how much of it, and how many sprays.",
      fields = {
        blood_pos_field,
        blood_angle_field,
        blood_strength_field,
        {
          name = "count",
          type = "integer",
          optional = true,
          default = 5,
          description = "How many sprays, from 1 to 255.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.blood_bath({ pos = trx.lara.item.pos, count = 10 })]],
  },
  impl = function(opts)
    raw.blood_bath(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.strength or 5,
      opts.angle or -1,
      opts.count or 5
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
