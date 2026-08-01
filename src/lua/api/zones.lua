local raw = trxc.items
local capi_events = trxc.events
local api = trx.api

require("trx.log")
require("trx.math")
require("trx.events")
require("trx.items")
require("trx.rooms")
require("trx.camera")

-- Script-defined trigger regions, tested once a logical frame.
--
-- The module is the whole mechanism. C answers what stands inside a region,
-- which room a position is in, and where the flyby camera is, while the
-- occupancy, the transitions and the order they are reported in are settled
-- here. Every transition goes out as an engine event, so a handler attaches and
-- detaches the way it does for any other.

-- The height a tile reaches, which is every height the level has. A coordinate
-- is 32-bit where the engine reads it, so the span stops there.
local FLOOR_OF_THE_WORLD = -2147483648
local CEILING_OF_THE_WORLD = 2147483647

-- The zones raise their own events rather than any the engine knows about, so
-- the types are declared here and are event types like any other from there on.
local ZONE_EVENTS = {}
for _, name in ipairs({
  "zone_enter",
  "zone_exit",
  "zone_tick",
  "zone_flyby_enter",
  "zone_flyby_exit",
}) do
  ZONE_EVENTS[name] = capi_events.declare(name)
end

-- Every zone, in the order they were made, and the same zones by the id an
-- event carries. An id is never reused within a level, so a transition found
-- before a handler removed its zone cannot land on a zone made afterwards.
local zones = {}
local by_id = {}
local next_id = 1
local lookup

-- A copy of the zone list, for a walk that runs script handlers along the way.
-- One of them may remove a zone, and table.remove would then move the rest out
-- from under the walk.
local function snapshot()
  local out = {}
  for i, zone in ipairs(zones) do
    out[i] = zone
  end
  return out
end

-- What a zone holds. The value a script is handed is an empty table, so nothing
-- it carries is reachable except through the declaration below.
local state = setmetatable({}, { __mode = "k" })

local function own_of(self)
  local own = state[self]
  if own == nil or own.removed then
    error("stale Zone handle", 3)
  end
  return own
end

-- The per-frame test runs while there is a zone to test, so a level with none
-- pays nothing for the module being loaded.
local driver = nil
local control

local function drive()
  if driver == nil and #zones > 0 then
    driver = trx.events.after_control(control)
  elseif driver ~= nil and #zones == 0 then
    driver:detach()
    driver = nil
  end
end

-- Whether a position is inside the region, by the shape the zone was made with.
-- Which room a tile belongs to is checked separately: a position alone does not
-- say which of two stacked rooms holds it.
local function holds_pos(own, pos)
  if own.shape == "sphere" then
    local dx = pos.x - own.centre.x
    local dy = pos.y - own.centre.y
    local dz = pos.z - own.centre.z
    return dx * dx + dy * dy + dz * dz <= own.radius * own.radius
  end
  return pos.x >= own.min.x
    and pos.x <= own.max.x
    and pos.y >= own.min.y
    and pos.y <= own.max.y
    and pos.z >= own.min.z
    and pos.z <= own.max.z
end

-- Whether the item counts as an occupant: held by the world, standing inside
-- the region, and for a tile in the room the tile belongs to.
local function holds_item(own, item)
  if item == nil or not item:is_valid() or not item.is_present then
    return false
  end
  if own.room_num ~= nil and item.room_num ~= own.room_num then
    return false
  end
  return holds_pos(own, item.pos)
end

