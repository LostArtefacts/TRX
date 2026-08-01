local raw = trxc.rooms
local api = trx.api

require("trx.log")

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
    return trx.events.on_room_change(function(item, old_room, new_room)
      if
        pick_room(old_room, new_room) == num
        and (watch == "all" or item == trx.lara.item)
      then
        callback(item)
      end
    end)
  end
end

local ROOM_HOOK_PARAMS = {
  {
    name = "callback",
    type = "function",
    params = {
      {
        name = "item",
        type = "Item",
        description = "The `trx.items.Item` that changed rooms.",
      },
    },
  },
  {
    name = "opts",
    type = "table",
    optional = true,
    description = 'Options. `watch = "lara"` (the default) reacts to Lara alone; `watch = "all"` '
      .. "to every item.",
  },
}

local FLOOR_HEIGHT_POS = {
  name = "pos",
  type = "vec3",
  description = "World position.",
}

local FLOOR_HEIGHT_OPTS = {
  name = "opts",
  type = "table",
  optional = true,
  description = "`fix_tilts`: whether a floor tilt that lies inside a wall is taken into account, "
    .. "`true` by default. `false` gives the flat height the original games read there, which is "
    .. "what the geometry glitches of the vanilla levels rest on.",
}

local FLOOR_HEIGHT_PARAMS = {
  FLOOR_HEIGHT_POS,
  {
    name = "room_num",
    type = "integer",
    optional = true,
    description = "0-based room to look from. The search crosses portals, so a neighbouring "
      .. "room's floor is found too. Without it, the room is looked up from the position, which "
      .. "takes the first room that contains it and passes over the flipped-away ones. Where "
      .. "rooms overlap, name the room, or ask the room itself with `room:floor_height`.",
  },
  FLOOR_HEIGHT_OPTS,
}

local ROOM_LISTENER = {
  type = "events.Listener",
  description = "The attached handler.",
}

api.module("rooms", {
  order = 6,
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
      description = "0-based room number, matching the numbers level editors show.",
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
    on_enter = {
      params = ROOM_HOOK_PARAMS,
      returns = ROOM_LISTENER,
      description = "Happens when something changes rooms into this one.",
      examples = {
        [[trx.rooms[7]:on_enter(function(item)
  trx.log.info("entered room 7")
end)]],
      },
      impl = room_hook(function(old_room, new_room)
        return new_room
      end),
    },
    on_exit = {
      params = ROOM_HOOK_PARAMS,
      returns = ROOM_LISTENER,
      description = "Happens when something changes rooms out of this one.",
      impl = room_hook(function(old_room, new_room)
        return old_room
      end),
    },
    is_valid = {
      returns = { type = "boolean" },
      description = "Whether the handle still refers to a room of the level that is loaded. A level "
        .. "change replaces the rooms, so a handle held across one goes stale rather than naming a "
        .. "different room: reading or writing a field on it raises an error. Check this for a "
        .. "handle held across time.",
      examples = {
        [[local start_room = trx.rooms[0]
trx.events.after_control(function()
  if start_room:is_valid() then
    trx.log.info(tostring(start_room.underwater))
  end
end)]],
      },
    },
    floor_height = {
      params = { FLOOR_HEIGHT_POS, FLOOR_HEIGHT_OPTS },
      returns = { type = "integer", nullable = true },
      description = "As `trx.rooms.floor_height`, looking from this room.",
      impl = function(room, pos, opts)
        return raw.get_height(pos, room.num, opts)
      end,
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
      description = "World-coordinate bounds of the room: `min_x`, `min_y`, `min_z`, `max_x`, "
        .. "`max_y`, `max_z`.",
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
  description = "Retrieves a room by number. Rooms count from zero, matching the "
    .. "room numbers level editors show.",
  params = {
    { name = "num", type = "integer", description = "0-based room number." },
  },
  returns = { type = "Room", nullable = true },
  examples = {
    [[local room = trx.rooms[14]
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

api.property("rooms.flipped", {
  type = "boolean",
  description = "Whether the room map is currently flipped.",
  get = raw.get_flipped,
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
    {
      name = "timer",
      type = "integer",
      optional = true,
      description = "Flip timer value.",
    },
  },
  examples = {
    [[trx.rooms.flip_effect(trx.catalog.flip_effects.floor_shake, 10)]],
  },
  impl = raw.flip_effect,
})

api.define("rooms.floor_height", {
  description = "The height of the floor under a world position. `nil` where there is no floor at "
    .. "all: inside solid geometry, or off the edge of the level.",
  params = FLOOR_HEIGHT_PARAMS,
  returns = { type = "integer", nullable = true },
  examples = {
    [[local floor = trx.lara.item.room:floor_height(trx.lara.item.pos)]],
  },
  impl = raw.get_height,
})

api.define("rooms.find_valid_pos", {
  description = "Nudges a position into valid room geometry, e.g. to find somewhere an item can "
    .. "legally be placed.",
  params = {
    { name = "pos", type = "vec3", description = "Position to search near." },
    {
      name = "room_num",
      type = "integer",
      description = "0-based room to search from.",
    },
  },
  returns = {
    {
      type = "vec3",
      nullable = true,
      description = "The valid position, or `nil` if none was found nearby.",
    },
    { type = "integer", description = "The 0-based room the position is in." },
  },
  impl = raw.find_valid_pos,
})

api.container("rooms", {
  description = "Indexing the module reaches a room, and `#trx.rooms` is how many the level has. "
    .. "Rooms count from zero, matching the room numbers level editors show. `pairs()` walks them "
    .. "in order, keyed by that number.",
  key = { type = "integer", description = "0-based room number." },
  value = { type = "Room", nullable = true },
  examples = {
    [[trx.log.info(#trx.rooms .. " rooms, first is " .. trx.rooms[0].num)
for num, room in pairs(trx.rooms) do
  room.cold = true
end]],
  },
  get = raw.get,
  count = raw.count,
})
