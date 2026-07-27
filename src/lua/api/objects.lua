local raw = trxc.objects
local api = trx.api

require("trx.strings")
require("trx.catalog")
require("trx.query")

api.module("objects", {
  order = 15,
  title = "Object",
  description = "Module for the object definitions a level is built from.\n\n"
    .. "An object is the pattern every item of that type is cut from: a wolf's radius, not this "
    .. "wolf's. Per-item state lives on the item - see `trx.items`.",
})

-- Object handles are bare userdata. Their metatable is populated by the
-- api.type declaration below, and by nothing else: a member of the C OBJECT
-- struct that is not named here is not reachable from a script at all.

local function make_properties(object)
  return setmetatable({}, {
    __index = function(_, key)
      if type(key) ~= "string" then
        return nil
      end
      return object:get_property(key)
    end,
    __newindex = function(_, key, value)
      object:set_property(key, value)
    end,
    __pairs = function()
      local names = object:get_property_names()
      local i = 0
      return function()
        i = i + 1
        local name = names[i]
        if name == nil then
          return nil
        end
        return name, object:get_property(name)
      end
    end,
  })
end

api.type("objects.Object", {
  backing = "OBJECT",
  description = "An object definition.",

  fields = {
    loaded = {
      from = "loaded",
      type = "boolean",
      writable = false,
      description = "Whether the current level has this object at all. An object it never loaded "
        .. "still has a definition; this is how a script tells.",
    },
    is_intelligent = {
      from = "intelligent",
      type = "boolean",
      writable = false,
      description = "Whether the object thinks - a creature rather than a door.",
    },
    mesh_count = {
      from = "mesh_count",
      type = "integer",
      writable = false,
      description = "How many meshes the object is built from.",
    },
    anim_count = {
      from = "anim_count",
      type = "integer",
      writable = false,
      description = "How many animations it has.",
    },
    radius = {
      from = "radius",
      type = "integer",
      description = "Collision radius.",
    },
    shadow_size = {
      from = "shadow_size",
      type = "integer",
      description = "Size of the blob shadow drawn under it, and 0 for none.",
    },
    smartness = {
      from = "smartness",
      type = "integer",
      description = "How readily a creature of this type finds its way to Lara.",
    },
    pivot_length = {
      from = "pivot_length",
      type = "integer",
      description = "How far in front of itself the object turns about.",
    },
    semi_transparent = {
      from = "semi_transparent",
      type = "boolean",
      description = "Whether the object is drawn see-through.",
    },
  },

  extensions = {
    names = {
      type = "table",
      description = "Every name the object answers to, in the player's language. An object has "
        .. "more than one: a large medipack is also a `medipack` and a `big medi`.",
      impl = function(object)
        return object:get_names()
      end,
    },
    default_names = {
      type = "table",
      description = "The compile-time English names. A lookup falls back on these when the "
        .. "player's language has no name to match, which is the case before a language file is "
        .. "loaded at all.",
      impl = function(object)
        return object:get_default_names()
      end,
    },
    properties = {
      type = "table",
      description = "The object's own typed properties, which every item of the type inherits. "
        .. "Writing here changes the default for all of them; write to `item.properties` to change "
        .. "one item only. Iterable with `pairs()`. See [Objects](../../OBJECTS.md).",
      impl = make_properties,
    },
  },

  methods = {
    get_names = {
      returns = { type = "table" },
      description = "Every name the object answers to, in the player's language. Prefer "
        .. "`object.names`.",
    },
    get_default_names = {
      returns = { type = "table" },
      description = "The compile-time English names, which a lookup falls back on before a "
        .. "language file is loaded. Prefer `object.default_names`.",
    },
    get_property = {
      params = { { name = "name", type = "string" } },
      returns = { type = "any", nullable = true },
      description = "Reads one of the object's properties. Prefer `object.properties.<name>`.",
    },
    set_property = {
      params = {
        { name = "name", type = "string" },
        { name = "value", type = "any" },
      },
      description = "Writes one of the object's properties. Prefer `object.properties.<name> = ...`.",
    },
    get_property_names = {
      returns = { type = "table" },
      description = "Names of every property this object declares.",
    },
  },
})

