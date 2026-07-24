local raw = trxc.items
local api = trx.api

require("trx.query")

api.module("items", {
  order = 4,
  description = "Module for controlling all moveables.",
})

api.enum("items.Status", {
  backing = "ITEM_STATUS",
  description = "The values `item.status` can take.",
  values = {
    INACTIVE = "In the level, but not yet triggered. Its control routine does not run.",
    ACTIVE = "Triggered: its control routine runs every frame.",
    DEACTIVATED = "Ran and finished - a creature that died, or a one-shot trigger that fired. It stays "
      .. "in the level, but no longer runs.",
    INVISIBLE = "Neither drawn nor collidable, as a pickup Lara has already collected is.",
  },
})

api.enum("items.PickupMode", {
  backing = "PICKUP_MODE",
  description = "The values the `pickup_mode` item property can take. It selects the animation Lara "
    .. "plays when collecting the item.",
  values = {
    NORMAL = "Picked up off the floor.",
    PLINTH_LOW = "Picked up from a low pedestal.",
    PLINTH_HIGH = "Picked up from a high pedestal.",
  },
})

api.enum("items.TriggerType", {
  backing = "ITEM_TRIGGER_KIND",
  description = "The kind of trigger `item:trigger` fires, matching the trigger types a level editor "
    .. "offers. Most are forward triggers that differ only in what trips them in a level; from a "
    .. "script they behave alike, and `TRIGGER` is the one to reach for.",
  values = {
    TRIGGER = "A plain trigger: sets the code bits and, once they are all set, starts the item.",
    ANTITRIGGER = "Takes the trigger back, clearing the code bits. The item is left running so it "
      .. "can stand itself down, which is how a door animates shut.",
    SWITCH = "Toggles the code bits, so firing it a second time takes the trigger back.",
    HEAVY = "A forward trigger a heavy object trips. A falling block reads this to know it was set "
      .. "off by weight.",
    HEAVY_SWITCH = "A switch a heavy object trips.",
  },
})

-- Item handles are bare userdata. Their metatable is populated by the api.type
-- declaration below, and by nothing else: a member of the C ITEM struct that is
-- not named here is not reachable from a script at all.

local function make_properties(item)
  return setmetatable({}, {
    __index = function(_, key)
      if type(key) ~= "string" then
        return nil
      end
      return item:get_property(key)
    end,
    __newindex = function(_, key, value)
      item:set_property(key, value)
    end,
    __pairs = function()
      local names = item:get_property_names()
      local i = 0
      return function()
        i = i + 1
        local name = names[i]
        if name == nil then
          return nil
        end
        return name, item:get_property(name)
      end
    end,
  })
end

-- on_trigger narrows the global event to this one item. trx.events is reached
-- at call time, so its module need not load before this one.
local function item_hook(event_name)
  return function(item, callback)
    return trx.events[event_name](function(fired, ...)
      if fired == item then
        callback(fired, ...)
      end
    end)
  end
end

local ITEM_LISTENER_ID = {
  type = "integer",
  description = "Listener id. Pass it to `trx.events.detach` to stop listening.",
}

