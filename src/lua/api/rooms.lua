local raw = trxc.rooms
local api = trx.api

require("trx.math")

local Box = api.class("math.Box")

require("trx.log")
require("trx.query")

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

local ROOM_HOOK_PARAMS = {
  {
    name = "callback",
    type = "function",
    description = "What to run when it happens.",
    params = {
      {
        name = "item",
        type = "items.Item",
        description = "The item that changed rooms.",
      },
    },
  },
  {
    name = "opts",
    type = "table",
    optional = true,
    description = "What to watch for.",
    fields = {
      {
        name = "watch",
        type = "string",
        optional = true,
        default = "lara",
        description = 'Either `"lara"`, which reacts to Lara alone, or `"all"`, which reacts '
          .. "to every item.",
      },
    },
  },
}

local FLOOR_HEIGHT_POS = {
  name = "pos",
  type = "math.Vec3",
  description = "World position.",
}

local FLOOR_HEIGHT_OPTS = {
  name = "opts",
  type = "table",
  optional = true,
  description = "How to read the floor.",
  fields = {
    {
      name = "fix_tilts",
      type = "boolean",
      optional = true,
      default = true,
      description = "Whether a floor tilt that lies inside a wall is taken into account. "
        .. "`false` gives the flat height the original games read there, which is what the "
        .. "geometry glitches of the vanilla levels rest on.",
    },
  },
}

local FLOOR_HEIGHT_PARAMS = {
  FLOOR_HEIGHT_POS,
  {
    name = "room_num",
    type = "rooms.Num",
    optional = true,
    description = "The search crosses portals, so a neighbouring "
      .. "room's floor is found too. Without it, the room is looked up from the position, which "
      .. "takes the first room that contains it and passes over the flipped-away ones. Where "
      .. "rooms overlap, name the room, or ask the room itself with `trx.rooms.Room:floor_height`.",
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
  description = "The values `trx.rooms.Room.flip_status` can take.",
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
      from = "room_num",
      type = "rooms.Num",
      writable = false,
    },
    underwater = {
      from = "flags.underwater",
      type = "boolean",
      description = "Whether the room is filled with water.",
    },
    swamp = {
      from = "flags.swamp",
      type = "boolean",
      description = "Whether the room is filled with swamp water, which Lara wades through and "
        .. "sinks into rather than swimming.",
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
      type = "rooms.FlipStatus",
      writable = false,
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
      impl = room_hook(function(old_room_num, new_room_num)
        return new_room_num
      end),
    },
    on_exit = {
      params = ROOM_HOOK_PARAMS,
      returns = ROOM_LISTENER,
      description = "Happens when something changes rooms out of this one.",
      impl = room_hook(function(old_room_num, new_room_num)
        return old_room_num
      end),
    },
    is_valid = {
      returns = {
        type = "boolean",
        description = "False once the level that held the room has been left.",
      },
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
      returns = {
        type = "math.Distance",
        nullable = true,
        description = "The height, with `nil` where there is no floor.",
      },
      description = "As `trx.rooms.floor_height`, looking from this room.",
      impl = function(room, pos, opts)
        return raw.get_height(pos, room.num, opts)
      end,
    },
  },

  extensions = {
    flipped_room = {
      type = "rooms.Room",
      description = "This room's flip pair, or `nil` if it has none.",
      impl = function(room)
        local num = raw.get_flipped_room(room)
        return num and trx.rooms[num] or nil
      end,
    },
    bounds = {
      type = "math.Box",
      description = "Where the room sits in the world.",
      impl = function(room)
        return setmetatable(raw.get_bounds(room), Box)
      end,
    },
    internal_bounds = {
      type = "math.Box",
      description = "As `trx.rooms.Room.bounds`, but excluding the outer ring of sectors, which "
        .. "is solid wall.",
      impl = function(room)
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
  },
})

api.number("rooms.Num", {
  base = 0,
  description = "Room number, matching the numbers level editors show.",
})