local get = api.define("objects.get", {
  description = "Retrieves an object definition by id or by name.",
  params = {
    {
      name = "key",
      type = "any",
      enum = "catalog.objects",
      description = 'Object id, or its catalog name: `trx.objects["wolf"]`.',
    },
  },
  returns = {
    type = "Object",
    nullable = true,
    description = "`nil` if no such object exists.",
  },
  examples = {
    [[local wolf = trx.objects.wolf
wolf.properties.max_hit_points = 30]],
  },
  impl = function(key)
    if type(key) == "number" then
      return raw.get(key)
    end
    if type(key) == "string" then
      local object_id = trx.catalog.objects[key]
      return object_id ~= nil and raw.get(object_id) or nil
    end
    return nil
  end,
})

-- Every object the engine knows, each once. The catalog answers to a name in
-- any case, so pairs() reaches an id under several keys; a seen set keeps a
-- candidate from being weighed more than once.
local function enumerate()
  local out, seen = {}, {}
  for _, id in pairs(trx.catalog.objects) do
    if not seen[id] then
      seen[id] = true
      local object = raw.get(id)
      if object ~= nil then
        out[#out + 1] = { id, object }
      end
    end
  end
  return out
end

-- The families an object belongs to. Membership is the engine's; the key is the
-- name a script narrows by and the name a player types, and the value is what
-- the engine calls the same family. Every one of them is searchable, so a
-- by_name of "pickup" matches every pickup and a command reaches a family by
-- name. Which families a command offers is then its own query's doing: /kill
-- narrows to what fights, so "pickup" is not among the names it answers to.
local FAMILIES = {
  creature = "creature",
  -- One of Lara's own: the butler, and Lara herself.
  loyal = "loyal",
  pickup = "pickup",
  switch = "switch",
  receptacle = "receptacle",
  door = "door",
  inventory_item = "inventory",
  null_object = "null",
  animation = "anim",
}

-- The narrowings a query offers: a state an object is in, and a family it
-- belongs to.
local filters = {
  loaded = function()
    return function(_id, object)
      return object.loaded
    end
  end,
  -- A thing that exists in the world at all, rather than an inventory icon, an
  -- animation, or a null placeholder.
  spawnable = function()
    return function(id, object)
      return object.loaded
        and not raw.is_type(id, "null")
        and not raw.is_type(id, "anim")
        and not raw.is_type(id, "inventory")
    end
  end,
  -- A creature that fights Lara rather than for her. The one family that is not
  -- the engine's own, so it is spelled out here.
  enemy = {
    searchable = true,
    test = function()
      return function(id)
        return raw.is_type(id, "creature") and not raw.is_type(id, "loyal")
      end
    end,
  },
}

for name, kind in pairs(FAMILIES) do
  filters[name] = {
    searchable = true,
    test = function()
      return function(id)
        return raw.is_type(id, kind)
      end
    end,
  }
end

local object_query = trx.query.new({
  enumerate = enumerate,
  id_of = function(id)
    return id
  end,
  filters = filters,
  names_of = function(object)
    return object.names
  end,
  default_names_of = function(object)
    return object.default_names
  end,
})

api.property("objects.query", {
  type = "table",
  description = [[
The identity query over every object definition. Narrow it and read it - see
[Query](../../QUERY.md).

Its own narrowings, beyond the shared `by_name` and the operators: the states
`loaded` and `spawnable`, and the families `creature`, `enemy`, `loyal`,
`pickup`, `switch`, `receptacle`, `door`, `inventory_item`, `null_object` and
`animation`. Every family is searchable: a `by_name` of the family's own name
matches every member, and `names` offers it for completion. Which families a
query answers to follows from what it kept, so one narrowed to what fights
offers no `pickup`.

Example: `trx.objects.query:spawnable():by_name("wolf"):ids()`.]],
  get = function()
    return object_query
  end,
})

api.define("objects.swap_mesh", {
  description = "Swaps meshes between two objects. With no mesh numbers, swaps all of them; with "
    .. "both, swaps just those two. One without the other raises.",
  params = {
    { name = "object_id1", type = "integer", enum = "catalog.objects" },
    { name = "object_id2", type = "integer", enum = "catalog.objects" },
    {
      name = "mesh_num1",
      type = "integer",
      optional = true,
      description = "Mesh of the first.",
    },
    {
      name = "mesh_num2",
      type = "integer",
      optional = true,
      description = "Mesh of the second.",
    },
  },
  impl = raw.swap_mesh,
})

api.container("objects", {
  description = "Indexing the module reaches an object definition, so `trx.objects.wolf` is the wolf. "
    .. "Keyed by object id or catalog name, not by position.",
  key = { type = "any", description = "Object id, or its catalog name." },
  value = { type = "Object", nullable = true },
  examples = { [[trx.objects.wolf.properties.max_hit_points = 30]] },
  get = get,
})
