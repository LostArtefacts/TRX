local raw = trxc.rooms
local h = require("trx.internal.helpers")

require("trx.math")

local Box = h.class_of("math.Box")

require("trx.log")
require("trx.query")

---@class trx
---@field rooms trx.rooms

-- on_enter and on_exit narrow trx.events.on_room_change to one room: the two
-- readings of a room change are that this room is the new one, or the old one.
local function room_hook(pick_room)
  return function(room, callback, opts)
    if opts ~= nil and type(opts) ~= "table" then
      error("opts must be a table", 2)
    end
    local watch = opts ~= nil and opts.watch or "lara"
    if watch ~= "lara" and watch ~= "all" then
      error('watch must be "lara" or "all"', 2)
    end
    local num = room.num
    return trx.events.on_room_change(function(item, old_room_num, new_room_num)
      if
        pick_room(old_room_num, new_room_num) == num
        and (watch == "all" or item == trx.lara.item)
      then
        callback(item)
      end
    end)
  end
end

---@class (exact) trx.rooms.Room.on_enter.opts
---@field watch? string Either `"lara"`, which reacts to Lara alone, or `"all"`, which reacts to every item.
---@trx.default watch "lara"

---@class (exact) trx.rooms.floor_height.opts
---@field fix_tilts? boolean Whether a floor tilt that lies inside a wall is taken into account. `false` gives the flat height read by the original games. Vanilla level geometry can depend on this behaviour.
---@trx.default fix_tilts true

---Module for inspecting and altering the rooms of the current level.
---@trx.module 7
---@class (exact) trx.rooms: table<trx.rooms.Num, trx.rooms.Room?>
---@trx.readonly flip_group_count, flipped, query
---@field flip_group_count integer How many flip groups a level can hold. A room belongs to one of them, and a flip moves that group alone.
---@field flipped boolean Whether the group that moved last is showing its flip pairs.
---@field query trx.rooms.RoomQuery The identity query over every room in the level. Narrow it and read it.
local M = h.module("rooms")

---The values `trx.rooms.Room.flip_status` can take.
---@enum trx.rooms.FlipStatus
local FlipStatus = {
  NONE = "This is a normal room.",
  UNFLIPPED = "This room is currently reachable by Lara.",
  FLIPPED = "This room is currently inactive and unreachable by Lara.",
}
M.FlipStatus = h.enum("rooms.FlipStatus", "ROOM_FLIP_STATUS", FlipStatus)

---A room in the current level.
---@class (exact) trx.rooms.Room
---@trx.readonly flip_status, num
---@field num trx.rooms.Num
---@field underwater boolean Whether the room is filled with water.
---@field swamp boolean Whether the room is filled with swamp water, which Lara wades through and sinks into rather than swimming.
---@field wind boolean Whether the room has a breeze. Requires the player to have breeze enabled.
---@field damaging boolean Whether the room drains Lara's exposure meter.
---@field cold boolean Whether Lara's breath is visible in the room.
---@field flip_status trx.rooms.FlipStatus Current flip status.
---@field flipped_room trx.rooms.Room This room's flip pair, or `nil` if it has none.
---@field bounds trx.math.Box Where the room sits in the world.
---@field internal_bounds trx.math.Box As `trx.rooms.Room.bounds`, but excluding the outer ring of sectors, which is solid wall.
local Room = h.handle("rooms.Room", "ROOM", {
  fields = {
    num = "room_num",
    underwater = "flags.underwater",
    swamp = "flags.swamp",
    wind = "flags.wind",
    damaging = "flags.damaging",
    cold = "flags.cold",
    flip_status = "flip_status",
    -- Deliberately not exposed: pos, size, ambient, light and mesh counts,
    -- item_num, effect_num, water_scheme, reverb_info, alternate_group and the
    -- remaining flags. They are engine internals, not a contract.
  },
  writable = { "underwater", "swamp", "wind", "damaging", "cold" },
  extensions = {
    flipped_room = function(room)
      local num = raw.get_flipped_room(room)
      return num and trx.rooms[num] or nil
    end,
    bounds = function(room)
      return setmetatable(raw.get_bounds(room), Box)
    end,
    internal_bounds = function(room)
      local b = raw.get_bounds(room)
      return setmetatable({
        min_x = b.min_x + 1024,
        min_y = b.min_y,
        min_z = b.min_z + 1024,
        max_x = b.max_x - 1024,
        max_y = b.max_y,
        max_z = b.max_z - 1024,
      }, Box)
    end,
  },
})