api.define("rooms.get", {
  description = "Retrieves a room by number.",
  params = {
    { name = "num", type = "rooms.Num" },
  },
  returns = {
    type = "rooms.Room",
    nullable = true,
    description = "The room, or `nil` where the level has no such number.",
  },
  examples = {
    [[local room = trx.rooms[14]
room.underwater = true]],
  },
  impl = raw.get,
})

api.define("rooms.count", {
  description = "Returns the number of rooms in the level. Same as `#trx.rooms`.",
  returns = {
    type = "integer",
    description = "How many rooms the loaded level holds.",
  },
  impl = raw.count,
})

api.property("rooms.flip_group_count", {
  type = "integer",
  description = [[How many flip groups a level can hold. A room belongs to one of them, and a flip
    moves that group alone.]],
  get = raw.flip_group_count,
})

api.define("rooms.flip_groups", {
  description = [[Puts rooms in flip groups. A level script can then move some flip pairs while the
    rest stay where they are. Each entry names one room and the group it belongs to. Its flip pair
    joins the same group.

    Call this only from the top level of a level script. Rooms must be grouped before the level
    starts, so the game can restore flipped groups correctly when it loads a save.

    A level with no groups moves all flip pairs together. After a script names any group, each flip
    trigger moves only the group with the same number.]],
  params = {
    {
      name = "groups",
      type = "table",
      description = "Flip groups, keyed by `trx.rooms.Num`.",
    },
  },
  examples = {
    [[trx.rooms.flip_groups({ [33] = 1, [37] = 2 })]],
  },
  impl = function(groups)
    for room_num, group in pairs(groups) do
      if math.type(room_num) ~= "integer" then
        error("a room is named by number", 2)
      end
      if math.type(group) ~= "integer" then
        error("a flip group is named by number", 2)
      end
      raw.declare_flip_group(room_num, group)
    end
  end,
})

local FLIP_GROUP_PARAM = {
  name = "group",
  type = "integer",
  optional = true,
  description = [[Which flip group to act on, counted from 0. A level splits its flip pairs into
    groups and moves one at a time; a game that names no group places every room in the first.
    Omit this to act on every group.]],
}

api.define("rooms.flip", {
  description = [[Flips rooms, swapping each with its flip pair. With no group given, every group
    moves.]],
  params = { FLIP_GROUP_PARAM },
  examples = {
    [[trx.rooms.flip()]],
    [[trx.rooms.flip(3)]],
  },
  impl = raw.flip,
})

api.define("rooms.is_flipped", {
  description = [[Whether a group of rooms is showing its flip pairs. With no group given, answers
    for the group that moved last, which is what the world itself reads.]],
  params = { FLIP_GROUP_PARAM },
  returns = {
    {
      type = "boolean",
      description = "Whether that group is showing its pairs.",
    },
  },
  impl = raw.get_flipped,
})

api.property("rooms.flipped", {
  type = "boolean",
  description = "Whether the group that moved last is showing its flip pairs.",
  get = raw.get_flipped,
})