api.type("items.Item", {
  backing = "ITEM",
  description = "An item, also known as a moveable.",

  fields = {
    pos = {
      from = "pos",
      type = "vec3",
      description = "World position. Updating this also updates `room` and `room_num`.",
    },
    rot = { from = "rot", type = "vec3", description = "Orientation." },
    anim = {
      from = "anim",
      type = "integer",
      description = "Object-relative animation number, 0-indexed.",
    },
    frame = {
      from = "frame",
      type = "integer",
      description = "Object-relative frame number, 0-indexed. Negative values count back from the end.",
    },
    index = {
      from = "index",
      type = "integer",
      writable = false,
      description = "The index `trx.items[i]` takes, counted from 0. An item handed over by a query "
        .. "can say where it lives.",
    },
    room_num = {
      from = "room_index",
      type = "integer",
      writable = false,
      description = "0-based number of the room containing this item. Set `pos` to move the item between rooms.",
    },
    hit_points = {
      from = "hit_points",
      type = "integer",
      description = "Current hit points. Raising this above the maximum also raises `properties.max_hit_points`.",
    },
    max_hit_points = {
      from = "max_hit_points",
      type = "integer",
      writable = false,
      description = "Maximum hit points. Set `properties.max_hit_points` to change it.",
    },
    name = {
      from = "name",
      type = "string",
      description = "Unique item name, or `nil`. Assigning a name already in use raises an error.",
    },
    object_id = {
      from = "object_id",
      type = "integer",
      writable = false,
      enum = "catalog.objects",
      description = "The item's object type.",
    },
    status = {
      from = "status",
      type = "integer",
      writable = false,
      enum = "items.Status",
      description = "Item status. Use `activate()` and `kill()` to change it, so the item's "
        .. "active-list membership stays in sync.",
    },
    flags = {
      from = "flags",
      type = "integer",
      writable = false,
      description = "Trigger-related flag bits. Read-only: writing them directly would let a script set `IF_DESTROYED` without unlinking the item, wedging engine state. Use `kill()` instead.",
    },
    timer = {
      from = "timer",
      type = "integer",
      description = "How long the item's trigger keeps it going, in game frames. `0` runs it until "
        .. "something takes the trigger back; `-1` means it has run out; anything else counts down. "
        .. "This is the raw frame count - `trigger()` takes its timer in seconds instead.",
    },
    is_triggered = {
      from = "is_triggered",
      type = "boolean",
      writable = false,
      description = "Whether the item's trigger currently says go. This is what a door, a switch or "
        .. "an alarm reads to decide whether to act; a creature ignores it and goes by whether it "
        .. "is running.\n\n"
        .. "It is a verdict on `trigger_mask`, `timer` and `is_reversed` together, not a field of "
        .. "its own.",
    },
    trigger_mask = {
      from = "trigger_mask",
      type = "integer",
      description = "The five code bits, counted the way a level editor counts them: `1` to `31`. "
        .. "The trigger only says go once every bit is set, which is how a level makes several "
        .. "triggers agree before anything happens. A lone trigger carries all of them.",
    },
    is_reversed = {
      from = "is_reversed",
      type = "boolean",
      description = "Whether the item's trigger is inverted, so it runs until triggered rather than "
        .. "once triggered. This is how a level ships something already on.",
    },
    speed = {
      from = "speed",
      type = "integer",
      description = "Forward speed.",
    },
    fall_speed = {
      from = "fall_speed",
      type = "integer",
      description = "Vertical speed.",
    },
    gravity = {
      from = "gravity",
      type = "boolean",
      description = "Whether gravity applies to this item.",
    },
    collidable = {
      from = "collidable",
      type = "boolean",
      description = "Whether Lara can collide with this item.",
    },
    is_alive = {
      from = "is_alive",
      type = "boolean",
      writable = false,
      description = "Whether the item is a living creature with hit points remaining.",
    },
    is_killed = {
      from = "is_killed",
      type = "boolean",
      writable = false,
      description = "Whether the item has already been killed.",
    },
    is_one_shot = {
      from = "is_one_shot",
      type = "boolean",
      description = "Whether the item's trigger has been spent and will never fire again.",
    },
    is_hostile = {
      from = "is_hostile",
      type = "boolean",
      writable = false,
      description = "Whether this item is a creature currently hostile to Lara.",
    },
    is_active = {
      from = "active",
      type = "boolean",
      writable = false,
      description = "Whether the item's control routine is running. Call `activate()` to start it.",
    },
    was_hit = {
      from = "hit_status",
      type = "boolean",
      writable = false,
      description = "Whether the item was hit during the current frame.",
    },
    mesh_bits = {
      from = "mesh_bits",
      type = "integer",
      description = "Bitmask of which of the item's meshes are drawn.",
    },
    touch_bits = {
      from = "touch_bits",
      type = "integer",
      writable = false,
      description = "Bitmask of which of the item's meshes Lara is touching.",
    },
    anim_state = {
      from = "current_anim_state",
      type = "integer",
      description = "Current animation state.",
    },
    goal_anim_state = {
      from = "goal_anim_state",
      type = "integer",
      description = "Animation state the item is transitioning towards.",
    },
    -- Deliberately not exposed: box_num, floor, next_item, next_active, gen,
    -- anim_num, frame_num, prev_frame_num, ai_bits, ai_tag, after_death and the
    -- render flags. They are engine internals, not a contract.
  },

  extensions = {
    room = {
      type = "Room",
      description = "The `trx.rooms.Room` containing this item.",
      impl = function(item)
        return trx.rooms[item.room_num]
      end,
    },
    properties = {
      type = "table",
      description = "Typed, object-specific item properties. Writing here overrides the object's "
        .. "default for this item only; reads fall back to the object. Iterable with `pairs()`. "
        .. "See [Objects](../../OBJECTS.md).",
      impl = make_properties,
    },
  },

  methods = {
    activate = {
      description = "Brings the item to life, exactly as tripping a trigger on it would: its control "
        .. "routine starts running, and a creature also gets its AI, without which it would stand "
        .. "there and ignore Lara.\n\n"
        .. "Objects with no control routine cannot be activated, and an item that is already active "
        .. "is left alone.",
    },
    deactivate = {
      description = "Stops the item: its control routine no longer runs, and a creature loses its AI "
        .. "and stands down. The item stays where it is and keeps its hit points, so this is not a "
        .. "way of getting rid of it - use `kill()` for that.\n\n"
        .. "A trigger can still bring it back, and so can `activate()`.",
    },
    trigger = {
      description = "Fires a trigger at the item, exactly as a floor trigger in the level would: "
        .. "sets the code bits, and once they are all set, starts the item running.\n\n"
        .. "This is the one to reach for on anything a level would trigger - a door, a switch, an "
        .. "alarm - because those read their trigger before they act, and merely `activate()`-ing "
        .. "one leaves it running but doing nothing. Pass `type = "
        .. "trx.items.TriggerType.ANTITRIGGER` to take the trigger back instead.",
      params = {
        {
          name = "opts",
          type = "table",
          optional = true,
          description = "`type`: which `items.TriggerType` to fire; a plain `TRIGGER` by default.\n\n"
            .. "`mask`: which of the five code bits to set, `1` to `31`, all of them by default. "
            .. "Pass fewer to act as one of several triggers a puzzle is waiting on.\n\n"
            .. "`timer`: how long it should keep the item going, in seconds. `0`, the default, means "
            .. "until something takes the trigger back. A timer of exactly `1` is a single frame, "
            .. "not a second, matching the level format.\n\n"
            .. "`one_shot`: never let it fire again.",
        },
      },
      examples = {
        [[trx.items[12]:trigger()]],
        [[trx.items[12]:trigger({ timer = 3, one_shot = true })]],
        [[trx.items[12]:trigger({ type = trx.items.TriggerType.ANTITRIGGER })]],
      },
    },
    on_trigger = {
      params = {
        {
          name = "callback",
          type = "function",
          params = {
            {
              name = "item",
              type = "Item",
              description = "This item.",
            },
            {
              name = "trigger",
              type = "table",
              description = "What the trigger carried: `type`, `mask`, `timer` and `one_shot`. See "
                .. "`trx.events.on_trigger`.",
            },
          },
        },
      },
      returns = ITEM_LISTENER_ID,
      description = "Happens every time a trigger is aimed at this item, of any kind. "
        .. "`trx.events.on_trigger`, narrowed to this item.",
      examples = {
        [[trx.items[12]:on_trigger(function(item, trigger)
  trx.log.info("triggered with mask " .. trigger.mask)
end)]],
      },
      impl = item_hook("on_trigger"),
    },
    kill = {
      description = "Removes the item from the game. Any other handle to it becomes stale.",
    },
    is_valid = {
      returns = { type = "boolean" },
      description = "Whether the handle still refers to a live item. Reading or writing a field on a "
        .. "stale handle raises an error rather than silently operating on an unrelated item, so "
        .. "check this for a handle held across time.",
      examples = {
        [[local wolf = trx.items.query:of_object(trx.catalog.objects.wolf):first()
trx.events.after_control(function()
  if wolf:is_valid() and wolf.hit_points <= 0 then
    trx.log.info("the wolf is down")
  end
end)]],
      },
    },
    die = {
      params = {
        {
          name = "explode",
          type = "boolean",
          optional = true,
          default = false,
          description = "Whether to burst the meshes as it dies.",
        },
      },
      description = "Runs the object's creature death handling: the corpse stays, and `explode` "
        .. "bursts its meshes as a rocket or grenade would. For creatures; `kill()` simply removes "
        .. "any item from the game.",
    },
    shatter = {
      params = {
        {
          name = "damage",
          type = "integer",
          optional = true,
          default = 0,
          description = "Splash damage dealt to nearby items.",
        },
      },
      description = "Bursts the item's meshes into flying debris, the visual `die(true)` produces, "
        .. "on its own. It does not kill or remove the item.",
    },
    distance_to = {
      params = {
        { name = "pos", type = "vec3", description = "World position." },
      },
      returns = { type = "integer" },
      description = "Distance from this item to a world position.",
    },
    get_property = {
      params = { { name = "name", type = "string" } },
      returns = { type = "any", nullable = true },
      description = "Reads an object property, falling back to the object's default. "
        .. "Prefer `item.properties.<name>`.",
    },
    set_property = {
      params = {
        { name = "name", type = "string" },
        { name = "value", type = "any" },
      },
      description = "Overrides an object property for this item. Prefer `item.properties.<name> = ...`.",
    },
    get_property_names = {
      returns = { type = "table" },
      description = "Names of every property this item's object declares.",
    },
  },
})