local on_enter = room_hook(function(old_room_num, new_room_num)
  return new_room_num
end)

---Happens when something changes rooms into this one.
---
---```lua
---trx.rooms[7]:on_enter(function(item)
---  trx.log.info("entered room 7")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@param opts? trx.rooms.Room.on_enter.opts What to watch for.
---@trx.arg callback.item The item that changed rooms.
---@return trx.events.Listener # The attached handler.
function Room:on_enter(callback, opts)
  return on_enter(self, callback, opts)
end

local on_exit = room_hook(function(old_room_num, new_room_num)
  return old_room_num
end)

---Happens when something changes rooms out of this one.
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@param opts? trx.rooms.Room.on_enter.opts What to watch for.
---@trx.arg callback.item The item that changed rooms.
---@return trx.events.Listener # The attached handler.
function Room:on_exit(callback, opts)
  return on_exit(self, callback, opts)
end

---Whether the handle still refers to a room of the level that is loaded. A
---level change replaces the rooms, so a handle held across one goes stale
---rather than naming a different room: reading or writing a field on it raises
---an error. Check this for a handle held across time.
---
---```lua
---local start_room = trx.rooms[0]
---trx.events.after_control(function()
---  if start_room:is_valid() then
---    trx.log.info(tostring(start_room.underwater))
---  end
---end)
---```
---@return boolean # False once the level that held the room has been left.
function Room:is_valid() end

---As `trx.rooms.floor_height`, looking from this room.
---@param pos trx.math.Vec3 World position.
---@param opts? trx.rooms.floor_height.opts How to read the height.
---@return trx.math.Distance? # The height, with `nil` where there is no floor.
function Room:floor_height(pos, opts)
  return raw.get_height(pos, self.num, opts)
end

---Returns the ceiling height, using this room as the starting room.
---@param pos trx.math.Vec3 World position.
---@param opts? trx.rooms.floor_height.opts How to read the height.
---@return trx.math.Distance? # The height, with `nil` where there is no ceiling.
function Room:ceiling_height(pos, opts)
  return raw.get_ceiling(pos, self.num, opts)
end

---Room number, matching the numbers level editors show.
---@trx.base 0
---@alias trx.rooms.Num integer

---Retrieves a room by number.
---
---```lua
---local room = trx.rooms[14]
---room.underwater = true
---```
---@param num trx.rooms.Num
---@return trx.rooms.Room? # The room, or `nil` where the level has no such number.
---@type fun(num: trx.rooms.Num): trx.rooms.Room?
M.get = raw.get

---Returns the number of rooms in the level. Same as `#trx.rooms`.
---@return integer # How many rooms the loaded level holds.
---@type fun(): integer
M.count = raw.count

---Puts rooms in flip groups. A level script can then move some flip pairs
---while the rest stay where they are. Each entry names one room and the group
---it belongs to. Its flip pair joins the same group.
---
---Call this only from the top level of a level script. Rooms must be grouped
---before the level starts, so the game can restore flipped groups correctly
---when it loads a save.
---
---A level with no groups moves all flip pairs together. After a script names
---any group, each flip trigger moves only the group with the same number.
---
---```lua
---trx.rooms.flip_groups({ [33] = 1, [37] = 2 })
---```
---@param groups table Flip groups, keyed by `trx.rooms.Num`.
function M.flip_groups(groups)
  for room_num, group in pairs(groups) do
    if math.type(room_num) ~= "integer" then
      error("a room is named by number", 2)
    end
    if math.type(group) ~= "integer" then
      error("a flip group is named by number", 2)
    end
    raw.declare_flip_group(room_num, group)
  end
