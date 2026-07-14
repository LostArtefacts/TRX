local raw = trxc.objects
local api = trx.api

api.module("objects", {
  order = 15,
  title = "Object",
  description = "Module for the object definitions a level is built from.\n\n"
    .. "An object is the pattern every item of that type is cut from: a wolf's radius, not this "
    .. "wolf's. Per-item state lives on the item - see `trx.items`.",
})

-- Object handles are bare userdata. Their metatable is populated by the api.type
-- declaration below, and by nothing else: a member of the C OBJECT struct that is
-- not named here is not reachable from a script at all.

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
    properties = {
      type = "table",
      description = "The object's own typed properties, which every item of the type inherits. "
        .. "Writing here changes the default for all of them; write to `item.properties` to change "
        .. "one item only. Iterable with `pairs()`. See [Objects](../../OBJECTS.md).",
      impl = make_properties,
    },
  },

  methods = {
    get_property = {
      params = { { name = "name", type = "string" } },
      returns = { type = "any", nullable = true },
      description = "Reads one of the object's properties. Prefer `object.properties.<name>`.",
    },
    set_property = {
      params = { { name = "name", type = "string" }, { name = "value", type = "any" } },
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
  returns = { type = "Object", nullable = true, description = "`nil` if no such object exists." },
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

api.define("objects.swap_mesh", {
  description = "Swaps meshes between two objects. With no mesh numbers, swaps all of them; with "
    .. "both, swaps just those two. One without the other raises.",
  params = {
    { name = "object_id1", type = "integer", enum = "catalog.objects" },
    { name = "object_id2", type = "integer", enum = "catalog.objects" },
    { name = "mesh_num1", type = "integer", optional = true, description = "Mesh of the first." },
    { name = "mesh_num2", type = "integer", optional = true, description = "Mesh of the second." },
  },
  impl = raw.swap_mesh,
})

api.container("objects", {
  description = "Indexing the module reaches an object definition, so `trx.objects.wolf` is the wolf.",
  key = { type = "any", description = "Object id, or its catalog name." },
  value = { type = "Object", nullable = true },
  examples = { [[trx.objects.wolf.properties.max_hit_points = 30]] },
  get = get,
})