-- The item numbers inside the zone this frame, in order. Watching Lara asks
-- after her position and nothing else, which is the common case and the cheap
-- one; watching everything asks the engine what stands in the region and
-- narrows what comes back.
local function occupants_now(own)
  local out = {}
  if own.watch == "lara" then
    local lara = trx.lara.item
    if holds_item(own, lara) then
      out[1] = lara.num
    end
    return out
  end

  local found = own.shape == "sphere" and raw.in_sphere(own.centre, own.radius)
    or raw.in_box(own.min, own.max)
  for _, num in ipairs(found) do
    if holds_item(own, trx.items[num]) then
      out[#out + 1] = num
    end
  end
  return out
end

-- One frame. Occupancy settles for every zone before anything is reported, so
-- what a handler does to the zones cannot change what this frame found; the
-- dispatch then asks whether each zone is still there.
--
-- Every exit is reported, then every enter, then a tick per occupant, so a
-- script watching two zones hears about the one an item left before the one it
-- entered.
function control()
  local exits, enters, ticks = {}, {}, {}
  local flyby_exits, flyby_enters = {}, {}

  -- Where the flyby camera is this frame, and nil when no sequence is playing.
  -- Read once rather than once per zone, and not at all the rest of the time.
  local flyby = nil
  if trx.camera.is_flyby_active then
    flyby = { pos = trx.camera.pos, room_num = trx.camera.room_num }
  end

  for _, zone in ipairs(zones) do
    local own = state[zone]
    if own.enabled then
      local flyby_inside = flyby ~= nil
        and (own.room_num == nil or flyby.room_num == own.room_num)
        and holds_pos(own, flyby.pos)
      if flyby_inside and not own.flyby then
        flyby_enters[#flyby_enters + 1] = { own.id }
      elseif own.flyby and not flyby_inside then
        flyby_exits[#flyby_exits + 1] = { own.id }
      end
      own.flyby = flyby_inside

      local inside = occupants_now(own)
      local now = {}
      for _, num in ipairs(inside) do
        now[num] = true
      end

      for _, num in ipairs(own.order) do
        if not now[num] then
          exits[#exits + 1] = { own.id, num }
        end
      end
      for _, num in ipairs(inside) do
        if not own.occupants[num] then
          enters[#enters + 1] = { own.id, num }
        end
        ticks[#ticks + 1] = { own.id, num }
      end
      own.occupants, own.order = now, inside
    end
  end

  local phases = {
    { ZONE_EVENTS.zone_exit, exits },
    { ZONE_EVENTS.zone_flyby_exit, flyby_exits },
    { ZONE_EVENTS.zone_enter, enters },
    { ZONE_EVENTS.zone_flyby_enter, flyby_enters },
    { ZONE_EVENTS.zone_tick, ticks },
  }
  for _, phase in ipairs(phases) do
    for _, transition in ipairs(phase[2]) do
      if by_id[transition[1]] ~= nil then
        capi_events.fire(phase[1], transition[1], transition[2])
      end
    end
  end
end

-- An item destroyed inside a zone leaves it, and this is the moment to say so:
-- the handle still resolves here, and by the next frame there is no way left to
-- name what went.
--
-- A disabled zone forgets the item as readily but reports nothing, the way it
-- reports nothing else. An item number is a slot the world hands out again, so
-- an occupant kept past its destruction would name the item that took the slot
-- rather than the one that left.
trx.events.on_destroy(function(item)
  local num = item.num
  for _, zone in ipairs(snapshot()) do
    local own = state[zone]
    if not own.removed and own.occupants[num] then
      own.occupants[num] = nil
      for i, held in ipairs(own.order) do
        if held == num then
          table.remove(own.order, i)
          break
        end
      end
      if own.enabled then
        capi_events.fire(ZONE_EVENTS.zone_exit, own.id, num)
      end
    end
  end
end)

local function detach_hooks(own)
  for _, listener in ipairs(own.listeners) do
    listener:detach()
  end
  own.listeners = {}
end

-- Zones are level-bound, like the coordinates they are made of. They go when
-- the engine lets go of the script that made them, and the next level's script
-- makes its own, as it does its handlers.
local function clear()
  for _, zone in ipairs(zones) do
    local own = state[zone]
    detach_hooks(own)
    own.removed = true
  end
  zones = {}
  by_id = {}
  next_id = 1
  drive()
end

trx.events.on_level_unload(clear)

api.module("zones", {
  order = 15,
  description = [[
A zone is a piece of the level worth keeping an eye on. Mark out a box or a
sphere in world space, or a single sector the way a floor trigger covers one,
and the zone reports when something steps into it, when it leaves, and for as
long as it stays.

A zone watches Lara and nobody else, unless it is made with `watch = "items"`,
and then it watches everything the level holds - enemies, pickups, anything
with a position. Its hooks hand over whatever set it off. To listen in one
place rather than zone by zone, `trx.events.on_zone_enter` and its siblings
hear about all of them.

A flyby camera passing through a zone is its own pair of hooks,
`trx.zones.Zone:on_flyby_enter` and `trx.zones.Zone:on_flyby_exit`, since a
camera is not an item and there is nothing to hand a handler but the zone.
Every zone answers for a flyby, whatever it watches, and a tile answers for one
in the room the tile belongs to. The camera is checked only while a sequence is
playing.

Every frame, each zone settles who is inside before anything goes out, and the
exits come before the enters - so a script following Lara from one zone into
the next hears her leave the first before she enters the second. Something
destroyed while it is inside counts as leaving.

A box and a sphere take no notice of rooms, so where two rooms sit one above
the other over the same ground, a box tall enough to reach both catches what
stands in either. Only a tile belongs to a room. A flipmap changes the geometry
under a zone but moves nothing and renumbers nothing, so no zone sees anything
come or go.

Zones belong to the level that made them. A level change clears them, and the
level script makes them again the same way it attaches its handlers. A zone made
outside a level script - from a global script, or from the console - goes with
the next level change as well, and nothing makes it again. A global script that
wants a zone in every level makes it from a handler that runs once the level has
loaded. They are not written to savegames either, so Lara standing in a zone when
a game is loaded enters it again.
]],
})

api.number("zones.Num", {
  base = 1,
  description = "Where a zone sits among the level's, counted in the order they were made. An "
    .. "earlier zone being removed shifts the rest along.",
})

local ZONE = { type = "zones.Zone", description = "The zone." }
local LISTENER = {
  type = "events.Listener",
  description = "The listener. `trx.zones.Zone:remove` detaches what a zone carries as well.",
}

local function hook_params(what)
  return {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "item",
          type = "items.Item",
          description = "The item that " .. what .. ".",
        },
      },
    },
  }
end

-- A hook is one listener on the matching global event, filtered down to this
-- zone: a transition wakes the hooks of every zone, and each answers only for
-- its own. The zone holds on to its listeners, so removing it takes them along.
local attach_hook

-- The zone events are attached to through C rather than through a hook
-- trx.events declares, so the listener a script is handed is made here. The
-- key it carries the engine's number under is trx.events' own.
local Listener = api.class("events.Listener")

local function listener_of(id)
  return setmetatable({ _id = id }, Listener)
end

local Zone = api.type("zones.Zone", {
  description = "A script-defined trigger region. Reading a field of one that has been removed, or "
    .. "that a level change took away, raises rather than answering for a zone that is no longer "
    .. "there.",

  fields = {
    type = {
      type = "string",
      description = 'The shape the zone was made with: `"box"`, `"sphere"` or `"tile"`.',
      get = function(self)
        return own_of(self).type
      end,
    },
    num = {
      type = "zones.Num",
      description = "Where the zone sits now.",
      get = function(self)
        own_of(self)
        for i, zone in ipairs(zones) do
          if zone == self then
            return i
          end
        end
        error("stale Zone handle", 2)
      end,
    },
    name = {
      type = "string",
      description = "The name the zone was made with, and `nil` for one made without. "
        .. "`trx.zones[name]` finds it again.",
      get = function(self)
        return own_of(self).name
      end,
    },
    enabled = {
      type = "boolean",
      description = "Whether the zone is tested. Disabling it suspends the hooks without forgetting "
        .. "who is inside, so an item that leaves while it is off is reported as leaving when it "
        .. "comes back on. One destroyed while it is off is forgotten instead, and nothing is "
        .. "reported for it.",
      get = function(self)
        return own_of(self).enabled
      end,
      set = function(self, value)
        own_of(self).enabled = value and true or false
      end,
    },
    watch = {
      type = "string",
      description = 'What sets the zone off: `"lara"` or `"items"`.',
      get = function(self)
        return own_of(self).watch
      end,
    },
    min = {
      type = "math.Vec3",
      description = "The lower corner of a box or a tile, and `nil` for a sphere.",
      get = function(self)
        return own_of(self).min
      end,
    },
    max = {
      type = "math.Vec3",
      description = "The upper corner of a box or a tile, and `nil` for a sphere.",
      get = function(self)
        return own_of(self).max
      end,
    },
    centre = {
      type = "math.Vec3",
      description = "The middle of a sphere, and `nil` for a box or a tile.",
      get = function(self)
        return own_of(self).centre
      end,
    },
    radius = {
      type = "math.Distance",
      description = "How far a sphere reaches, and `nil` for a box or a tile.",
      get = function(self)
        return own_of(self).radius
      end,
    },
    room_num = {
      type = "rooms.Num",
      description = "The room a tile belongs to, which is what keeps the same sector column in the "
        .. "room above or below from setting it off. Settled when the zone is made and fixed from "
        .. "then on, a flipmap included. `nil` for a box or a sphere.",
      get = function(self)
        return own_of(self).room_num
      end,
    },
  },

  methods = {
    enable = {
      description = "Starts testing the zone again. The same as `zone.enabled = true`.",
      impl = function(self)
        own_of(self).enabled = true
      end,
    },

    disable = {
      description = "Stops testing the zone, without forgetting who is inside, other than an "
        .. "occupant destroyed meanwhile. The same as `zone.enabled = false`.",
      impl = function(self)
        own_of(self).enabled = false
      end,
    },

    contains_point = {
      description = "Whether a world position lies inside the region. A plain test: no hooks are "
        .. "involved, and a disabled zone answers as readily as any other. A tile answers on "
        .. "position alone, so a point in the room stacked above it counts here where an item "
        .. "standing there would not.",
      params = {
        { name = "pos", type = "math.Vec3", description = "World position." },
      },
      returns = {
        type = "boolean",
        description = "Whether the point is inside.",
      },
      impl = function(self, pos)
        return holds_pos(own_of(self), pos)
      end,
    },

    contains_item = {
      description = "Whether the item is inside the region: the same test the zone makes every "
        .. "frame, so this answers whether the item counts as an occupant. An item is tested by the "
        .. "point it stands at, and one the world does not hold is nowhere.",
      params = { { name = "item", type = "items.Item" } },
      returns = {
        type = "boolean",
        description = "Whether the item counts as an occupant.",
      },
      examples = {
        [[if plate:contains_item(trx.lara.item) then
  trx.log.info("she is standing on it")
end]],
      },
      impl = function(self, item)
        return holds_item(own_of(self), item)
      end,
    },

    occupants = {
      description = "The items inside the zone as of the last frame it was tested, in item order. A "
        .. "disabled zone hands back who was inside when it was disabled.",
      returns = { type = "items.Item", list = true },
      impl = function(self)
        local out = {}
        for _, num in ipairs(own_of(self).order) do
          local item = trx.items[num]
          if item ~= nil then
            out[#out + 1] = item
          end
        end
        return out
      end,
    },

    clear_occupants = {
      description = "Forgets who is inside, so anything still there enters again on the next frame. "
        .. "This is how a zone is made to fire a second time for an item that never left it. A "
        .. "flyby passing through is forgotten with the rest.",
      impl = function(self)
        local own = own_of(self)
        own.occupants, own.order, own.flyby = {}, {}, false
      end,
    },

    on_enter = {
      description = "Happens when something enters the zone.",
      params = hook_params("entered"),
      returns = LISTENER,
      examples = {
        [[local door = trx.zones.box(
  { x = 51200, y = -2048, z = 30720 },
  { x = 53248, y = 0, z = 32768 })
door:on_enter(function(item)
  trx.log.info("someone stepped in")
end)]],
      },
      impl = function(self, callback)
        return attach_hook(self, "on_zone_enter", callback)
      end,
    },

    on_exit = {
      description = "Happens when something leaves the zone, and when something inside it is "
        .. "destroyed.",
      params = hook_params("left"),
      returns = LISTENER,
      impl = function(self, callback)
        return attach_hook(self, "on_zone_exit", callback)
      end,
    },

    on_tick = {
      description = "Happens on every logical frame something is inside the zone, including the "
        .. "frame it enters.",
      params = hook_params("is inside"),
      returns = LISTENER,
      impl = function(self, callback)
        return attach_hook(self, "on_zone_tick", callback)
      end,
    },

    on_flyby_enter = {
      description = "Happens when a flyby camera enters the zone. A flyby is not an item and sets "
        .. "off no other hook, so a handler takes nothing: the zone is the whole of what happened.",
      params = {
        {
          name = "callback",
          type = "function",
          description = "What to run when it happens.",
        },
      },
      returns = LISTENER,
      examples = {
        [[local hall = trx.zones.box(
  { x = 51200, y = -2048, z = 30720 },
  { x = 53248, y = 0, z = 32768 })
hall:on_flyby_enter(function()
  trx.music.play(trx.catalog.music.main_theme)
end)]],
      },
      impl = function(self, callback)
        return attach_hook(self, "on_zone_flyby_enter", callback)
      end,
    },

    on_flyby_exit = {
      description = "Happens when a flyby camera leaves the zone, and when the sequence ends while "
        .. "the camera is still inside one.",
      params = {
        {
          name = "callback",
          type = "function",
          description = "What to run when it happens.",
        },
      },
      returns = LISTENER,
      impl = function(self, callback)
        return attach_hook(self, "on_zone_flyby_exit", callback)
      end,
    },

    remove = {
      description = "Removes the zone and detaches the hooks attached to it. Its handle goes stale: "
        .. "`trx.zones.Zone:is_valid` says so, and reading a field raises.",
      impl = function(self)
        local own = own_of(self)
        detach_hooks(own)
        own.removed = true
        by_id[own.id] = nil
        for i, zone in ipairs(zones) do
          if zone == self then
            table.remove(zones, i)
            break
          end
        end
        drive()
      end,
    },

    is_valid = {
      description = "Whether the zone still exists. `trx.zones.Zone:remove` and a level change both leave a "
        .. "handle stale.",
      returns = {
        type = "boolean",
        description = "Whether the zone is still there.",
      },
      impl = function(self)
        local own = state[self]
        return own ~= nil and not own.removed
      end,
    },
  },
})

-- What the event carries beyond the zone is what the hook hands over: the item
-- for the three that name one, and nothing at all for a flyby.
function attach_hook(zone, event_name, callback)
  local own = own_of(zone)
  local listener = trx.events[event_name](function(fired, ...)
    if fired == zone then
      callback(...)
    end
  end)
  own.listeners[#own.listeners + 1] = listener
  return listener
end

-- A flyby event names the zone and nothing else.
local function flyby_event(event_type)
  return function(callback)
    return listener_of(capi_events.attach(event_type, function(zone_id)
      local zone = by_id[zone_id]
      if zone ~= nil then
        callback(zone)
      end
    end))
  end
end

local FLYBY_PARAMS = {
  {
    name = "callback",
    type = "function",
    description = "What to run when it happens.",
    params = {
      {
        name = "zone",
        type = "zones.Zone",
        description = "The `trx.zones.Zone` the camera entered or left.",
      },
    },
  },
}

-- The zone events are declared here because narrowing one to the zone it is
-- about is this module's own bookkeeping. They are engine events like any
-- other, so trx.events.detach takes their listeners and a level script's are
-- dropped when the level ends.
local function zone_event(event_type)
  return function(callback)
    return listener_of(
      capi_events.attach(event_type, function(zone_id, item_num)
        local zone = by_id[zone_id]
        if zone ~= nil then
          callback(zone, trx.items[item_num])
        end
      end)
    )
  end
end

local EVENT_PARAMS = {
  {
    name = "callback",
    type = "function",
    description = "What to run when it happens.",
    params = {
      {
        name = "zone",
        type = "zones.Zone",
        description = "The `trx.zones.Zone` the moment is about.",
      },
      {
        name = "item",
        type = "items.Item",
        description = "The `trx.items.Item` that entered, left, or is inside.",
      },
    },
  },
}

local EVENT_LISTENER = {
  type = "events.Listener",
  description = "The attached handler.",
}

api.define("events.on_zone_enter", {
  description = "Happens when something enters a zone. Fires for every zone; `trx.zones.Zone:on_enter` is "
    .. "the same moment narrowed to one of them.",
  params = EVENT_PARAMS,
  returns = EVENT_LISTENER,
  examples = {
    [[trx.events.on_zone_enter(function(zone, item)
  trx.log.info("something entered " .. tostring(zone.name))
end)]],
  },
  impl = zone_event(ZONE_EVENTS.zone_enter),
})

api.define("events.on_zone_exit", {
  description = "Happens when something leaves a zone, and when something inside one is destroyed.",
  params = EVENT_PARAMS,
  returns = EVENT_LISTENER,
  impl = zone_event(ZONE_EVENTS.zone_exit),
})

api.define("events.on_zone_tick", {
  description = "Happens on every logical frame something is inside a zone, including the frame it "
    .. "enters.",
  params = EVENT_PARAMS,
  returns = EVENT_LISTENER,
  impl = zone_event(ZONE_EVENTS.zone_tick),
})

api.define("events.on_zone_flyby_enter", {
  description = "Happens when a flyby camera enters a zone. Fires for every zone; "
    .. "`trx.zones.Zone:on_flyby_enter` is the same moment narrowed to one of them.",
  params = FLYBY_PARAMS,
  returns = EVENT_LISTENER,
  impl = flyby_event(ZONE_EVENTS.zone_flyby_enter),
})

api.define("events.on_zone_flyby_exit", {
  description = "Happens when a flyby camera leaves a zone, and when the sequence ends while the "
    .. "camera is still inside one.",
  params = FLYBY_PARAMS,
  returns = EVENT_LISTENER,
  impl = flyby_event(ZONE_EVENTS.zone_flyby_exit),
})

local WATCHES = { lara = true, items = true }

local function check_opts(opts)
  local watch = opts ~= nil and opts.watch or "lara"
  if not WATCHES[watch] then
    error('watch must be "lara" or "items"', 4)
  end
  local name = opts ~= nil and opts.name or nil
  if name ~= nil then
    if type(name) ~= "string" then
      error("name must be a string", 4)
    end
    -- A name is what trx.zones[name] answers by, so two zones sharing one would
    -- leave the second unreachable.
    if lookup(name) ~= nil then
      error("a zone named '" .. name .. "' is already there", 4)
    end
  end
  return watch, name
end

local function add(own, opts)
  own.watch, own.name = check_opts(opts)
  own.id = next_id
  own.enabled = true
  own.occupants = {}
  own.order = {}
  own.listeners = {}
  next_id = next_id + 1

  local zone = setmetatable({}, Zone)
  state[zone] = own
  zones[#zones + 1] = zone
  by_id[own.id] = zone
  drive()
  return zone
end

local WATCH_OPTS = {
  name = "opts",
  type = "table",
  optional = true,
  description = "How the zone is watched, and what it is called.",
  fields = {
    {
      name = "watch",
      type = "string",
      optional = true,
      default = "lara",
      description = 'What sets the zone off: `"lara"` tests Lara alone, and `"items"` tests '
        .. "every item the level holds.",
    },
    {
      name = "name",
      type = "string",
      optional = true,
      description = "A name `trx.zones[name]` finds the zone by. Raises where the level already "
        .. "has a zone of that name.",
    },
  },
}

api.define("zones.box", {
  description = "Creates a zone from a world-space box. The corners may come in any order. Rooms "
    .. "play no part: what stands inside the box is inside it, whichever room holds it.",
  params = {
    {
      name = "min",
      type = "math.Vec3",
      description = "One corner of the box.",
    },
    {
      name = "max",
      type = "math.Vec3",
      description = "The opposite corner of the box.",
    },
    WATCH_OPTS,
  },
  returns = ZONE,
  examples = {
    [[local arena = trx.zones.box(
  { x = 51200, y = -2048, z = 30720 },
  { x = 53248, y = 0, z = 32768 },
  { watch = "items", name = "arena" })]],
  },
  impl = function(min, max, opts)
    return add({
      type = "box",
      shape = "box",
      min = {
        x = math.min(min.x, max.x),
        y = math.min(min.y, max.y),
        z = math.min(min.z, max.z),
      },
      max = {
        x = math.max(min.x, max.x),
        y = math.max(min.y, max.y),
        z = math.max(min.z, max.z),
      },
    }, opts)
  end,
})

api.define("zones.sphere", {
  description = "Creates a zone from a point and a radius. Rooms play no part, as they do not for "
    .. "`trx.zones.box`.",
  params = {
    {
      name = "centre",
      type = "math.Vec3",
      description = "Middle of the sphere.",
    },
    {
      name = "radius",
      type = "math.Distance",
      description = "How far out it reaches.",
    },
    WATCH_OPTS,
  },
  returns = ZONE,
  examples = {
    [[local bell = trx.zones.sphere(trx.lara.item.pos, 2048, { watch = "items" })]],
  },
  impl = function(centre, radius, opts)
    if radius < 0 then
      error("radius must not be negative", 3)
    end
    return add({
      type = "sphere",
      shape = "sphere",
      centre = { x = centre.x, y = centre.y, z = centre.z },
      radius = radius,
    }, opts)
  end,
})

api.define("zones.tile", {
  description = "Creates a zone from the sector under a position, in the room holding that "
    .. "position, at any height - the way a floor trigger occupies a sector. The same sector column "
    .. "in the room above or below does not set it off. Where rooms overlap and several of them "
    .. "hold the position, the zone takes the first, which is the lowest-numbered room rather than "
    .. "the nearest floor; `trx.zones.Zone.room_num` says which one it settled on.",
  params = {
    {
      name = "pos",
      type = "math.Vec3",
      description = "A world position inside the sector.",
    },
    WATCH_OPTS,
  },
  returns = {
    type = "zones.Zone",
    nullable = true,
    description = "The zone, or `nil` when the position lies outside the level.",
  },
  examples = {
    [[local plate = trx.zones.tile(trx.lara.item.pos)
plate:on_enter(function(item)
  trx.log.info("stepped on the plate")
end)]],
  },
  impl = function(pos, opts)
    local room = trx.rooms.query:at(pos):first()
    if room == nil then
      return nil
    end
    local corner = trx.math.round_to_sector(pos)
    return add({
      type = "tile",
      shape = "box",
      min = { x = corner.x, y = FLOOR_OF_THE_WORLD, z = corner.z },
      max = {
        x = corner.x + trx.math.WALL_L - 1,
        y = CEILING_OF_THE_WORLD,
        z = corner.z + trx.math.WALL_L - 1,
      },
      room_num = room.num,
    }, opts)
  end,
})

function lookup(key)
  if type(key) == "number" then
    return zones[key]
  end
  for _, zone in ipairs(zones) do
    if state[zone].name == key then
      return zone
    end
  end
  return nil
end

api.define("zones.get", {
  description = "Retrieves a zone by its place in the module or by the name it was made with. The "
    .. "same as indexing the module.",
  params = {
    {
      name = "key",
      type = { "zones.Num", "string" },
      description = "Where the zone sits, or the name it was made with.",
    },
  },
  returns = { type = "zones.Zone", nullable = true },
  impl = lookup,
})

api.define("zones.count", {
  description = "How many zones the level has. The same as `#trx.zones`.",
  returns = {
    type = "integer",
    description = "How many zones the level holds.",
  },
  impl = function()
    return #zones
  end,
})

api.container("zones", {
  description = "Indexing the module reaches a zone, by the order the zones were made or by the "
    .. "name one was made with. `#trx.zones` is how many there are, and `pairs()` walks them in "
    .. "that order.",
  key = {
    type = { "zones.Num", "string" },
    description = "Where the zone sits, or the name it was made with. A script holds the zone "
      .. "itself, or the name it gave it, rather than the number.",
  },
  value = { type = "zones.Zone", nullable = true },
  examples = {
    [[trx.log.info(#trx.zones .. " zones, the first is a " .. trx.zones[1].type)
for _, zone in pairs(trx.zones) do
  zone:disable()
end]],
  },
  get = lookup,
  count = function()
    return #zones
  end,
})