api.define("items.get", {
  description = "Retrieves an item by index or by name. Items count from zero, matching the "
    .. "item numbers level editors show.",
  params = {
    {
      name = "key",
      type = "any",
      description = "0-based index, or the item's unique name.",
    },
  },
  returns = { type = "Item", nullable = true },
  examples = {
    [==[local item = trx.items[0]
item.name = "lara"
local lara = trx.items["lara"]]==],
  },
  impl = raw.get,
})

api.define("items.spawn", {
  description = "Creates a new item of the given object type at the given position.",
  params = {
    {
      name = "object_id",
      type = "integer",
      enum = "catalog.objects",
      description = "Object type to spawn.",
    },
    {
      name = "pos",
      type = "vec3",
      description = "World position. Must lie inside the level.",
    },
    {
      name = "angle_y",
      type = "integer",
      optional = true,
      default = 0,
      description = "Facing angle.",
    },
    {
      name = "opts",
      type = "table",
      optional = true,
      description = "`activate`: bring the item to life, enabling AI for creatures.",
    },
  },
  returns = {
    type = "Item",
    nullable = true,
    description = "`nil` if the item pool is exhausted.",
  },
  examples = {
    [[local wolf = trx.items.spawn(
  trx.catalog.objects.wolf, trx.lara.item.pos, 0, { activate = true })]],
  },
  impl = raw.spawn,
})

