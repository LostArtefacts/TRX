local raw = trxc.math
local h = require("trx.internal.helpers")

-- One full turn in the engine's angle units.
local DEG_360 = 4 * raw.DEG_90

---@class (partial) trx
---@field math trx.math

---Fixed-point trigonometry, matching the engine's own tables. Using these
---rather than Lua's `math` library guarantees a script places things exactly
---where the engine would. `trx.math.Angle` says what an angle is here.
---@trx.module 32
---@class (exact) trx.math
local M = h.module("math")

---An angle in the engine's own units, where 65536 is a full turn rather than
---2 pi. An angle counts in cycles, so one past the end of a turn wraps round
---to name the same direction: adding a half turn to a rotation always works.
---`trx.math.DEG_1` converts from degrees.
---@trx.unit TRX units, angle units
---@alias trx.math.Angle integer

---A length in the units the engine measures the world in, where one sector is
---`trx.math.WALL_L`. Y grows downwards, so a greater Y is further down.
---@trx.unit world units, world coordinates
---@alias trx.math.Distance integer

---A point or a direction in the world.
---@trx.record
---@class trx.math.Vec3
---@field x trx.math.Distance The east-west axis.
---@field y trx.math.Distance The up-down axis.
---@field z trx.math.Distance The north-south axis.

---An orientation, as three angles about the world axes.
---@trx.record
---@class trx.math.Rot
---@field x trx.math.Angle Pitch, nose up and down.
---@field y trx.math.Angle Yaw, the direction it faces.
---@field z trx.math.Angle Roll, the tilt about its own length.

---An axis-aligned box. Whether it is placed in the world or in something's
---own frame is for the call that hands it over to say.
---@class (exact) trx.math.Box
---@field max_x trx.math.Distance East edge.
---@field max_y trx.math.Distance Bottom edge.
---@field max_z trx.math.Distance North edge.
---@field min_x trx.math.Distance West edge.
---@field min_y trx.math.Distance Top edge.
---@field min_z trx.math.Distance South edge.
local Box = h.class("math.Box")

-- A color stores its channels directly. Colors read from a field remember their
-- source, so changing a channel can update that field in the engine.
local function owner_of(color)
  return rawget(color, "_owner"), rawget(color, "_key")
end

local function flush(color)
  local owner, key = owner_of(color)
  if owner ~= nil then
    owner[key] = color
  end
end

local function to_byte(channel)
  local rounded = channel + 0.5 - (channel + 0.5) % 1
  if rounded < 0 then
    return 0
  end
  if rounded > 255 then
    return 255
  end
  return rounded
end

local function channel_field(name, slot)
  return {
    get = function(color)
      return rawget(color, slot)
    end,
    set = function(color, value)
      if type(value) ~= "number" then
        error(("math.Color.%s takes a number"):format(name), 2)
      end
      rawset(color, slot, value)
      flush(color)
    end,
  }
end

---A color, as three channels counted 0 to 255.
---
---Assigning one takes either a color or the hex text a color is written as, so
---`"33e5ff"` and `{ r = 51, g = 229, b = 255 }` say the same thing. A channel
---may also be written on its own, and a color read off something the engine
---owns writes that change straight back to it.
---
---Some colors the engine keeps are stored as fractions rather than bytes, and
---those carry more precision than the hex text shows: a channel of one may
---read back as `191.25`.
---
---```lua
---local water = trx.config.get("visuals.water_color")
---trx.log.info(("water is %s, and %d parts red"):format(water.hex, water.r))
---trx.config.set("visuals.water_color", "33e5ff")
---```
---@class (exact) trx.math.Color
---@operator concat(string): string
---@trx.operator concat A color joins text as its hex, whichever side of the `..` it is on.
---@trx.operator eq Two colors are equal when their channels are.
---@trx.operator tostring The color as its hex text.
---@field b number The blue channel.
---@field g number The green channel.
---@field hex string The color as six hex digits, which is how a setting and a data file spell one. Writing it takes a leading `#` as well.
---@field r number The red channel.
local Color = h.class("math.Color", {
  fields = {
    r = channel_field("r", "_r"),
    g = channel_field("g", "_g"),
    b = channel_field("b", "_b"),
    hex = {
      get = function(color)
        return ("%02x%02x%02x"):format(
          to_byte(rawget(color, "_r")),
          to_byte(rawget(color, "_g")),
          to_byte(rawget(color, "_b"))
        )
      end,
      set = function(color, text)
        local r, g, b = tostring(text):match("^#?(%x%x)(%x%x)(%x%x)$")
        if r == nil or g == nil or b == nil then
          error("math.Color.hex takes six hex digits", 2)
        end
        rawset(color, "_r", tonumber(r, 16))
        rawset(color, "_g", tonumber(g, 16))
        rawset(color, "_b", tonumber(b, 16))
        flush(color)
      end,
    },
  },
  operators = {
    tostring = function(color)
      return color.hex
    end,
    eq = function(a, b)
      return a.r == b.r and a.g == b.g and a.b == b.b
    end,
    concat = function(a, b)
      return tostring(a) .. tostring(b)
    end,
  },
})

local function make_color(r, g, b, owner, key)
  local color = h.new(Color)
  rawset(color, "_r", r)
  rawset(color, "_g", g)
  rawset(color, "_b", b)
  rawset(color, "_owner", owner)
  rawset(color, "_key", key)
  return color
end

