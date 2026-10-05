local raw = trxc.fx
local h = require("trx.internal.helpers")

require("trx.math")
require("trx.rooms")

---@class trx
---@field fx trx.fx

---What a script puts in front of the player: things seen rather than things
---the game holds.
---@trx.module 18
---@class (exact) trx.fx
---@field fog_color trx.math.Color? The color override for distance fog.
---
---  `nil` means no override. Write `nil` to restore the level fog color. A
---  level change clears the override. Savegames keep it. This controls
---  distance fog only. Fog bulbs have their own colors in `trx.fx.fog_bulbs`.
local M = h.module("fx")

local WHITE = { r = 255, g = 255, b = 255 }

---@class (exact) trx.fx.emit_light.opts.color
---@field r integer Red.
---@field g integer Green.
---@field b integer Blue.

---@class (exact) trx.fx.emit_light.opts
---@field pos trx.math.Vec3 World position.
---@field radius? trx.math.Distance How far it reaches, at least an eighth of a sector. Rounded down to the eighth of a sector the engine measures a dynamic light in, and carried about a quarter further than asked for in TR4, which falls a light off more gently.
---@field color? trx.fx.emit_light.opts.color Its color, each channel 0 to 255. Defaults to white.
---@trx.default radius 3072

---Lights the world around a point for this frame. Make the call every frame to
---keep the light up, and at a new position each time to move it.
---
---A frame shows only `trx.fx.MAX_LIGHTS` lights. A light asked for past that
---takes the place of the one furthest from the camera, so the nearest are the
---ones seen. No error is raised.
---
---TR1 and TR2 light in brightness alone, so there the light is as bright as
---its brightest channel and comes out white.
---
---```lua
---trx.events.after_control(function()
---  trx.fx.emit_light({
---    pos = trx.lara.item.pos,
---    radius = 2048,
---    color = { r = 0, g = 255, b = 192 },
---  })
---end)
---```
---@param opts trx.fx.emit_light.opts Where the light is and what it looks like.
function M.emit_light(opts)
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
end

---@class (exact) trx.fx.emit_fog.opts.color
---@field r integer Red.
---@field g integer Green.
---@field b integer Blue.

---@class (exact) trx.fx.emit_fog.opts
---@field pos trx.math.Vec3 World position.
---@field radius? trx.math.Distance How far the fog reaches, at least one unit.
---@field density? integer How thick it is, from 0 for nothing to 255.
---@field color? trx.fx.emit_fog.opts.color Its color, each channel 0 to 255. Defaults to white.
---@trx.default radius 2048
---@trx.default density 128

---Fills a ball of air with fog for this frame, which the player sees through
---rather than on. Where a light brightens what it falls on, this hangs in the
---space itself. Make the call every frame to keep the fog up.
---
---A frame shows only `trx.fx.MAX_FOG` balls of fog. A ball asked for past that
---takes the place of the one furthest from the camera, so the nearest are the
---ones seen. No error is raised.
---
---```lua
---trx.events.after_control(function()
---  trx.fx.emit_fog({
---    pos = { x = 32768, y = -1024, z = 45056 },
---    radius = 3072,
---    density = 64,
---    color = { r = 128, g = 160, b = 192 },
---  })
---end)
---```
---@param opts trx.fx.emit_fog.opts Where the fog is and what it looks like.
function M.emit_fog(opts)
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
end

---A level fog bulb is a ball of fog drawn inside a room.
---
---TR4 stores fog bulbs as room lights. A script can change their color and
---density. The level sets their position and radius. A bulb follows the fog
---color until a script gives it a color of its own.
---@class (exact) trx.fx.FogBulb
---@trx.readonly pos, radius
---@field color trx.math.Color? The color a script gave the bulb.
---
---  `nil` means none was given, and the bulb is drawn in the fog color in
---  force. Write `nil` to hand a bulb back to that color.
---@field density integer Fog density, from `0` for none to `255`. A value outside this range raises an error.
---@field pos trx.math.Vec3 Center of the fog bulb.
---@field radius trx.math.Distance How far the fog reaches from that position.
---@field room trx.rooms.Room The room the bulb sits in.
local FogBulb = h.handle("fx.FogBulb", "FOG_BULB", {
  fields = {
    color = "color",
    density = "density",
    pos = "pos",
    radius = "radius",
  },
  writable = { "color", "density" },
  extensions = {
    room = function(bulb)
      local num = raw.get_fog_bulb_room(bulb)
      return num and trx.rooms[num] or nil
    end,
  },
})

---Reports whether the handle still names a bulb in the loaded level.
---
---A level change replaces all bulbs. A handle held across one becomes stale,
---and field access raises an error.
---@return boolean # False after the level that held the bulb is left.
function FogBulb:is_valid() end

