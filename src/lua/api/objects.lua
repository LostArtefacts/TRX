local raw = trxc.objects
local api = trx.api

require("trx.strings")
require("trx.catalog")
require("trx.query")

api.module("objects", {
  order = 7,
  title = "Object",
  description = "Module for the object definitions a level is built from.\n\n"
    .. "An object is the pattern every item of that type is cut from: a wolf's radius, not this "
    .. "wolf's. Per-item state lives on the item - see `trx.items`.",
})

api.number("objects.MeshNum", {
  base = 0,
  description = "The mesh's number within the object it belongs to.",
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

local Object = api.type("objects.Object", {
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
    name = {
      type = "string",
      nullable = true,
      description = "The name the game shows for the object. It is the first value in "
        .. "`trx.objects.Object.names`, or `nil` where the object has no name.",
      impl = function(object)
        return object:get_names()[1]
      end,
    },
    names = {
      type = "table",
      description = "Every name the object answers to, in the player's language. An object has "
        .. "more than one: a large medipack is also a `medipack` and a `big medi`. <!--noref: medipack-->",
      impl = function(object)
        return object:get_names()
      end,
    },
    default_names = {
      type = "table",
      description = "The compile-time English names. A lookup tries these when the "
        .. "player's language has no matching name, so an English name still reaches the object "
        .. "in a translated install.",
      impl = function(object)
        return object:get_default_names()
      end,
    },
    properties = {
      type = "table",
      description = "The object's own typed properties, which every item of the type inherits. "
        .. "Writing here changes the default for all of them; write to `trx.items.Item.properties` to change "
        .. "one item only. Iterable with `pairs()`. See [Objects](docs/trx/OBJECTS.md).",
      impl = make_properties,
    },
  },

  methods = {
    get_names = {
      returns = {
        type = "string",
        list = true,
      },
      description = "Every name the object answers to, in the player's language. Prefer "
        .. "`trx.objects.Object.names`.",
    },
    get_default_names = {
      returns = {
        type = "string",
        list = true,
      },
      description = "The compile-time English names. A lookup tries these when the "
        .. "player's language has no matching name. Prefer `trx.objects.Object.default_names`.",
    },
    add_family = {
      params = {
        {
          name = "family",
          type = "string",
          description = "Which family, by the name it answers to.",
        },
      },
      description = "Puts the object in a family, so a query narrowed to that family finds "
        .. "it. A family a script mints is reached the same way as one the game ships.",
      examples = {
        [[local family = trx.catalog.mint(trx.catalog.Context.FAMILIES, "mymod:explosive")
trx.objects.barrel:add_family("mymod:explosive")
for _, id in ipairs(trx.objects.query:family("mymod:explosive"):ids()) do ... end]],
      },
    },
    remove_family = {
      params = {
        {
          name = "family",
          type = "string",
          description = "Which family, by the name it answers to.",
        },
      },
      description = "Takes the object out of a family.",
    },
    get_property = {
      params = {
        {
          name = "name",
          type = "string",
          description = "Which property, as the object declares it.",
        },
      },
      returns = {
        type = "any",
        nullable = true,
        description = "The value, of the type the property is declared with.",
      },
      description = "Reads one of the object's properties. Prefer `object.properties.<name>`.",
    },
    set_property = {
      params = {
        {
          name = "name",
          type = "string",
          description = "Which property, as the object declares it.",
        },
        {
          name = "value",
          type = "any",
          description = "What to write, of the type the property is declared with.",
        },
      },
      description = "Writes one of the object's properties. Prefer `object.properties.<name> = ...`.",
    },
    get_property_names = {
      returns = {
        type = "string",
        list = true,
      },
      description = "Names of every property this object declares.",
    },
  },
})

local get = api.define("objects.get", {
  description = "Retrieves an object definition by id or by name.",
  params = {
    {
      name = "key",
      type = "catalog.objects",
      description = 'Object id, or its catalog name: `trx.objects["wolf"]`.',
    },
  },
  returns = {
    type = "objects.Object",
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

-- The families an object belongs to. Membership is the engine's: `kind` is what
-- the engine calls the family, and the name beside it is what a script narrows
-- by and what a player types.
local FAMILIES = {
  { "creature", "creature", "The object is a creature." },
  {
    "boss",
    "boss",
    "A creature the game treats as a boss, which the enemy health bar can be held to.",
  },
  {
    "loyal",
    "loyal",
    "One of Lara's own: the butler, and Lara herself.",
  },
  { "pickup", "pickup", "Something Lara can pick up." },
  { "gun", "gun", "A weapon." },
  { "ammo", "ammo", "Clips for a weapon." },
  { "supply", "supply", "A pickup Lara spends rather than keeps." },
  {
    "tool",
    "tool",
    "A pickup named for itself rather than filling a numbered slot: the crowbar, the lasersight, "
      .. "the binoculars, the waterskins, the leadbar.",
  },
  { "key", "key", "A key, by the slot it fills." },
  { "puzzle", "puzzle", "A puzzle item, by the slot it fills." },
  {
    "quest",
    "quest",
    "A quest item, by the slot it fills. This is what carries the scion.",
  },
  { "examine", "examine", "An examine item, by the slot it fills." },
  { "collectible", "collectible", "A collectible, by the slot it fills." },
  { "secret", "secret", "The trinket a secret trigger sits under." },
  { "switch", "switch", "A switch Lara throws." },
  { "receptacle", "receptacle", "A slot a puzzle item goes into." },
  { "pushable", "pushable", "A block Lara pushes and pulls." },
  { "door", "door", "A door." },
  {
    "inventory_item",
    "inventory",
    "An icon in the inventory rather than a thing in the world.",
  },
  { "null_object", "null", "A placeholder that is never drawn." },
  {
    "animation",
    "anim",
    "An animation an object borrows rather than a thing of its own.",
  },
}

local QUERY = { type = "query.Query", description = "The narrowed query." }

local function family_test(kind)
  return function()
    return function(id)
      return raw.is_type(id, kind)
    end
  end
end

local function enemy_test()
  return function(id)
    return raw.is_type(id, "creature") and not raw.is_type(id, "loyal")
  end
end

local methods = {
  loaded = {
    description = "The level loaded the object, so items of it exist.",
    returns = QUERY,
    impl = trx.query.narrowing(function()
      return function(_id, object)
        return object.loaded
      end
    end),
  },
  spawnable = {
    description = "The object is a thing in the world at all, rather than an inventory icon, an "
      .. "animation, or a null placeholder.",
    returns = QUERY,
    impl = trx.query.narrowing(function()
      return function(id, object)
        return object.loaded
          and not raw.is_type(id, "null")
          and not raw.is_type(id, "anim")
          and not raw.is_type(id, "inventory")
      end
    end),
  },
  family = {
    description = "Narrows to a family by name, which is how a query reaches a family a "
      .. "script mints. The families the game ships have a narrowing of their own.",
    params = {
      {
        name = "family",
        type = "string",
        description = "Which family, by the name it answers to.",
      },
    },
    returns = QUERY,
    examples = {
      [[trx.objects.query:family("mymod:explosive"):ids()]],
    },
    impl = trx.query.narrowing(function(name)
      return function(id)
        return raw.is_type(id, name)
      end
    end),
  },

  -- A creature that fights Lara rather than for her. The one family that is not
  -- the engine's own, so it is spelled out here.
  enemy = {
    description = "A creature that fights Lara rather than for her.",
    returns = QUERY,
    impl = trx.query.narrowing(enemy_test),
  },
}

-- Every family is searchable, so a `by_name` of the family's own name matches
-- every member and a command reaches a family by name. Which families a query
-- answers to follows from what it kept, so one narrowed to what fights offers
-- no `pickup`.
local searchable = { { key = "enemy", pred = enemy_test() } }
for _, family in ipairs(FAMILIES) do
  local name, kind, description = family[1], family[2], family[3]
  local test = family_test(kind)
  methods[name] = {
    description = description,
    returns = QUERY,
    impl = trx.query.narrowing(test),
  }
  searchable[#searchable + 1] = { key = name, pred = test() }
end

-- The group names are offered for completion in this order.
table.sort(searchable, function(a, b)
  return a.key < b.key
end)

local ObjectQuery = api.type("objects.ObjectQuery", {
  extends = "query.NamedQuery",
  description = "A `trx.query.Query` over every object the engine knows, with the narrowings below "
    .. "on top of the ones every query has. Objects answer to names, so it carries the name layer "
    .. "too - see `trx.query.NamedQuery`.\n\n"
    .. "The families do not cover `trx.objects.ObjectQuery:pickup` between them: a second state of something Lara already "
    .. "carries, such as a part-full waterskin, is in none of them.",
  methods = methods,
})

local object_query = trx.query.new({
  enumerate = enumerate,
  id_of = function(id)
    return id
  end,
  searchable = searchable,
  names_of = function(object)
    return object.names
  end,
  default_names_of = function(object)
    return object.default_names
  end,
}, ObjectQuery)

api.property("objects.query", {
  type = "objects.ObjectQuery",
  description = "The identity query over every object definition. Narrow it and read it.",
  get = function()
    return object_query
  end,
})

api.define("objects.declare", {
  description = "Defines setup for an object created by a script. The setup is applied "
    .. "at each level load because object records are rebuilt for each level.\n\n"
    .. "`control` runs once each frame for each active item. `initialise` runs when "
    .. "an item is created. Both functions receive the item. "
    .. "<!--noref: control--><!--noref: initialise-->",
  params = {
    {
      name = "object_id",
      type = "catalog.objects",
      description = "The object created with `trx.catalog.mint`.",
    },
    {
      name = "spec",
      type = "table",
      description = "The object setup: `control`, `initialise`, `radius`, "
        .. "`shadow_size` and `save_position`. "
        .. "<!--noref: control--><!--noref: initialise--><!--noref: radius-->"
        .. "<!--noref: shadow_size--><!--noref: save_position-->",
    },
  },
  examples = {
    [[local blast = trx.catalog.mint(trx.catalog.Context.OBJECTS, "mymod:blast")
trx.objects.declare(blast, {
  radius = 128,
  save_position = true,
  initialise = function(item) item.hit_points = 60 end,
  control = function(item) item.pos.y = item.pos.y - 8 end,
})]],
  },
  impl = raw.declare,
})

api.define("objects.borrow_content", {
  description = "Copies meshes and animations from another object. Use this when the new "
    .. "object has no models in the level, such as a custom projectile that uses the "
    .. "rocket model.",
  params = {
    {
      name = "object_id",
      type = "catalog.objects",
      description = "The object that receives the meshes and animations.",
    },
    {
      name = "source_id",
      type = "catalog.objects",
      description = "The object that gives the meshes and animations.",
    },
  },
  returns = {
    type = "boolean",
    description = "`false` if the level has no content for the source object.",
  },
  examples = {
    [[trx.objects.borrow_content(my_blast, trx.catalog.objects.rocket)]],
  },
  impl = raw.borrow_content,
})

api.define("objects.swap_mesh", {
  description = "Swaps meshes between two objects. With no mesh numbers, swaps all of them; with "
    .. "both, swaps just those two. One without the other raises.",
  params = {
    { name = "object_id1", type = "catalog.objects" },
    { name = "object_id2", type = "catalog.objects" },
    {
      name = "mesh_num1",
      type = "objects.MeshNum",
      optional = true,
      description = "Mesh of the first.",
    },
    {
      name = "mesh_num2",
      type = "objects.MeshNum",
      optional = true,
      description = "Mesh of the second.",
    },
  },
  impl = raw.swap_mesh,
})

api.define("objects.swap_sprite", {
  description = [[
Swaps the sprites of two objects, which is how a pickup looks when 3D pickups
are turned off. Raises if either object is drawn from meshes rather than a
sprite.]],
  params = {
    { name = "object_id1", type = "catalog.objects" },
    { name = "object_id2", type = "catalog.objects" },
  },
  impl = raw.swap_sprite,
})

api.container("objects", {
  description = "Indexing the module reaches an object definition, so `trx.objects.wolf` is the wolf. "
    .. "Keyed by object id or catalog name, not by position.",
  key = {
    type = { "catalog.objects", "string" },
    description = "Object id, or its catalog name.",
  },
  value = { type = "objects.Object", nullable = true },
  examples = { [[trx.objects.wolf.properties.max_hit_points = 30]] },
  get = get,
})