api.define("items.count", {
  description = "Returns the total number of allocated items. Same as `#trx.items`.",
  returns = { type = "integer" },
  impl = raw.count,
})

-- Every item the level holds, each by its number.
local function enumerate()
  local out = {}
  for i = 0, raw.count() - 1 do
    local item = raw.get(i)
    if item ~= nil then
      out[#out + 1] = { i, item }
    end
  end
  return out
end

-- An object named by string resolves through the object query, so `of_object`
-- takes a name the same way a player would. An id passes straight through.
-- trx.objects is reached at call time, not required: it is up long before a
-- console line runs, and leaving it out keeps a script that only queries items
-- from dragging the object surface in behind it.
local function resolve_object(key)
  if type(key) == "number" then
    return key
  end
  return trx.objects.query:by_name(key):ids()[1]
end

local filters = {
  active = function()
    return function(_i, item)
      return item.is_active
    end
  end,
  of_object = function(key)
    local object_id = resolve_object(key)
    return function(_i, item)
      return object_id ~= nil and item.object_id == object_id
    end
  end,
  in_room = function(room_num)
    return function(_i, item)
      return item.room_num == room_num
    end
  end,
}

local item_query = trx.query.new({
  enumerate = enumerate,
  id_of = function(i)
    return i
  end,
  filters = filters,
})

api.property("items.query", {
  type = "table",
  description = "The identity query over every item in the level. Narrow it and read it - see "
    .. "[Query](../../QUERY.md).\n\n"
    .. "Its own narrowings, beyond the operators: `active`, `of_object` (by object id or name) and "
    .. "`in_room`.\n\n"
    .. 'Example: `trx.items.query:of_object("wolf"):active():matches()`.',
  get = function()
    return item_query
  end,
})

api.container("items", {
  description = "Indexing the module reaches an item, and `#trx.items` is how many the level has. "
    .. "Items count from zero, matching the item numbers level editors show. `pairs()` walks them "
    .. "in order, keyed by that number.",
  key = {
    type = "any",
    description = "0-based index, or the item's unique name.",
  },
  value = { type = "Item", nullable = true },
  examples = {
    [[for num, item in pairs(trx.items) do
  trx.log.info(item.object_id)
end]],
  },
  get = raw.get,
  count = raw.count,
})