---The level fog bulbs, counted from 1. `#trx.fx.fog_bulbs` is the count.
---`pairs()` walks them in order. A level shows at most twenty. The player can
---turn them off.
---
---```lua
---for _, bulb in pairs(trx.fx.fog_bulbs) do
---  bulb.color = trx.math.color(245, 200, 60)
---end
---```
---@type table<integer, trx.fx.FogBulb?>
M.fog_bulbs = h.container("fx.fog_bulbs", {
  base = 1,
  get = function(idx)
    return raw.get_fog_bulb(idx - 1)
  end,
  count = raw.fog_bulb_count,
})

h.properties(M, "fx", {
  fog_color = {
    get = raw.get_fog_color,
    set = raw.set_fog_color,
  },
})

---@class (exact) trx.fx.blood.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field angle? trx.math.Angle The way the drops fly. Left out, TR4 throws them every way, which is what its own hits do where nothing aims them; the other games read it as straight ahead.
---@field strength? integer How heavy the hit reads, from 1 to 255. TR3 and TR4 count it in drops, TR4 measures the width of a cloud under water with it, and TR1 and TR2 have one drifting sprite whose speed it sets.
---@trx.default strength 5

---Throws a spray of blood into the world at a point, the way a blow that lands
---does. The drops then fall on their own.
---
---TR3 and TR4 throw drops that fall and darken as they go, and in TR4 a hit
---under water spreads as a cloud instead. TR1 and TR2 have one blood sprite
---that drifts up.
---
---Unlike the rest of the module, this has a bearing on what the game decides.
---The engine places the drops from the control random stream, and in TR1 and
---TR2 the spray takes a slot from the effect pool a save holds.
---
---```lua
---trx.events.on_hit(function(item, damage)
---  trx.fx.blood({ pos = item.pos, angle = item.rot.y, strength = damage })
---end)
---```
---@param opts trx.fx.blood.opts Where the blood is and how much of it.
function M.blood(opts)
  raw.blood(
    opts.pos.x,
    opts.pos.y,
    opts.pos.z,
    opts.strength or 5,
    opts.angle or -1
  )
end

---@class (exact) trx.fx.blood_bath.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field angle? trx.math.Angle The way the drops fly. Left out, TR4 throws them every way, which is what its own hits do where nothing aims them; the other games read it as straight ahead.
---@field strength? integer How heavy the hit reads, from 1 to 255. TR3 and TR4 count it in drops, TR4 measures the width of a cloud under water with it, and TR1 and TR2 have one drifting sprite whose speed it sets.
---@field count? integer How many sprays, from 1 to 255.
---@trx.default strength 5
---@trx.default count 5

---Throws several sprays of blood about a point, the way a trap that kills
---does. Each one lands anywhere in the half-sector box around the point, so
---the blood covers a body rather than a spot.
---
---Each spray costs what `trx.fx.blood` costs, in random draws and in effect
---slots.
---
---```lua
---trx.fx.blood_bath({ pos = trx.lara.item.pos, count = 10 })
---```
---@param opts trx.fx.blood_bath.opts Where the blood is, how much of it, and how many sprays.
function M.blood_bath(opts)
  raw.blood_bath(
    opts.pos.x,
    opts.pos.y,
    opts.pos.z,
    opts.strength or 5,
    opts.angle or -1,
    opts.count or 5
  )
end

---How many lights a script can put up in one frame.
---@type integer
---@trx.value 64
M.MAX_LIGHTS = h.const("fx.MAX_LIGHTS", raw.MAX_LIGHTS)

---How many balls of fog can be seen at once. TR4 levels carry fog of their
---own, which takes its slots first, so fewer than this reach the screen where
---a level is already using them.
---@type integer
---@trx.value 10
M.MAX_FOG = h.const("fx.MAX_FOG", raw.MAX_FOG)

---@class (exact) trx.fx.explosion.opts
---@field pos trx.math.Vec3 World position.
---@field sound? boolean Whether the explosion sound plays with it.
---@trx.default sound true

