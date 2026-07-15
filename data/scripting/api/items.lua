local raw = trxc.items
local api = trx.api

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
    room_num = {
      from = "room_index",
      type = "integer",
      writable = false,
      description = "1-based number of the room containing this item. Set `pos` to move the item between rooms.",
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
      description = "Trigger-related flag bits. Read-only: writing them directly would let a script set `IF_KILLED` without unlinking the item, wedging engine state. Use `kill()` instead.",
    },
    timer = {
      from = "timer",
      type = "integer",
      description = "Trigger-related timer value.",
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
      description = "Adds the item to the active list and starts its control routine. "
        .. "Objects with no control routine cannot be activated.",
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
        [[local wolf = trx.items.first({ object_id = trx.catalog.objects.wolf })
trx.events.after_control(function()
  if wolf:is_valid() and wolf.hit_points <= 0 then
    trx.log.info("the wolf is down")
  end
end)]],
      },
    },
    explode = {
      description = "Runs the object's death handling with an explosion, as a rocket or a grenade "
        .. "would. Unlike `kill()`, which simply removes the item from the game.",
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

local FIND_KEYS = { object_id = true, room_num = true }

local function validate_query(query, fn_name)
  if type(query) ~= "table" then
    error("trx.items." .. fn_name .. " query must be a table", 3)
  end
  for key, _ in pairs(query) do
    if not FIND_KEYS[key] then
      trx.log.warn(
        "trx.items."
          .. fn_name
          .. ": unknown property '"
          .. tostring(key)
          .. "'"
      )
    end
  end
end

local function matches(item, query)
  for key, _ in pairs(FIND_KEYS) do
    if query[key] ~= nil and item[key] ~= query[key] then
      return false
    end
  end
  return true
end

local function search(query, first_only)
  local found = {}
  for i = 1, raw.count() do
    local item = raw.get(i)
    if item ~= nil and matches(item, query) then
      if first_only then
        return item
      end
      found[#found + 1] = item
    end
  end
  if first_only then
    return nil
  end
  return found
end

api.define("items.get", {
  description = "Retrieves an item by 1-based index or by name.",
  params = {
    {
      name = "key",
      type = "any",
      description = "1-based index, or the item's unique name.",
    },
  },
  returns = { type = "Item", nullable = true },
  examples = {
    [==[local item = trx.items[1]
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

api.define("items.find", {
  description = "Finds all items matching the query.",
  params = {
    {
      name = "query",
      type = "table",
      optional = true,
      description = "Supported keys: `object_id`, `room_num`. Unknown keys are ignored and logged. "
        .. "Omit it for no matches.",
    },
  },
  returns = { type = "table", description = "List of `Item`." },
  examples = {
    [[local wolves = trx.items.find({ object_id = trx.catalog.objects.wolf })]],
  },
  impl = function(query)
    if query == nil then
      return {}
    end
    validate_query(query, "find")
    return search(query, false)
  end,
})

api.define("items.first", {
  description = "Finds the first item matching the query.",
  params = {
    {
      name = "query",
      type = "table",
      optional = true,
      description = "Supported keys: `object_id`, `room_num`. Omit it for no match.",
    },
  },
  returns = { type = "Item", nullable = true },
  examples = {
    [[local natla = trx.items.first({ object_id = trx.catalog.objects.natla })]],
  },
  impl = function(query)
    if query == nil then
      return nil
    end
    validate_query(query, "first")
    return search(query, true)
  end,
})

api.container("items", {
  description = "Indexing the module reaches an item, and `#trx.items` is how many the level has.",
  key = {
    type = "any",
    description = "1-based index, or the item's unique name.",
  },
  value = { type = "Item", nullable = true },
  examples = {
    [[for i = 1, #trx.items do
  trx.log.info(trx.items[i].object_id)
end]],
  },
  get = raw.get,
  count = raw.count,
})