end

---Flips rooms, swapping each with its flip pair. With no group given, every
---group moves.
---
---```lua
---trx.rooms.flip()
---```
---
---```lua
---trx.rooms.flip(3)
---```
---@param group? integer Which flip group to act on, counted from 0. A level splits its flip pairs into groups and moves one at a time; a game that names no group places every room in the first. Omit this to act on every group.
---@type fun(group?: integer)
M.flip = raw.flip

---Whether a group of rooms is showing its flip pairs. With no group given,
---answers for the group that moved last, which is what the world itself reads.
---@param group? integer Which flip group to act on, counted from 0. A level splits its flip pairs into groups and moves one at a time; a game that names no group places every room in the first. Omit this to act on every group.
---@return boolean # Whether that group is showing its pairs.
---@type fun(group?: integer): boolean
M.is_flipped = raw.get_flipped

---Sets the active flip effect, and optionally its timer.
---
---```lua
---trx.rooms.flip_effect(trx.catalog.flip_effects.floor_shake, 10)
---```
---@param effect_id trx.catalog.flip_effects Use `-1` to clear the current effect.
---@param timer? integer Flip timer value.
---@type fun(effect_id: trx.catalog.flip_effects, timer?: integer)
M.flip_effect = raw.flip_effect

---The height of the floor under a world position. `nil` where there is no
---floor at all: inside solid geometry, or off the edge of the level.
---
---```lua
---local floor = trx.lara.item.room:floor_height(trx.lara.item.pos)
---```
---@param pos trx.math.Vec3 World position.
---@param room_num? trx.rooms.Num The search crosses portals, so a neighbouring room's floor is found too. Without it, the room is looked up from the position, which takes the first room that contains it and passes over the flipped-away ones. Where rooms overlap, name the room, or ask the room itself with `trx.rooms.Room:floor_height`.
---@param opts? trx.rooms.floor_height.opts How to read the height.
---@return trx.math.Distance? # The height, with `nil` where there is no floor.
---@type fun(pos: trx.math.Vec3, room_num?: trx.rooms.Num, opts?: trx.rooms.floor_height.opts): trx.math.Distance?
M.floor_height = raw.get_height

---The height of the ceiling over a world position. Returns `nil` inside solid
---geometry or outside the level.
---
---```lua
---local ceiling = trx.lara.item.room:ceiling_height(trx.lara.item.pos)
---```
---@param pos trx.math.Vec3 World position.
---@param room_num? trx.rooms.Num The search crosses portals, so a neighbouring room's floor is found too. Without it, the room is looked up from the position, which takes the first room that contains it and passes over the flipped-away ones. Where rooms overlap, name the room, or ask the room itself with `trx.rooms.Room:floor_height`.
---@param opts? trx.rooms.floor_height.opts How to read the height.
---@return trx.math.Distance? # The height, with `nil` where there is no ceiling.
---@type fun(pos: trx.math.Vec3, room_num?: trx.rooms.Num, opts?: trx.rooms.floor_height.opts): trx.math.Distance?
M.ceiling_height = raw.get_ceiling

---Nudges a position into valid room geometry, e.g. to find somewhere an item
---can legally be placed.
---@param pos trx.math.Vec3 Position to search near.
---@param room_num trx.rooms.Num
---@return trx.math.Vec3? # The valid position, or `nil` if none was found nearby.
---@return trx.rooms.Num # The room the position is in.
---@type fun(pos: trx.math.Vec3, room_num: trx.rooms.Num): trx.math.Vec3?, trx.rooms.Num
M.find_valid_pos = raw.find_valid_pos