-- Every color the engine hands a script is built here, so the type is the one
-- the docs describe rather than a bare table of channels.
trxc.api.set_color_ctor(make_color)

---Builds a color, out of three channels or out of hex text. The color it
---hands back belongs to the caller: assign it somewhere for the engine to take
---it.
---
---```lua
---local gold = trx.math.color("ffbf20")
---local teal = trx.math.color(51, 229, 255)
---```
---@param value string|number The hex text, or the red channel.
---@param g? number The green channel, where the first argument was the red one.
---@param b? number The blue channel.
---@return trx.math.Color
function M.color(value, g, b)
  if type(value) == "string" then
    local color = make_color(0, 0, 0)
    color.hex = value
    return color
  end
  if type(g) ~= "number" or type(b) ~= "number" then
    error("trx.math.color takes hex text, or all three channels", 2)
  end
  return make_color(value, g, b)
end

---Converts an angle from degrees. A whole degree and a part of one both
---remain exact. `trx.math.DEG_1` uses the nearest whole unit and falls four
---units short over a quarter turn.
---
---```lua
---local half_turn = trx.math.degrees(180)
---```
---@param degrees number The angle in degrees.
---@return trx.math.Angle
function M.degrees(degrees)
  return math.floor(degrees * DEG_360 / 360 + 0.5)
end

---Sine of an angle.
---@param angle trx.math.Angle
---@return number # A value in [-1, 1].
---@type fun(angle: trx.math.Angle): number
M.sin = raw.sin

---Cosine of an angle.
---@param angle trx.math.Angle
---@return number # A value in [-1, 1].
---@type fun(angle: trx.math.Angle): number
M.cos = raw.cos

---Angle of the vector (x, z).
---
---```lua
----- face an item towards Lara
---local angle = trx.math.atan(lara.pos.z - pos.z, lara.pos.x - pos.x)
---```
---@param z trx.math.Distance How far the vector reaches north.
---@param x trx.math.Distance How far it reaches east.
---@return trx.math.Angle
---@type fun(z: trx.math.Distance, x: trx.math.Distance): trx.math.Angle
M.atan = raw.atan

---Snaps a position back to the corner of the sector it stands in, the way the
---level's own geometry is laid out. A whole position keeps its height: a
---sector is a column, and rounding it is about the ground plan rather than how
---far up the position sits. A single coordinate rounds on its own, which is
---what an axis at a time needs.
---
---The corner is always the one to the west and the south, on both sides of
---the origin, so two positions in the same sector always answer with the same
---corner.
---
---```lua
----- a zone over the sector Lara stands on, a sector tall.
----- y grows downwards, so the ceiling of the box is the lesser y.
---local corner = trx.math.round_to_sector(trx.lara.item.pos)
---trx.zones.box(corner, {
---  x = corner.x + trx.math.WALL_L,
---  y = corner.y - trx.math.WALL_L,
---  z = corner.z + trx.math.WALL_L,
---})
---```
---@param value trx.math.Vec3|trx.math.Distance A world position, or one coordinate of one.
---@return trx.math.Vec3|trx.math.Distance # The corner of the sector, in whichever of the two came in.
function M.round_to_sector(value)
  if type(value) == "number" then
    return math.floor(value / raw.WALL_L) * raw.WALL_L
  end
  if type(value) ~= "table" then
    error("value must be a position or a coordinate", 3)
  end
  return {
    x = math.floor(value.x / raw.WALL_L) * raw.WALL_L,
    y = value.y,
    z = math.floor(value.z / raw.WALL_L) * raw.WALL_L,
  }
end

---Says a length in sectors, which is how a level is laid out, in the units
---the engine measures the world in. A part of a sector is a length of its
---own, so `0.5` is half a sector.
---
---```lua
----- how far the uzis reach, eight sectors out
---trx.weapons.get(trx.catalog.weapons.uzis).target_dist =
---  trx.math.from_sectors(8)
---```
---@param value number A length in sectors.
---@return trx.math.Distance # The same length.
function M.from_sectors(value)
  if type(value) ~= "number" then
    error("value must be a number", 3)
  end
  return math.floor(value * raw.WALL_L + 0.5)
end

---Says a length in sectors, which is what it reads as on a level's own grid.
---A length that is not a whole number of sectors reads as a fraction.
---
---```lua
---local sectors = trx.math.to_sectors(trx.lara.item.pos.y)
---```
---@param value trx.math.Distance The length to say.
---@return number # The same length in sectors.
function M.to_sectors(value)
  if type(value) ~= "number" then
    error("value must be a number", 3)
  end
  return value / raw.WALL_L
end

---One degree. Multiply by it to say an angle in degrees:
---`45 * trx.math.DEG_1`.
---@type trx.math.Angle
---@trx.value 182
M.DEG_1 = h.const("math.DEG_1", raw.DEG_1)

---A 45-degree turn.
---@type trx.math.Angle
---@trx.value 8192
M.DEG_45 = h.const("math.DEG_45", raw.DEG_45)

---A quarter turn. A full turn is four of these.
---@type trx.math.Angle
---@trx.value 16384
M.DEG_90 = h.const("math.DEG_90", raw.DEG_90)

---The size of one sector. Level geometry is laid out on this grid, so it is
---the step to take to move an item a sector over.
---@type trx.math.Distance
---@trx.value 1024
M.WALL_L = h.const("math.WALL_L", raw.WALL_L)

-- Unused until a module hands one out, which reaches it through h.class_of.
local _ = Box
