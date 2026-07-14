local raw = trxc.rooms
local api = trx.api

require("trx.log")

api.module("rooms", {
  order = 10,
  description = "Module for inspecting and altering the rooms of the current level.",
})

api.enum("rooms.FlipStatus", {
  backing = "ROOM_FLIP_STATUS",
  description = "The values `room.flip_status` can take.",
  values = {
    NONE = "This is a normal room.",
    UNFLIPPED = "This room is currently reachable by Lara.",
    FLIPPED = "This room is currently inactive and unreachable by Lara.",
  },
})

api.type("rooms.Room", {
  backing = "ROOM",
  description = "A room in the current level.",

  fields = {
    num = {
      from = "room_index",
      type = "integer",
      writable = false,
      description = "1-based room number.",
    },
    underwater = {
      from = "flags.underwater",
      type = "boolean",
      description = "Whether the room is filled with water.",
    },
    wind = {
      from = "flags.wind",
      type = "boolean",
      description = "Whether the room has a breeze. Requires the player to have breeze enabled.",
    },
    damaging = {
      from = "flags.damaging",
      type = "boolean",
      description = "Whether the room drains Lara's exposure meter.",
    },
    cold = {
      from = "flags.cold",
      type = "boolean",
      description = "Whether Lara's breath is visible in the room.",
    },
    flip_status = {
      from = "flip_status",
      type = "integer",
      writable = false,
      enum = "rooms.FlipStatus",
      description = "Current flip status.",
    },
    -- Deliberately not exposed: pos, size, ambient, light and mesh counts,
    -- item_num, effect_num, water_scheme, reverb_info, alternate_group and the
    -- remaining flags. They are engine internals, not a contract.
  },

  methods = {
    is_valid = {
      returns = { type = "boolean" },
      description = "Whether the handle still refers to a room of the level that is loaded. A level "
        .. "change replaces the rooms, so a handle held across one goes stale rather than naming a "
        .. "different room: reading or writing a field on it raises an error. Check this for a "
        .. "handle held across time.",
      examples = {
        [[local start_room = trx.rooms[1]
trx.events.after_control(function()
  if start_room:is_valid() then
    trx.log.info(tostring(start_room.underwater))
  end
end)]],
      },
    },
  },

  extensions = {
    flipped_room = {
      type = "Room",
      description = "This room's flip pair, or `nil` if it has none.",
      impl = function(room)
        local num = raw.get_flipped_room(room)
        return num and trx.rooms[num] or nil
      end,
    },
    bounds = {
      type = "table",
      description = "World-coordinate bounds of the room: `min_x`, `min_y`, `min_z`, `max_x`, " .. "`max_y`, `max_z`.",
      impl = function(room)
        return raw.get_bounds(room)
      end,
    },
    internal_bounds = {
      type = "table",
      description = "As `bounds`, but excluding the outer ring of sectors, which is solid wall.",
      impl = function(room)
        local b = raw.get_bounds(room)
        return {
          min_x = b.min_x + 1024,
          min_y = b.min_y,
          min_z = b.min_z + 1024,
          max_x = b.max_x - 1024,
          max_y = b.max_y,
          max_z = b.max_z - 1024,
        }
      end,
    },
  },
})

api.define("rooms.get", {
  description = "Retrieves a room by 1-based index.",
  params = { { name = "num", type = "integer", description = "1-based room number." } },
  returns = { type = "Room", nullable = true },
  examples = {
    [[local room = trx.rooms[15]
room.underwater = true]],
  },
  impl = raw.get,
})

api.define("rooms.count", {
  description = "Returns the number of rooms in the level. Same as `#trx.rooms`.",
  returns = { type = "integer" },
  impl = raw.count,
})

api.define("rooms.flip", {
  description = "Flips the current room map, swapping every room with its flip pair.",
  impl = raw.flip,
})

api.define("rooms.flip_effect", {
  description = "Sets the active flip effect, and optionally its timer.",
  params = {
    {
      name = "effect_id",
      type = "integer",
      enum = "catalog.flip_effects",
      description = "Use `-1` to clear the current effect.",
    },
    { name = "timer", type = "integer", optional = true, description = "Flip timer value." },
  },
  examples = {
    [[trx.rooms.flip_effect(trx.catalog.flip_effects.floor_shake, 10)]],
  },
  impl = raw.flip_effect,
})

api.define("rooms.find_valid_pos", {
  description = "Nudges a position into valid room geometry, e.g. to find somewhere an item can "
    .. "legally be placed.",
  params = {
    { name = "pos", type = "vec3", description = "Position to search near." },
    { name = "room_num", type = "integer", description = "1-based room to search from." },
  },
  returns = {
    { type = "vec3", nullable = true, description = "The valid position, or `nil` if none was found nearby." },
    { type = "integer", description = "The 1-based room the position is in." },
  },
  impl = raw.find_valid_pos,
})

api.container("rooms", {
  description = "Indexing the module reaches a room, and `#trx.rooms` is how many the level has.",
  key = { type = "integer", description = "1-based room number." },
  value = { type = "Room", nullable = true },
  examples = { [[trx.log.info(#trx.rooms .. " rooms, first is " .. trx.rooms[1].num)]] },
  get = raw.get,
  count = raw.count,
})