---Shows the explosion a rocket or a grenade leaves behind, without the damage.
---
---TR1, TR2 and TR4 draw the explosion sprite the level carries. TR3 has none,
---and gets a fireball of sparks instead. Under water the effect uses the
---drowned version, which throws a burst of bubbles and lifts a splash where
---the water ends.
---
---```lua
---trx.fx.explosion({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.explosion.opts Where the explosion is and whether it is heard.
function M.explosion(opts)
  raw.explosion(
    opts.pos.x,
    opts.pos.y,
    opts.pos.z,
    opts.sound == nil or opts.sound
  )
end

---@class (exact) trx.fx.fire.opts
---@field pos trx.math.Vec3 World position.
---@field size? integer How big it burns: `0` small, `1` medium, `2` big.
---@field fade? integer How far it is dimmed, `0` being full strength.
---@trx.default size 1
---@trx.default fade 0

---Burns a fire at a point for this frame. Make the call every frame to keep
---the fire alight.
---
---TR4 only. The other games have no such fire, so the call does nothing there.
---
---```lua
---trx.events.after_control(function()
---  trx.fx.fire({ pos = trx.lara.item.pos, size = 2 })
---end)
---```
---@param opts trx.fx.fire.opts Where the fire is and how it burns.
function M.fire(opts)
  raw.fire(opts.pos.x, opts.pos.y, opts.pos.z, opts.size or 1, opts.fade or 0)
end

---Breaks the water at an item, the way a body falling in does. The item says
---where the splash is; the water it lands in says how big.
---
---```lua
---trx.fx.splash(trx.lara.item)
---```
---@param item trx.items.Item The item the splash rises around.
function M.splash(item)
  raw.splash(item.num)
end

---Breaks the water around an item wading through it.
---
---```lua
---trx.fx.wade_splash(trx.lara.item, 512)
---```
---@param item trx.items.Item The item doing the wading.
---@param depth trx.math.Distance How deep the water stands about it.
function M.wade_splash(item, depth)
  raw.wade_splash(item.num, depth)
end

---@class (exact) trx.fx.ripple.opts
---@field pos trx.math.Vec3 World position.
---@field size? integer How wide it starts, from 1 to 255.
---@field slow? boolean Whether it spreads at half speed.
---@field dark? boolean Whether it is drawn dark rather than bright.
---@field blood? boolean Whether it is drawn in the blood color.
---@field jitter? boolean Whether the ring wavers as it spreads.
---@trx.default size 8
---@trx.default slow false
---@trx.default dark false
---@trx.default blood false
---@trx.default jitter false

---Spreads a ring on the water surface. The ring widens and fades on its own.
---
---```lua
---trx.fx.ripple({ pos = trx.lara.item.pos, size = 16 })
---```
---@param opts trx.fx.ripple.opts Where the ring is and how it spreads.
function M.ripple(opts)
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
end

---@class (exact) trx.fx.small_splash.opts
---@field pos trx.math.Vec3 World position.
---@field count? integer How many drops, from 1 to 255.
---@trx.default count 1

---Throws a handful of drops off the water surface.
---
---```lua
---trx.fx.small_splash({ pos = trx.lara.item.pos, count = 8 })
---```
---@param opts trx.fx.small_splash.opts Where the drops are and how many of them.
function M.small_splash(opts)
  raw.small_splash(opts.pos.x, opts.pos.y, opts.pos.z, opts.count or 1)
end

---@class (exact) trx.fx.underwater_blood.opts
---@field pos trx.math.Vec3 World position.
---@field size? integer How wide it spreads, from 1 to 255.
---@field dark? boolean Whether it is drawn in the darker TR3 gold color.
---@trx.default size 8
---@trx.default dark false

---Spreads a cloud of blood under water, the way a hit that lands there does.
---
---```lua
---trx.fx.underwater_blood({ pos = trx.lara.item.pos, size = 32 })
---```
---@param opts trx.fx.underwater_blood.opts Where the cloud is and how wide it spreads.
function M.underwater_blood(opts)
  raw.underwater_blood(
    opts.pos.x,
    opts.pos.y,
    opts.pos.z,
    opts.size or 8,
    opts.dark
  )
end

---Leaves a footprint under an item, on the floor the item stands on. The floor
---material decides whether one is left at all.
---
---```lua
---trx.fx.footprint(trx.lara.item, true)
---```
---@param item trx.items.Item The item the print is taken from.
---@param left_foot boolean Whether it is the left foot rather than the right.
function M.footprint(item, left_foot)
  raw.footprint(item.num, left_foot)
end

---@class (exact) trx.fx.gun_flash.opts
---@field mesh integer Which of the item's joints the flash hangs off.
---@field pos? trx.math.Vec3 Offset from that joint, in the joint's own axes. Defaults to the joint itself.
---@field rot_x? trx.math.Angle Pitch of the flash about the offset point. Defaults to `-trx.math.DEG_90`, which is the way the flash meshes are modelled and what the engine draws Lara's own flash with.
---@field object? trx.catalog.objects The object the flash mesh is taken from. Defaults to `trx.catalog.objects.gun_flash`; `trx.catalog.objects.m16_flash` is the longer one a rifle throws.

---Draws a muzzle flash at one of an item's joints for a few frames, and lights
---the area around it where gun lighting is on. This is the flash an enemy
---firing throws, put where a script asks for it, so that an actor who fires in
---a cutscene has one as well.
---
---Raises if this level does not carry the flash object.
---
---```lua
---trx.fx.gun_flash(actor, { mesh = 10, pos = { x = 0, y = 180, z = 55 } })
---```
---@param item trx.items.Item The item the flash is drawn on.
---@param opts trx.fx.gun_flash.opts Where the flash sits and what it is drawn from.
---@return boolean # Whether a flash was drawn.
function M.gun_flash(item, opts)
  local pos = opts.pos or { x = 0, y = 0, z = 0 }
  return raw.gun_flash(
    item.num,
    opts.mesh,
    pos.x,
    pos.y,
    pos.z,
    opts.rot_x or -trx.math.DEG_90,
    opts.object or trx.catalog.objects.gun_flash
  )
end

---@class (exact) trx.fx.knockback.opts
---@field pos trx.math.Vec3 World position.
---@field tilt? trx.math.Angle Maximum tilt for each ring in either direction. `0` keeps them level.
---@trx.default tilt 0

---Spreads a ring of force out from a point, as a blast does. The ring is drawn
---and widens on its own.
---
---```lua
---trx.fx.knockback({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.knockback.opts Where the ring starts and how far it may lean.
function M.knockback(opts)
  raw.knockback(opts.pos.x, opts.pos.y, opts.pos.z, opts.tilt or 0)
end

---Which spark-set sprite a spark is drawn with.
---@enum trx.fx.SparkType
local SparkType = {
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
}
M.SparkType = h.enum("fx.SparkType", "SPARK_SPRITE_TYPE", SparkType)

---The context for which a spark is spawned.
---@enum trx.fx.SparkContext
local SparkContext = {
  DEFAULT = "No specific context.",
  BLOOD = "Blood on an enemy or Lara.",
  BREATH = "Lara's breath in cold rooms.",
  BUBBLE = "Air bubbles either from Lara or weapons underwater.",
  ELECTRICITY = "Electric sparks from fences or enemies.",
  EXPLOSION = "The result of a grenade, rocket or enemy exploding.",
  FIRE = "Any type of flame.",
  GAS = "Toxic gas from mutants.",
  PARTICLE = "Small particles, such as from flares burning.",
  PICKUP_AID = "The twinkle effect shown above pickup items.",
  PLASMA = "Spawned from enemies such as Sophia Lee.",
  RICOCHET = "Spawned when bullets hit walls.",
  SMOKE = "Any type of smoke.",
  SPLASH = "Spawned when hitting water causes a splash.",
  WATER_MIST = "Mist spawned from waterfalls and water vehicles.",
}
M.SparkContext = h.enum("fx.SparkContext", "SPARK_CONTEXT", SparkContext)

---How a sprite is laid over what is behind it.
---@enum trx.fx.DrawType
local DrawType = {
  OPAQUE = "It covers what is behind it.",
  BLEND = "It is mixed with what is behind it.",
  BLEND_ADD = "It is added to what is behind it, so it lightens.",
  BLEND_SUB = "It is taken from what is behind it, so it darkens.",
  REFLECTIVE_OPAQUE = "Opaque, and carrying the room reflection.",
  REFLECTIVE_BLEND_ADD = "Added, and carrying the room reflection.",
}
M.DrawType = h.enum("fx.DrawType", "DRAW_TYPE", DrawType)

---A spark is one particle from the spark pool: a sprite that lives for a set
---number of frames, moving, resizing and fading on its own as it does.
---
---A spark that runs out of life leaves its slot to the next one asked for. A
---handle held across that becomes stale, and field access raises an error.
---@class (exact) trx.fx.Spark
---@trx.readonly draw_type, room_num
---@field life integer Frames of life left. It reaches zero and the spark ends.
---@field life_span integer Frames of life it started with, which the fades are measured against.
---@field pos trx.math.Vec3 Where it sits, in the world, or from what it is attached to. Read `trx.fx.Spark.world_pos` for the position it is drawn at.
---@field vel trx.math.Vec3 How far it moves each frame.
---@field width integer How wide it is drawn now.
---@field height integer How tall it is drawn now.
---@field start_width integer The width it grows from.
---@field start_height integer The height it grows from.
---@field end_width integer The width it grows to.
---@field end_height integer The height it grows to.
---@field color trx.math.Color The color it is drawn in now.
---@field start_color trx.math.Color The color it fades from.
---@field end_color trx.math.Color The color it fades to.
---@field fade_speed integer How fast it travels from one color to the other.
---@field fade_to_black integer How many frames of life are left when it starts to darken toward black.
---@field scalar integer How strongly the size is scaled with distance.
---@field gravity integer How much it is pulled down each frame.
---@field max_y_vel integer As fast as gravity may carry it down.
---@field friction integer How fast it is slowed each frame.
---@field rot_angle integer How far it is turned about the view, from 0 to 4095.
---@field rot_add integer How far it turns each frame.
---@field extras integer How many further sparks it leaves behind as it ends.
---@field node_num integer Which of the sixteen body points it hangs off, where it is attached to one.
---@field room_num trx.rooms.Num The room it was spawned in.
---@field draw_type trx.fx.DrawType How it is laid over what is behind it.
---@field context trx.fx.SparkContext The context for which the spark is spawned.
---@field scales boolean Whether it travels from its start size to its end size.
---@field rotates boolean Whether it turns as it lives.
---@field is_blood boolean Whether it counts as blood, which the pool sheds first when it runs short of slots.
---@field is_outside boolean Whether the wind carries it.
---@field is_underwater boolean Whether it drifts like an underwater particle.
---@field is_green boolean Whether it is drawn in the green of the poison gas.
---@field uses_alt_sprite boolean Whether it is drawn with the second sprite of its kind.
---@field room trx.rooms.Room The room it was spawned in.
---@field world_pos trx.math.Vec3 Where it is drawn, which is where it sits unless it is attached to something that carries it.
---@field item trx.items.Item? The item it is attached to, and `nil` where it hangs on nothing.
local Spark = h.handle("fx.Spark", "SPARK", {
  fields = {
    life = "life",
    life_span = "s_life",
    pos = "pos",
    vel = "vel",
    width = "size.width",
    height = "size.height",
    start_width = "src_size.width",
    start_height = "src_size.height",
    end_width = "dst_size.width",
    end_height = "dst_size.height",
    color = "color",
    start_color = "src_color",
    end_color = "dst_color",
    fade_speed = "col_fade_speed",
    fade_to_black = "fade_to_black",
    scalar = "scalar",
    gravity = "gravity",
    max_y_vel = "max_y_vel",
    friction = "friction",
    rot_angle = "rot_angle",
    rot_add = "rot_add",
    extras = "extras",
    node_num = "node_num",
    room_num = "room_num",
    draw_type = "draw_type",
    context = "context",
    scales = "scales",
    rotates = "rotates",
    is_blood = "is_blood",
    is_outside = "is_outside",
    is_underwater = "is_underwater",
    is_green = "is_green",
    uses_alt_sprite = "uses_alt_sprite",
  },
  writable = {
    "life",
    "life_span",
    "pos",
    "vel",
    "width",
    "height",
    "start_width",
    "start_height",
    "end_width",
    "end_height",
    "color",
    "start_color",
    "end_color",
    "fade_speed",
    "fade_to_black",
    "scalar",
    "gravity",
    "max_y_vel",
    "friction",
    "rot_angle",
    "rot_add",
    "extras",
    "node_num",
    "context",
    "scales",
    "rotates",
    "is_blood",
    "is_outside",
    "is_underwater",
    "is_green",
    "uses_alt_sprite",
  },
  extensions = {
    room = function(spark)
      return trx.rooms[spark.room_num]
    end,
    world_pos = function(spark)
      return raw.get_spark_world_pos(spark)
    end,
    item = function(spark)
      local num = raw.get_spark_item(spark)
      return num and trx.items[num] or nil
    end,
  },
})

---Reports whether the handle still names a live spark.
---
---A spark that runs out of life leaves its slot to the next one asked for.
---@return boolean # False once the spark has ended.
function Spark:is_valid() end

---Ends the spark now and frees its slot.
function Spark:kill() end

---The spark pool: the particles TR3 and TR4 draw their smoke, flames, sparks
---and splashes with.
---
---TR1 and TR2 carry no spark set. There the pool stays empty, and everything
---here returns nothing rather than raising.
---@class (exact) trx.fx.sparks
---@field wind trx.fx.Wind The wind that carries the sparks marked `trx.fx.Spark.is_outside`.
---
---  The engine works this out again every frame from the breeze setting, so a
---  value written here holds for that frame alone.
M.sparks = h.namespace("fx.sparks")

---How many sparks the pool holds. A spark spawned after that takes the slot of
---the one with the least life left.
---@type integer
---@trx.value 400
M.sparks.MAX_COUNT = h.const("fx.sparks.MAX_COUNT", raw.spark_max_count())

---The spark pool, counted from 1. `#trx.fx.sparks.pool` is
---`trx.fx.sparks.MAX_COUNT` rather than how many sparks are alive: a slot
---holding no live spark reads as `nil`.
---
---```lua
---for _, spark in pairs(trx.fx.sparks.pool) do
---  spark.color = trx.math.color(255, 0, 0)
---end
---```
---@type table<integer, trx.fx.Spark?>
M.sparks.pool = h.container("fx.sparks.pool", {
  base = 1,
  get = function(idx)
    return raw.get_spark(idx - 1)
  end,
  count = raw.spark_max_count,
})

---How far the wind carries a spark each frame.
---@trx.record
---@class trx.fx.Wind
---@field x trx.math.Distance The east-west axis.
---@field z trx.math.Distance The north-south axis.

h.properties(M.sparks, "fx.sparks", {
  wind = {
    get = function()
      local x, z = raw.get_smoke_wind()
      return { x = x, z = z }
    end,
    set = function(wind)
      raw.set_smoke_wind(wind.x, wind.z)
    end,
  },
})

---@class (exact) trx.fx.sparks.spawn.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field vel? trx.math.Vec3 How far it moves each frame. Defaults to standing still.
---@field sprite_type? trx.fx.SparkType Which sprite it is drawn with.
---@field spark_context? trx.fx.SparkContext The context for which a spark is spawned.
---@field life? integer How many frames it lives, from 1 to 255.
---@field color? trx.math.Color The color it starts in. Defaults to white.
---@field end_color? trx.math.Color The color it fades to. Defaults to the color it starts in, so it holds one color.
---@field fade_speed? integer How fast it travels between the two colors.
---@field fade_to_black? integer How many frames of life are left when it starts to darken toward black.
---@field width? integer How wide it starts, from 0 to 255.
---@field height? integer How tall it starts, from 0 to 255.
---@field end_width? integer The width it grows to, for a spark that scales. Defaults to the width it starts at.
---@field end_height? integer The height it grows to, for a spark that scales. Defaults to the height it starts at.
---@field scales? boolean Whether it travels from its start size to its end size over its life.
---@field scalar? integer How strongly the size is scaled with distance.
---@field gravity? integer How much it is pulled down each frame.
---@field max_y_vel? integer As fast as gravity may carry it down.
---@field friction? integer How fast it is slowed each frame.
---@field rot_angle? integer How far it starts turned about the view, from 0 to 4095.
---@field rot_add? integer How far it turns each frame, for a spark that turns.
---@field rotates? boolean Whether it turns as it lives.
---@field extras? integer How many further sparks it leaves behind as it ends.
---@field draw_type? trx.fx.DrawType How it is laid over what is behind it.
---@field is_outside? boolean Whether the wind carries it.
---@field is_underwater? boolean Whether it drifts like an underwater particle.
---@field is_green? boolean Whether it is drawn in the green of the poison gas.
---@field uses_alt_sprite? boolean Whether it is drawn with the second sprite of its kind.
---@trx.default sprite_type trx.fx.SparkType.PARTICLE
---@trx.default spark_context trx.fx.SparkContext.DEFAULT
---@trx.default life 16
---@trx.default fade_speed 8
---@trx.default fade_to_black 8
---@trx.default width 4
---@trx.default height 4
---@trx.default scales false
---@trx.default scalar 2
---@trx.default gravity 0
---@trx.default max_y_vel 0
---@trx.default friction 0
---@trx.default rot_angle 0
---@trx.default rot_add 0
---@trx.default rotates false
---@trx.default extras 0
---@trx.default draw_type trx.fx.DrawType.BLEND_ADD
---@trx.default is_outside false
---@trx.default is_underwater false
---@trx.default is_green false
---@trx.default uses_alt_sprite false

---Puts one spark in the world and hands it back, so a script can draw with the
---pool the game draws its own smoke and flames with.
---
---The spark lives for the frames it is given, moving, resizing and fading on
---its own, and then frees its slot. Nothing has to be called each frame to
---keep it up.
---
---Returns `nil` where the level carries no spark set, which is every TR1 and
---TR2 level.
---
---```lua
---trx.fx.sparks.spawn({
---  pos = trx.lara.item.pos,
---  vel = { x = 0, y = -8, z = 0 },
---  sprite_type = trx.fx.SparkType.EXPLOSION,
---  life = 48,
---  color = trx.math.color(255, 200, 64),
---  end_color = trx.math.color(64, 0, 0),
---  width = 16,
---  end_width = 48,
---  scales = true,
---})
---```
---@param opts trx.fx.sparks.spawn.opts What the spark is and how it behaves.
---@return trx.fx.Spark? # The spark, or `nil` where the level carries no spark set.
---@type fun(opts: trx.fx.sparks.spawn.opts): trx.fx.Spark?
M.sparks.spawn = raw.spawn_spark

---@class (exact) trx.fx.sparks.explosion.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field extras? integer How many further sparks each one leaves behind as it ends, from 0 to 3.
---@field light? integer How the fireball lights the room around it: `-2` bright, `-1` dim, `0` not at all.
---@field underwater? boolean Whether it uses the underwater burst.
---@trx.default extras 3
---@trx.default light -2
---@trx.default underwater false

---Throws the fireball of sparks an explosion is drawn with.
---
---```lua
---trx.fx.sparks.explosion({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.explosion.opts Where the fireball is and how strongly it bursts.
function M.sparks.explosion(opts)
  raw.spark_explosion(
    opts.pos.x,
    opts.pos.y,
    opts.pos.z,
    opts.extras or 3,
    opts.light or -2,
    opts.underwater
  )
end

---@class (exact) trx.fx.sparks.explosion_smoke.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field underwater? boolean Whether it lifts as it does under water.
---@field ending? boolean Whether it is the thinner smoke that closes the burst rather than the smoke that opens it.
---@trx.default underwater false
---@trx.default ending false

---Lifts the smoke an explosion leaves behind.
---
---```lua
---trx.fx.sparks.explosion_smoke({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.explosion_smoke.opts Where the smoke is and which part of the burst it is.
function M.sparks.explosion_smoke(opts)
  raw.spark_explosion_smoke(
    opts.pos.x,
    opts.pos.y,
    opts.pos.z,
    opts.underwater,
    opts.ending
  )
end

---@class (exact) trx.fx.sparks.explosion_bubble.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.

---Throws the burst of bubbles an explosion under water makes.
---
---```lua
---trx.fx.sparks.explosion_bubble({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.explosion_bubble.opts Where the bubbles rise.
function M.sparks.explosion_bubble(opts)
  raw.spark_explosion_bubble(opts.pos.x, opts.pos.y, opts.pos.z)
end

---@class (exact) trx.fx.sparks.fire_flame.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field variant? integer Which colors it burns in: `0` orange, `2` pale, `254` green.
---@trx.default variant 0

---Throws one tongue of flame, the way a burning body does. Make the call every
---frame to keep a fire burning.
---
---No flame is thrown more than twenty sectors from Lara.
---
---```lua
---trx.fx.sparks.fire_flame({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.fire_flame.opts Where the flame is and what color it burns.
function M.sparks.fire_flame(opts)
  raw.spark_fire_flame(opts.pos.x, opts.pos.y, opts.pos.z, opts.variant or 0)
end

---@class (exact) trx.fx.sparks.fire_smoke.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field variant? integer Which colors it burns in: `0` orange, `2` pale, `254` green.
---@trx.default variant 0

---Lifts one puff of the smoke a fire gives off.
---
---```lua
---trx.fx.sparks.fire_smoke({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.fire_smoke.opts Where the smoke is and which flame color it follows.
function M.sparks.fire_smoke(opts)
  raw.spark_fire_smoke(opts.pos.x, opts.pos.y, opts.pos.z, opts.variant or 0)
end

---@class (exact) trx.fx.sparks.static_flame.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field size? integer How big it burns.
---@trx.default size 32

---Throws one tongue of the flame a standing fire burns with.
---
---```lua
---trx.fx.sparks.static_flame({ pos = trx.lara.item.pos, size = 64 })
---```
---@param opts trx.fx.sparks.static_flame.opts Where the flame is and how big it burns.
function M.sparks.static_flame(opts)
  raw.spark_static_flame(opts.pos.x, opts.pos.y, opts.pos.z, opts.size or 32)
end

---@class (exact) trx.fx.sparks.side_flame.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field angle trx.math.Angle The way the flame is thrown.
---@field speed? integer How hard it is thrown.
---@field pilot? boolean Whether it is the small pilot flame rather than the jet itself.
---@trx.default speed 32
---@trx.default pilot false

---Throws a tongue of flame sideways, as a jet does.
---
---```lua
---trx.fx.sparks.side_flame({
---  pos = trx.lara.item.pos,
---  angle = trx.lara.item.rot.y,
---})
---```
---@param opts trx.fx.sparks.side_flame.opts Where the jet is and which way it burns.
function M.sparks.side_flame(opts)
  raw.spark_side_flame(
    opts.pos.x,
    opts.pos.y,
    opts.pos.z,
    opts.angle,
    opts.speed or 32,
    opts.pilot
  )
end

---@class (exact) trx.fx.sparks.flamethrower_flame.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.

---Throws the flame a flamethrower leaves where its jet lands.
---
---```lua
---trx.fx.sparks.flamethrower_flame({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.flamethrower_flame.opts Where the flame lands.
function M.sparks.flamethrower_flame(opts)
  raw.spark_flamethrower_flame(opts.pos.x, opts.pos.y, opts.pos.z)
end

---@class (exact) trx.fx.sparks.flamethrower_smoke.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field underwater? boolean Whether it lifts as it does under water.
---@trx.default underwater false

---Lifts the smoke a flamethrower jet leaves behind.
---
---```lua
---trx.fx.sparks.flamethrower_smoke({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.flamethrower_smoke.opts Where the smoke is.
function M.sparks.flamethrower_smoke(opts)
  raw.spark_flamethrower_smoke(
    opts.pos.x,
    opts.pos.y,
    opts.pos.z,
    opts.underwater
  )
end

---@class (exact) trx.fx.sparks.gun_smoke.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field weapon trx.catalog.weapons Which weapon it is drawn for. `UNKNOWN`, `UNARMED`, and out-of-range values raise.
---@field shade? integer How light the smoke is, from 0 to 255.
---@field initial? boolean Whether it is the first puff of a shot, which is denser than the ones that follow.
---@field vel? trx.math.Vec3 Which way the smoke is pushed. Left out, it lifts straight up.
---@trx.default shade 64
---@trx.default initial false

---Lifts the smoke a fired weapon leaves at its muzzle.
---
---```lua
---trx.fx.sparks.gun_smoke({
---  pos = trx.lara.item.pos,
---  weapon = trx.catalog.weapons.pistols,
---  initial = true,
---})
---```
---@param opts trx.fx.sparks.gun_smoke.opts Where the smoke is and which weapon left it.
function M.sparks.gun_smoke(opts)
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
end

---@class (exact) trx.fx.sparks.dart_smoke.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field vel? trx.math.Vec3 How far it moves each frame. Defaults to standing still.
---@field hit? boolean Whether the dart has landed, which puffs the smoke out rather than trailing it.
---@trx.default hit false

---Lifts the trail of smoke a flying dart leaves.
---
---```lua
---trx.fx.sparks.dart_smoke({ pos = trx.lara.item.pos, hit = true })
---```
---@param opts trx.fx.sparks.dart_smoke.opts Where the smoke is and which way the dart flies.
function M.sparks.dart_smoke(opts)
  local vel = opts.vel or { x = 0, z = 0 }
  raw.spark_dart_smoke(
    opts.pos.x,
    opts.pos.y,
    opts.pos.z,
    vel.x,
    vel.z,
    opts.hit
  )
end

---@class (exact) trx.fx.sparks.rocket_smoke.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field shade? integer How light the smoke turns as it fades, from 0 to 191.
---@trx.default shade 0

---Lifts the trail of smoke a flying rocket leaves.
---
---```lua
---trx.fx.sparks.rocket_smoke({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.rocket_smoke.opts Where the smoke is and how light it lifts.
function M.sparks.rocket_smoke(opts)
  raw.spark_rocket_smoke(opts.pos.x, opts.pos.y, opts.pos.z, opts.shade or 0)
end

---@class (exact) trx.fx.sparks.flare.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field vel? trx.math.Vec3 How far it moves each frame. Defaults to standing still.
---@field smoke? boolean Whether smoke is lifted with them.
---@trx.default smoke false

---Throws the sparks a burning flare gives off.
---
---```lua
---trx.fx.sparks.flare({ pos = trx.lara.item.pos, smoke = true })
---```
---@param opts trx.fx.sparks.flare.opts Where the sparks are and which way they fly.
function M.sparks.flare(opts)
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
end

---@class (exact) trx.fx.sparks.shotgun.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field vel? trx.math.Vec3 How far it moves each frame. Defaults to standing still.

---Throws the sparks a shotgun blast strikes off what it hits.
---
---```lua
---trx.fx.sparks.shotgun({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.shotgun.opts Where the sparks are and which way they fly.
function M.sparks.shotgun(opts)
  local vel = opts.vel or { x = 0, y = 0, z = 0 }
  raw.spark_shotgun(opts.pos.x, opts.pos.y, opts.pos.z, vel.x, vel.y, vel.z)
end

---@class (exact) trx.fx.sparks.ricochet.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field angle trx.math.Angle The way the sparks fly.
---@field count? integer How many streaks, from 1 to 255.
---@field smoke_only? boolean Whether smoke is lifted rather than sparks struck. TR4 only.
---@trx.default count 3
---@trx.default smoke_only false

---Strikes the sparks a shot makes where it lands on a wall.
---
---TR4 throws as many streaks as asked for and can lift smoke instead. TR3
---throws one burst, and uses the count as its size.
---
---```lua
---trx.fx.sparks.ricochet({ pos = trx.lara.item.pos, angle = 0 })
---```
---@param opts trx.fx.sparks.ricochet.opts Where the shot lands and which way the sparks fly.
function M.sparks.ricochet(opts)
  raw.spark_ricochet(
    opts.pos.x,
    opts.pos.y,
    opts.pos.z,
    opts.angle,
    opts.count or 3,
    opts.smoke_only
  )
end

---@class (exact) trx.fx.sparks.bubble.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field size? integer How big it is at its smallest.
---@field size_range? integer How much bigger than that it may be drawn.
---@trx.default size 8
---@trx.default size_range 8

---Lifts one bubble through the water.
---
---```lua
---trx.fx.sparks.bubble({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.bubble.opts Where the bubble is and how big it is.
function M.sparks.bubble(opts)
  raw.spark_bubble(
    opts.pos.x,
    opts.pos.y,
    opts.pos.z,
    opts.size or 8,
    opts.size_range or 8
  )
end

---@class (exact) trx.fx.sparks.breath.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field vel? trx.math.Vec3 How far it moves each frame. Defaults to standing still.

---Puffs the cloud of breath a body gives off in the cold.
---
---```lua
---trx.fx.sparks.breath({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.breath.opts Where the breath is and which way it drifts.
function M.sparks.breath(opts)
  local vel = opts.vel or { x = 0, y = 0, z = 0 }
  raw.spark_breath(opts.pos.x, opts.pos.y, opts.pos.z, vel.x, vel.y, vel.z)
end

---@class (exact) trx.fx.sparks.pickup_aid.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field vel? trx.math.Vec3 How far it moves each frame. Defaults to standing still.

---Throws the twinkle that marks a pickup worth reaching.
---
---```lua
---trx.fx.sparks.pickup_aid({ pos = trx.lara.item.pos })
---```
---@param opts trx.fx.sparks.pickup_aid.opts Where the twinkle is and which way it drifts.
function M.sparks.pickup_aid(opts)
  local vel = opts.vel or { x = 0, z = 0 }
  raw.spark_pickup_aid(opts.pos.x, opts.pos.y, opts.pos.z, vel.x, vel.z)
end

---@class (exact) trx.fx.sparks.waterfall_mist.opts
---@field pos trx.math.Vec3 World position. Must lie inside the level.
---@field angle trx.math.Angle The way the waterfall faces.

---Lifts the mist that stands at the foot of a waterfall.
---
---```lua
---trx.fx.sparks.waterfall_mist({ pos = trx.lara.item.pos, angle = 0 })
---```
---@param opts trx.fx.sparks.waterfall_mist.opts Where the mist is and which way it faces.
function M.sparks.waterfall_mist(opts)
  raw.spark_waterfall_mist(opts.pos.x, opts.pos.y, opts.pos.z, opts.angle)
end