api.define("rooms.flip_effect", {
  description = "Sets the active flip effect, and optionally its timer.",
  params = {
    {
      name = "effect_id",
      type = "catalog.flip_effects",
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
  returns = {
    type = "math.Distance",
    nullable = true,
    description = "The height, with `nil` where there is no floor.",
  },
  examples = {
    [[local floor = trx.lara.item.room:floor_height(trx.lara.item.pos)]],
  },
  impl = raw.get_height,
})

api.define("rooms.find_valid_pos", {
  description = "Nudges a position into valid room geometry, e.g. to find somewhere an item can "
    .. "legally be placed.",
  params = {
    {
      name = "pos",
      type = "math.Vec3",
      description = "Position to search near.",
    },
    {
      name = "room_num",
      type = "rooms.Num",
    },
  },
  returns = {
    {
      type = "math.Vec3",
      nullable = true,
      description = "The valid position, or `nil` if none was found nearby.",
    },
    {
      type = "rooms.Num",
      description = "The room the position is in.",
    },
  },
  impl = raw.find_valid_pos,
})

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
local function flag_narrowing(field, description)
  return {
    description = description,
    returns = { type = "query.Query", description = "The narrowed query." },
    impl = trx.query.narrowing(function()
      return function(_num, room)
        return room[field]
      end
    end),
  }
end

local RoomQuery = api.type("rooms.RoomQuery", {
  extends = "query.Query",
  description = "A `trx.query.Query` over the rooms of the current level, with the narrowings below "
    .. "on top of the ones every query has. Rooms answer to no names, so the name layer is absent.",

  methods = {
    underwater = flag_narrowing(
      "underwater",
      "The room is filled with water."
    ),
    swamp = flag_narrowing("swamp", "The room is filled with swamp water."),
    dry = {
      description = "The room holds neither water nor swamp water.",
      returns = { type = "query.Query", description = "The narrowed query." },
      impl = trx.query.narrowing(function()
        return function(_num, room)
          return not room.underwater and not room.swamp
        end
      end),
    },

    reachable = {
      description = "The room is part of the level as it stands: an ordinary room, or the half of "
        .. "a flip pair the level is showing. This is what a script asking about the world wants, "
        .. "and what `trx.rooms.RoomQuery:at` already applies.",
      returns = { type = "query.Query", description = "The narrowed query." },
      examples = { [[trx.rooms.query:reachable():underwater():count()]] },
      impl = trx.query.narrowing(function()
        return function(_num, room)
          return room.flip_status ~= trx.rooms.FlipStatus.FLIPPED
        end
      end),
    },
    flipped = {
      description = "The room is the half of a flip pair the level is not showing. Its geometry is "
        .. "still there to inspect, but nothing can be in it.",
      returns = { type = "query.Query", description = "The narrowed query." },
      impl = trx.query.narrowing(function()
        return function(_num, room)
          return room.flip_status == trx.rooms.FlipStatus.FLIPPED
        end
      end),
    },

    at = {
      description = "The room contains a world position. Rooms overlap, so a position can be in "
        .. "several at once and every one of them matches, in room order. A room claims a point "
        .. "when the point is within its bounds, the outer ring of solid wall aside, and the "
        .. "column it stands in has a floor - the test the engine itself puts a position through. "
        .. "The hidden half of a flip pair is passed over.",
      params = {
        { name = "pos", type = "math.Vec3", description = "World position." },
      },
      returns = { type = "query.Query", description = "The narrowed query." },
      examples = { [[trx.rooms.query:at(trx.lara.item.pos):first()]] },
      impl = trx.query.narrowing(function(pos)
        return function(_num, room)
          -- A flipped room holds the half of a flip pair the level is not
          -- showing. Its geometry still covers the point, and the engine's own
          -- lookup passes it over, so this does too.
          return room.flip_status ~= trx.rooms.FlipStatus.FLIPPED
            and raw.point_inside(room, pos)
        end
      end),
    },
  },
})

local room_query = trx.query.new({
  enumerate = enumerate,
  id_of = function(i)
    return i
  end,
}, RoomQuery)

api.property("rooms.query", {
  type = "rooms.RoomQuery",
  description = "The identity query over every room in the level. Narrow it and read it.",
  get = function()
    return room_query
  end,
})

api.container("rooms", {
  description = "Indexing the module reaches a room, and `#trx.rooms` is how many the level has. "
    .. "`pairs()` walks them in order, keyed by the room number.",
  key = { type = "rooms.Num" },
  value = { type = "rooms.Room", nullable = true },
  examples = {
    [[trx.log.info(#trx.rooms .. " rooms, first is " .. trx.rooms[0].num)
for num, room in pairs(trx.rooms) do
  room.cold = true
end]],
  },
  get = raw.get,
  count = raw.count,
})