-- Every room of the level, each by its number.
local function enumerate()
  local out = {}
  for i = 0, raw.count() - 1 do
    local room = raw.get(i)
    if room ~= nil then
      out[#out + 1] = { i, room }
    end
  end
  return out
end

-- One of a room's own true-or-false flags, as a narrowing.
local function flag_narrowing(field)
  return trx.query.narrowing(function()
    return function(_num, room)
      return room[field]
    end
  end)
end

---A `trx.query.Query` over the rooms of the current level, with the narrowings
---below on top of the ones every query has. Rooms answer to no names, so the
---name layer is absent.
---@class (exact) trx.rooms.RoomQuery: trx.query.Query
local RoomQuery = h.class("rooms.RoomQuery", { extends = "query.Query" })

local underwater = flag_narrowing("underwater")

---The room is filled with water.
---@return trx.query.Query # The narrowed query.
function RoomQuery:underwater()
  return underwater(self)
end

local swamp = flag_narrowing("swamp")

---The room is filled with swamp water.
---@return trx.query.Query # The narrowed query.
function RoomQuery:swamp()
  return swamp(self)
end

local dry = trx.query.narrowing(function()
  return function(_num, room)
    return not room.underwater and not room.swamp
  end
end)

---The room holds neither water nor swamp water.
---@return trx.query.Query # The narrowed query.
function RoomQuery:dry()
  return dry(self)
end

local reachable = trx.query.narrowing(function()
  return function(_num, room)
    return room.flip_status ~= trx.rooms.FlipStatus.FLIPPED
  end
end)

---The room is part of the level as it stands: an ordinary room, or the half of
---a flip pair the level is showing. This is what a script asking about the
---world wants, and what `trx.rooms.RoomQuery:at` already applies.
---
---```lua
---trx.rooms.query:reachable():underwater():count()
---```
---@return trx.query.Query # The narrowed query.
function RoomQuery:reachable()
  return reachable(self)
end

local flipped = trx.query.narrowing(function()
  return function(_num, room)
    return room.flip_status == trx.rooms.FlipStatus.FLIPPED
  end
end)

---The room is the half of a flip pair the level is not showing. Its geometry
---is still there to inspect, but nothing can be in it.
---@return trx.query.Query # The narrowed query.
function RoomQuery:flipped()
  return flipped(self)
end

local at = trx.query.narrowing(function(pos)
  return function(_num, room)
    -- A flipped room holds the half of a flip pair the level is not
    -- showing. Its geometry still covers the point, and the engine's own
    -- lookup passes it over, so this does too.
    return room.flip_status ~= trx.rooms.FlipStatus.FLIPPED
      and raw.point_inside(room, pos)
  end
end)

---The room contains a world position. Rooms overlap, so a position can be in
---several at once and every one of them matches, in room order. A room claims
---a point when the point is within its bounds, the outer ring of solid wall
---aside, and the column it stands in has a floor - the test the engine itself
---puts a position through. The hidden half of a flip pair is passed over.
---
---```lua
---trx.rooms.query:at(trx.lara.item.pos):first()
---```
---@param pos trx.math.Vec3 World position.
---@return trx.query.Query # The narrowed query.
function RoomQuery:at(pos)
  return at(self, pos)
end

local room_query = trx.query.new({
  enumerate = enumerate,
  id_of = function(i)
    return i
  end,
}, RoomQuery)

h.properties(M, "rooms", {
  flip_group_count = { get = raw.flip_group_count },
  flipped = { get = raw.get_flipped },
  query = {
    get = function()
      return room_query
    end,
  },
})

---Indexing the module reaches a room, and `#trx.rooms` is how many the level
---has. `pairs()` walks them in order, keyed by the room number.
---
---```lua
---trx.log.info(#trx.rooms .. " rooms, first is " .. trx.rooms[0].num)
---for num, room in pairs(trx.rooms) do
---  room.cold = true
---end
---```
---@type table<trx.rooms.Num, trx.rooms.Room?>
h.container("rooms", { base = 0, get = raw.get, count = raw.count }, M)
