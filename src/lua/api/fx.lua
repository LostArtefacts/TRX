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

api.define("fx.explosion", {
  description = [[
Shows the explosion a rocket or a grenade leaves behind, without the damage.

TR1, TR2 and TR4 draw the explosion sprite the level carries. TR3 has none, and
gets a fireball of sparks instead. Under water the effect uses the drowned
version, which throws a burst of bubbles and lifts a splash where the water
ends.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the explosion is and whether it is heard.",
      fields = {
        pos_field,
        {
          name = "sound",
          type = "boolean",
          optional = true,
          default = true,
          description = "Whether the explosion sound plays with it.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.explosion({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    raw.explosion(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.sound == nil or opts.sound
    )
  end,
})

api.define("fx.fire", {
  description = [[
Burns a fire at a point for this frame. Make the call every frame to keep the
fire alight.

TR4 only. The other games have no such fire, so the call does nothing there.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the fire is and how it burns.",
      fields = {
        pos_field,
        {
          name = "size",
          type = "integer",
          optional = true,
          default = 1,
          description = "How big it burns: `0` small, `1` medium, `2` big.",
        },
        {
          name = "fade",
          type = "integer",
          optional = true,
          default = 0,
          description = "How far it is dimmed, `0` being full strength.",
        },
      },
    },
  },
  examples = {
    [[trx.events.after_control(function()
  trx.fx.fire({ pos = trx.lara.item.pos, size = 2 })
end)]],
  },
  impl = function(opts)
    raw.fire(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.size or 1,
      opts.fade or 0
    )
  end,
})

api.define("fx.splash", {
  description = [[
Breaks the water at an item, the way a body falling in does. The item says
where the splash is; the water it lands in says how big.]],
  params = {
    {
      name = "item",
      type = "items.Item",
      description = "The item the splash rises around.",
    },
  },
  examples = { [[trx.fx.splash(trx.lara.item)]] },
  impl = function(item)
    raw.splash(item.num)
  end,
})

api.define("fx.wade_splash", {
  description = "Breaks the water around an item wading through it.",
  params = {
    {
      name = "item",
      type = "items.Item",
      description = "The item doing the wading.",
    },
    {
      name = "depth",
      type = "math.Distance",
      description = "How deep the water stands about it.",
    },
  },
  examples = { [[trx.fx.wade_splash(trx.lara.item, 512)]] },
  impl = function(item, depth)
    raw.wade_splash(item.num, depth)
  end,
})

api.define("fx.ripple", {
  description = [[
Spreads a ring on the water surface. The ring widens and fades on its own.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the ring is and how it spreads.",
      fields = {
        pos_field,
        {
          name = "size",
          type = "integer",
          optional = true,
          default = 8,
          description = "How wide it starts, from 1 to 255.",
        },
        {
          name = "slow",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it spreads at half speed.",
        },
        {
          name = "dark",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it is drawn dark rather than bright.",
        },
        {
          name = "blood",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it is drawn in the blood color.",
        },
        {
          name = "jitter",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether the ring wavers as it spreads.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.ripple({ pos = trx.lara.item.pos, size = 16 })]],
  },
  impl = function(opts)
    raw.ripple(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.size or 8,
      opts.slow,
      opts.dark,
      opts.blood,
      opts.jitter
    )
  end,
})

api.define("fx.small_splash", {
  description = "Throws a handful of drops off the water surface.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the drops are and how many of them.",
      fields = {
        pos_field,
        {
          name = "count",
          type = "integer",
          optional = true,
          default = 1,
          description = "How many drops, from 1 to 255.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.small_splash({ pos = trx.lara.item.pos, count = 8 })]],
  },
  impl = function(opts)
    raw.small_splash(opts.pos.x, opts.pos.y, opts.pos.z, opts.count or 1)
  end,
})

api.define("fx.underwater_blood", {
  description = [[
Spreads a cloud of blood under water, the way a hit that lands there does.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the cloud is and how wide it spreads.",
      fields = {
        pos_field,
        {
          name = "size",
          type = "integer",
          optional = true,
          default = 8,
          description = "How wide it spreads, from 1 to 255.",
        },
        {
          name = "dark",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it is drawn in the darker TR3 gold color.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.underwater_blood({ pos = trx.lara.item.pos, size = 32 })]],
  },
  impl = function(opts)
    raw.underwater_blood(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.size or 8,
      opts.dark
    )
  end,
})

api.define("fx.footprint", {
  description = [[
Leaves a footprint under an item, on the floor the item stands on. The floor
material decides whether one is left at all.]],
  params = {
    {
      name = "item",
      type = "items.Item",
      description = "The item the print is taken from.",
    },
    {
      name = "left_foot",
      type = "boolean",
      description = "Whether it is the left foot rather than the right.",
    },
  },
  examples = { [[trx.fx.footprint(trx.lara.item, true)]] },
  impl = function(item, left_foot)
    raw.footprint(item.num, left_foot)
  end,
})

api.define("fx.knockback", {
  description = [[
Spreads a ring of force out from a point, as a blast does. The ring is drawn
and widens on its own.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the ring starts.",
      fields = { pos_field },
    },
  },
  examples = { [[trx.fx.knockback({ pos = trx.lara.item.pos })]] },
  impl = function(opts)
    raw.knockback(opts.pos.x, opts.pos.y, opts.pos.z)
  end,
})

local SparkType = api.enum("fx.SparkType", {
  backing = "SPARK_SPRITE_TYPE",
  description = "Which spark-set sprite a spark is drawn with.",
  values = {
    EXPLOSION = "The soft round puff fire, smoke and explosions are drawn with.",
    SMALL_SPLASH = "A single drop of water.",
    BIG_SPLASH = "A sheet of water.",
    RIPPLE = "A ring on the water surface.",
    PARTICLE = "The plain speck, which is also what a footprint is drawn with.",
    SHIELD = "The bubble drawn around a shielded target.",
    ROPE = "A length of rope.",
    DRIVE = "The forward gear light of a vehicle.",
    REVERSE = "The reverse gear light of a vehicle.",
    RICOCHET = "The spark struck off a wall by a shot.",
    BLOOD = "A drop of blood.",
  },
})

local DrawType = api.enum("fx.DrawType", {
  backing = "DRAW_TYPE",
  description = "How a sprite is laid over what is behind it.",
  values = {
    OPAQUE = "It covers what is behind it.",
    BLEND = "It is mixed with what is behind it.",
    BLEND_ADD = "It is added to what is behind it, so it lightens.",
    BLEND_SUB = "It is taken from what is behind it, so it darkens.",
    REFLECTIVE_OPAQUE = "Opaque, and carrying the room reflection.",
    REFLECTIVE_BLEND_ADD = "Added, and carrying the room reflection.",
  },
})

api.type("fx.Spark", {
  backing = "SPARK",
  description = [[
A spark is one particle from the spark pool: a sprite that lives for a set
number of frames, moving, resizing and fading on its own as it does.

TR3 and TR4 only. The earlier games carry no spark set, so nothing spawns one
and the pool stays empty.

A spark that runs out of life leaves its slot to the next one asked for. A
handle held across that becomes stale, and field access raises an error.]],
  fields = {
    life = {
      from = "life",
      type = "integer",
      description = "Frames of life left. It reaches zero and the spark ends.",
    },
    life_span = {
      from = "s_life",
      type = "integer",
      description = "Frames of life it started with, which the fades are "
        .. "measured against.",
    },
    pos = {
      from = "pos",
      type = "math.Vec3",
      description = "Where it sits, in the world, or from what it is attached "
        .. "to. Read `trx.fx.Spark.world_pos` for the position it is drawn at.",
    },
    vel = {
      from = "vel",
      type = "math.Vec3",
      description = "How far it moves each frame.",
    },
    width = {
      from = "size.width",
      type = "integer",
      description = "How wide it is drawn now.",
    },
    height = {
      from = "size.height",
      type = "integer",
      description = "How tall it is drawn now.",
    },
    start_width = {
      from = "src_size.width",
      type = "integer",
      description = "The width it grows from.",
    },
    start_height = {
      from = "src_size.height",
      type = "integer",
      description = "The height it grows from.",
    },
    end_width = {
      from = "dst_size.width",
      type = "integer",
      description = "The width it grows to.",
    },
    end_height = {
      from = "dst_size.height",
      type = "integer",
      description = "The height it grows to.",
    },
    color = {
      from = "color",
      type = "math.Color",
      description = "The color it is drawn in now.",
    },
    start_color = {
      from = "src_color",
      type = "math.Color",
      description = "The color it fades from.",
    },
    end_color = {
      from = "dst_color",
      type = "math.Color",
      description = "The color it fades to.",
    },
    fade_speed = {
      from = "col_fade_speed",
      type = "integer",
      description = "How fast it travels from one color to the other.",
    },
    fade_to_black = {
      from = "fade_to_black",
      type = "integer",
      description = "How many frames of life are left when it starts to "
        .. "darken toward black.",
    },
    scalar = {
      from = "scalar",
      type = "integer",
      description = "How strongly the size is scaled with distance.",
    },
    gravity = {
      from = "gravity",
      type = "integer",
      description = "How much it is pulled down each frame.",
    },
    max_y_vel = {
      from = "max_y_vel",
      type = "integer",
      description = "As fast as gravity may carry it down.",
    },
    friction = {
      from = "friction",
      type = "integer",
      description = "How fast it is slowed each frame.",
    },
    rot_angle = {
      from = "rot_angle",
      type = "integer",
      description = "How far it is turned about the view, from 0 to 4095.",
    },
    rot_add = {
      from = "rot_add",
      type = "integer",
      description = "How far it turns each frame.",
    },
    extras = {
      from = "extras",
      type = "integer",
      description = "How many further sparks it leaves behind as it ends.",
    },
    node_num = {
      from = "node_num",
      type = "integer",
      description = "Which of the sixteen body points it hangs off, where it "
        .. "is attached to one.",
    },
    room_num = {
      from = "room_num",
      type = "rooms.Num",
      writable = false,
      description = "The room it was spawned in.",
    },
    draw_type = {
      from = "draw_type",
      type = "fx.DrawType",
      writable = false,
      description = "How it is laid over what is behind it.",
    },
    scales = {
      from = "scales",
      type = "boolean",
      description = "Whether it travels from its start size to its end size.",
    },
    rotates = {
      from = "rotates",
      type = "boolean",
      description = "Whether it turns as it lives.",
    },
    is_blood = {
      from = "is_blood",
      type = "boolean",
      description = "Whether it counts as blood, which the pool sheds first "
        .. "when it runs short of slots.",
    },
    is_outside = {
      from = "is_outside",
      type = "boolean",
      description = "Whether the wind carries it.",
    },
    is_underwater = {
      from = "is_underwater",
      type = "boolean",
      description = "Whether it drifts like an underwater particle.",
    },
    is_green = {
      from = "is_green",
      type = "boolean",
      description = "Whether it is drawn in the green of the poison gas.",
    },
    uses_alt_sprite = {
      from = "uses_alt_sprite",
      type = "boolean",
      description = "Whether it is drawn with the second sprite of its kind.",
    },
  },

  extensions = {
    room = {
      type = "rooms.Room",
      description = "The room it was spawned in.",
      impl = function(spark)
        return trx.rooms[spark.room_num]
      end,
    },
    world_pos = {
      type = "math.Vec3",
      description = "Where it is drawn, which is where it sits unless it is "
        .. "attached to something that carries it.",
      impl = function(spark)
        return raw.get_spark_world_pos(spark)
      end,
    },
    item = {
      type = "items.Item",
      nullable = true,
      description = "The item it is attached to, and `nil` where it hangs on "
        .. "nothing.",
      impl = function(spark)
        local num = raw.get_spark_item(spark)
        return num and trx.items[num] or nil
      end,
    },
  },

  methods = {
    is_valid = {
      returns = {
        type = "boolean",
        description = "False once the spark has ended.",
      },
      description = [[Reports whether the handle still names a live spark.

A spark that runs out of life leaves its slot to the next one asked for.]],
    },
    kill = {
      description = "Ends the spark now and frees its slot.",
    },
  },
})

api.namespace("fx.sparks", {
  description = [[
The spark pool: the particles TR3 and TR4 draw their smoke, flames, sparks and
splashes with.

TR1 and TR2 carry no spark set. There the pool stays empty, and everything here
returns nothing rather than raising.]],
})

api.const("fx.sparks.MAX_COUNT", {
  value = raw.spark_max_count(),
  type = "integer",
  description = "How many sparks the pool holds. A spark spawned after that "
    .. "takes the slot of the one with the least life left.",
})

api.container("fx.sparks.pool", {
  description = [[The spark pool, counted from 1. `#trx.fx.sparks.pool` is
`trx.fx.sparks.MAX_COUNT` rather than how many sparks are alive: a slot holding
no live spark reads as `nil`.]],
  key = { type = "integer" },
  value = { type = "fx.Spark", nullable = true },
  examples = {
    [[for _, spark in pairs(trx.fx.sparks.pool) do
  spark.color = trx.math.color(255, 0, 0)
end]],
  },
  get = function(idx)
    return raw.get_spark(idx - 1)
  end,
  count = raw.spark_max_count,
})

api.type("fx.Wind", {
  record = true,
  description = "How far the wind carries a spark each frame.",
  fields = {
    x = { type = "math.Distance", description = "The east-west axis." },
    z = { type = "math.Distance", description = "The north-south axis." },
  },
})

api.property("fx.sparks.wind", {
  type = "fx.Wind",
  description = [[
The wind that carries the sparks marked `trx.fx.Spark.is_outside`.

The engine works this out again every frame from the breeze setting, so a value
written here holds for that frame alone.]],
  get = function()
    local x, z = raw.get_smoke_wind()
    return { x = x, z = z }
  end,
  set = function(wind)
    raw.set_smoke_wind(wind.x, wind.z)
  end,
})

local spark_pos_field = {
  name = "pos",
  type = "math.Vec3",
  description = "World position. Must lie inside the level.",
}

local spark_vel_field = {
  name = "vel",
  type = "math.Vec3",
  optional = true,
  description = "How far it moves each frame. Defaults to standing still.",
}

api.define("fx.sparks.spawn", {
  description = [[
Puts one spark in the world and hands it back, so a script can draw with the
pool the game draws its own smoke and flames with.

The spark lives for the frames it is given, moving, resizing and fading on its
own, and then frees its slot. Nothing has to be called each frame to keep it up.

Returns `nil` where the level carries no spark set, which is every TR1 and TR2
level.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "What the spark is and how it behaves.",
      fields = {
        spark_pos_field,
        spark_vel_field,
        {
          name = "sprite_type",
          type = "fx.SparkType",
          optional = true,
          default = SparkType.PARTICLE,
          description = "Which sprite it is drawn with.",
        },
        {
          name = "life",
          type = "integer",
          optional = true,
          default = 16,
          description = "How many frames it lives, from 1 to 255.",
        },
        {
          name = "color",
          type = "math.Color",
          optional = true,
          description = "The color it starts in. Defaults to white.",
        },
        {
          name = "end_color",
          type = "math.Color",
          optional = true,
          description = "The color it fades to. Defaults to the color it "
            .. "starts in, so it holds one color.",
        },
        {
          name = "fade_speed",
          type = "integer",
          optional = true,
          default = 8,
          description = "How fast it travels between the two colors.",
        },
        {
          name = "fade_to_black",
          type = "integer",
          optional = true,
          default = 8,
          description = "How many frames of life are left when it starts to "
            .. "darken toward black.",
        },
        {
          name = "width",
          type = "integer",
          optional = true,
          default = 4,
          description = "How wide it starts, from 0 to 255.",
        },
        {
          name = "height",
          type = "integer",
          optional = true,
          default = 4,
          description = "How tall it starts, from 0 to 255.",
        },
        {
          name = "end_width",
          type = "integer",
          optional = true,
          description = "The width it grows to, for a spark that scales. "
            .. "Defaults to the width it starts at.",
        },
        {
          name = "end_height",
          type = "integer",
          optional = true,
          description = "The height it grows to, for a spark that scales. "
            .. "Defaults to the height it starts at.",
        },
        {
          name = "scales",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it travels from its start size to its end "
            .. "size over its life.",
        },
        {
          name = "scalar",
          type = "integer",
          optional = true,
          default = 2,
          description = "How strongly the size is scaled with distance.",
        },
        {
          name = "gravity",
          type = "integer",
          optional = true,
          default = 0,
          description = "How much it is pulled down each frame.",
        },
        {
          name = "max_y_vel",
          type = "integer",
          optional = true,
          default = 0,
          description = "As fast as gravity may carry it down.",
        },
        {
          name = "friction",
          type = "integer",
          optional = true,
          default = 0,
          description = "How fast it is slowed each frame.",
        },
        {
          name = "rot_angle",
          type = "integer",
          optional = true,
          default = 0,
          description = "How far it starts turned about the view, from 0 to "
            .. "4095.",
        },
        {
          name = "rot_add",
          type = "integer",
          optional = true,
          default = 0,
          description = "How far it turns each frame, for a spark that turns.",
        },
        {
          name = "rotates",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it turns as it lives.",
        },
        {
          name = "extras",
          type = "integer",
          optional = true,
          default = 0,
          description = "How many further sparks it leaves behind as it ends.",
        },
        {
          name = "draw_type",
          type = "fx.DrawType",
          optional = true,
          default = DrawType.BLEND_ADD,
          description = "How it is laid over what is behind it.",
        },
        {
          name = "is_outside",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether the wind carries it.",
        },
        {
          name = "is_underwater",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it drifts like an underwater particle.",
        },
        {
          name = "is_green",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it is drawn in the green of the poison gas.",
        },
        {
          name = "uses_alt_sprite",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it is drawn with the second sprite of its "
            .. "kind.",
        },
      },
    },
  },
  returns = {
    type = "fx.Spark",
    nullable = true,
    description = "The spark, or `nil` where the level carries no spark set.",
  },
  examples = {
    [[trx.fx.sparks.spawn({
  pos = trx.lara.item.pos,
  vel = { x = 0, y = -8, z = 0 },
  sprite_type = trx.fx.SparkType.EXPLOSION,
  life = 48,
  color = trx.math.color(255, 200, 64),
  end_color = trx.math.color(64, 0, 0),
  width = 16,
  end_width = 48,
  scales = true,
})]],
  },
  impl = raw.spawn_spark,
})

api.define("fx.sparks.explosion", {
  description = "Throws the fireball of sparks an explosion is drawn with.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the fireball is and how strongly it bursts.",
      fields = {
        spark_pos_field,
        {
          name = "extras",
          type = "integer",
          optional = true,
          default = 3,
          description = "How many further sparks each one leaves behind as it "
            .. "ends, from 0 to 3.",
        },
        {
          name = "light",
          type = "integer",
          optional = true,
          default = -2,
          description = "How the fireball lights the room around it: `-2` "
            .. "bright, `-1` dim, `0` not at all.",
        },
        {
          name = "underwater",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it uses the underwater burst.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.explosion({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    raw.spark_explosion(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.extras or 3,
      opts.light or -2,
      opts.underwater
    )
  end,
})

api.define("fx.sparks.explosion_smoke", {
  description = "Lifts the smoke an explosion leaves behind.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the smoke is and which part of the burst it is.",
      fields = {
        spark_pos_field,
        {
          name = "underwater",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it lifts as it does under water.",
        },
        {
          name = "ending",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it is the thinner smoke that closes the "
            .. "burst rather than the smoke that opens it.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.explosion_smoke({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    raw.spark_explosion_smoke(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.underwater,
      opts.ending
    )
  end,
})

api.define("fx.sparks.explosion_bubble", {
  description = "Throws the burst of bubbles an explosion under water makes.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the bubbles rise.",
      fields = { spark_pos_field },
    },
  },
  examples = {
    [[trx.fx.sparks.explosion_bubble({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    raw.spark_explosion_bubble(opts.pos.x, opts.pos.y, opts.pos.z)
  end,
})

local flame_variant_field = {
  name = "variant",
  type = "integer",
  optional = true,
  default = 0,
  description = "Which colors it burns in: `0` orange, `2` pale, `254` green.",
}

api.define("fx.sparks.fire_flame", {
  description = [[
Throws one tongue of flame, the way a burning body does. Make the call every
frame to keep a fire burning.

No flame is thrown more than twenty sectors from Lara.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the flame is and what color it burns.",
      fields = { spark_pos_field, flame_variant_field },
    },
  },
  examples = {
    [[trx.fx.sparks.fire_flame({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    raw.spark_fire_flame(opts.pos.x, opts.pos.y, opts.pos.z, opts.variant or 0)
  end,
})

api.define("fx.sparks.fire_smoke", {
  description = "Lifts one puff of the smoke a fire gives off.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the smoke is and which flame color it follows.",
      fields = { spark_pos_field, flame_variant_field },
    },
  },
  examples = {
    [[trx.fx.sparks.fire_smoke({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    raw.spark_fire_smoke(opts.pos.x, opts.pos.y, opts.pos.z, opts.variant or 0)
  end,
})

api.define("fx.sparks.static_flame", {
  description = "Throws one tongue of the flame a standing fire burns with.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the flame is and how big it burns.",
      fields = {
        spark_pos_field,
        {
          name = "size",
          type = "integer",
          optional = true,
          default = 32,
          description = "How big it burns.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.static_flame({ pos = trx.lara.item.pos, size = 64 })]],
  },
  impl = function(opts)
    raw.spark_static_flame(opts.pos.x, opts.pos.y, opts.pos.z, opts.size or 32)
  end,
})

api.define("fx.sparks.side_flame", {
  description = "Throws a tongue of flame sideways, as a jet does.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the jet is and which way it burns.",
      fields = {
        spark_pos_field,
        {
          name = "angle",
          type = "math.Angle",
          description = "The way the flame is thrown.",
        },
        {
          name = "speed",
          type = "integer",
          optional = true,
          default = 32,
          description = "How hard it is thrown.",
        },
        {
          name = "pilot",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it is the small pilot flame rather than the "
            .. "jet itself.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.side_flame({
  pos = trx.lara.item.pos,
  angle = trx.lara.item.rot.y,
})]],
  },
  impl = function(opts)
    raw.spark_side_flame(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.angle,
      opts.speed or 32,
      opts.pilot
    )
  end,
})

api.define("fx.sparks.flamethrower_flame", {
  description = "Throws the flame a flamethrower leaves where its jet lands.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the flame lands.",
      fields = { spark_pos_field },
    },
  },
  examples = {
    [[trx.fx.sparks.flamethrower_flame({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    raw.spark_flamethrower_flame(opts.pos.x, opts.pos.y, opts.pos.z)
  end,
})

api.define("fx.sparks.flamethrower_smoke", {
  description = "Lifts the smoke a flamethrower jet leaves behind.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the smoke is.",
      fields = {
        spark_pos_field,
        {
          name = "underwater",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it lifts as it does under water.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.flamethrower_smoke({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    raw.spark_flamethrower_smoke(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.underwater
    )
  end,
})

api.define("fx.sparks.gun_smoke", {
  description = "Lifts the smoke a fired weapon leaves at its muzzle.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the smoke is and which weapon left it.",
      fields = {
        spark_pos_field,
        {
          name = "weapon",
          type = "catalog.weapons",
          description = "Which weapon it is drawn for. `UNKNOWN`, `UNARMED`, "
            .. "and out-of-range values raise.",
        },
        {
          name = "shade",
          type = "integer",
          optional = true,
          default = 64,
          description = "How light the smoke is, from 0 to 255.",
        },
        {
          name = "initial",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether it is the first puff of a shot, which is "
            .. "denser than the ones that follow.",
        },
        {
          name = "vel",
          type = "math.Vec3",
          optional = true,
          description = "Which way the smoke is pushed. Left out, it lifts "
            .. "straight up.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.gun_smoke({
  pos = trx.lara.item.pos,
  weapon = trx.catalog.weapons.pistols,
  initial = true,
})]],
  },
  impl = function(opts)
    local vel = opts.vel
    raw.spark_gun_smoke(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.weapon,
      opts.shade or 64,
      opts.initial,
      vel and vel.x,
      vel and vel.y,
      vel and vel.z
    )
  end,
})

api.define("fx.sparks.dart_smoke", {
  description = "Lifts the trail of smoke a flying dart leaves.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the smoke is and which way the dart flies.",
      fields = {
        spark_pos_field,
        spark_vel_field,
        {
          name = "hit",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether the dart has landed, which puffs the smoke "
            .. "out rather than trailing it.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.dart_smoke({ pos = trx.lara.item.pos, hit = true })]],
  },
  impl = function(opts)
    local vel = opts.vel or { x = 0, z = 0 }
    raw.spark_dart_smoke(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      vel.x,
      vel.z,
      opts.hit
    )
  end,
})

api.define("fx.sparks.rocket_smoke", {
  description = "Lifts the trail of smoke a flying rocket leaves.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the smoke is and how light it lifts.",
      fields = {
        spark_pos_field,
        {
          name = "shade",
          type = "integer",
          optional = true,
          default = 0,
          description = "How light the smoke turns as it fades, from 0 to 191.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.rocket_smoke({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    raw.spark_rocket_smoke(opts.pos.x, opts.pos.y, opts.pos.z, opts.shade or 0)
  end,
})

api.define("fx.sparks.flare", {
  description = "Throws the sparks a burning flare gives off.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the sparks are and which way they fly.",
      fields = {
        spark_pos_field,
        spark_vel_field,
        {
          name = "smoke",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether smoke is lifted with them.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.flare({ pos = trx.lara.item.pos, smoke = true })]],
  },
  impl = function(opts)
    local vel = opts.vel or { x = 0, y = 0, z = 0 }
    raw.spark_flare(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      vel.x,
      vel.y,
      vel.z,
      opts.smoke
    )
  end,
})

api.define("fx.sparks.shotgun", {
  description = "Throws the sparks a shotgun blast strikes off what it hits.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the sparks are and which way they fly.",
      fields = { spark_pos_field, spark_vel_field },
    },
  },
  examples = {
    [[trx.fx.sparks.shotgun({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    local vel = opts.vel or { x = 0, y = 0, z = 0 }
    raw.spark_shotgun(opts.pos.x, opts.pos.y, opts.pos.z, vel.x, vel.y, vel.z)
  end,
})

api.define("fx.sparks.ricochet", {
  description = [[
Strikes the sparks a shot makes where it lands on a wall.

TR4 throws as many streaks as asked for and can lift smoke instead. TR3 throws
one burst, and uses the count as its size.]],
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the shot lands and which way the sparks fly.",
      fields = {
        spark_pos_field,
        {
          name = "angle",
          type = "math.Angle",
          description = "The way the sparks fly.",
        },
        {
          name = "count",
          type = "integer",
          optional = true,
          default = 3,
          description = "How many streaks, from 1 to 255.",
        },
        {
          name = "smoke_only",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether smoke is lifted rather than sparks struck. "
            .. "TR4 only.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.ricochet({ pos = trx.lara.item.pos, angle = 0 })]],
  },
  impl = function(opts)
    raw.spark_ricochet(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.angle,
      opts.count or 3,
      opts.smoke_only
    )
  end,
})

api.define("fx.sparks.bubble", {
  description = "Lifts one bubble through the water.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the bubble is and how big it is.",
      fields = {
        spark_pos_field,
        {
          name = "size",
          type = "integer",
          optional = true,
          default = 8,
          description = "How big it is at its smallest.",
        },
        {
          name = "size_range",
          type = "integer",
          optional = true,
          default = 8,
          description = "How much bigger than that it may be drawn.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.bubble({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    raw.spark_bubble(
      opts.pos.x,
      opts.pos.y,
      opts.pos.z,
      opts.size or 8,
      opts.size_range or 8
    )
  end,
})

api.define("fx.sparks.breath", {
  description = "Puffs the cloud of breath a body gives off in the cold.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the breath is and which way it drifts.",
      fields = { spark_pos_field, spark_vel_field },
    },
  },
  examples = {
    [[trx.fx.sparks.breath({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    local vel = opts.vel or { x = 0, y = 0, z = 0 }
    raw.spark_breath(opts.pos.x, opts.pos.y, opts.pos.z, vel.x, vel.y, vel.z)
  end,
})

api.define("fx.sparks.pickup_aid", {
  description = "Throws the twinkle that marks a pickup worth reaching.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the twinkle is and which way it drifts.",
      fields = { spark_pos_field, spark_vel_field },
    },
  },
  examples = {
    [[trx.fx.sparks.pickup_aid({ pos = trx.lara.item.pos })]],
  },
  impl = function(opts)
    local vel = opts.vel or { x = 0, z = 0 }
    raw.spark_pickup_aid(opts.pos.x, opts.pos.y, opts.pos.z, vel.x, vel.z)
  end,
})

api.define("fx.sparks.waterfall_mist", {
  description = "Lifts the mist that stands at the foot of a waterfall.",
  params = {
    {
      name = "opts",
      type = "table",
      description = "Where the mist is and which way it faces.",
      fields = {
        spark_pos_field,
        {
          name = "angle",
          type = "math.Angle",
          description = "The way the waterfall faces.",
        },
      },
    },
  },
  examples = {
    [[trx.fx.sparks.waterfall_mist({ pos = trx.lara.item.pos, angle = 0 })]],
  },
  impl = function(opts)
    raw.spark_waterfall_mist(opts.pos.x, opts.pos.y, opts.pos.z, opts.angle)
  end,
})
