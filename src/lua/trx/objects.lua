local raw = trxc.objects
local h = require("trx.internal.helpers")

require("trx.strings")
require("trx.catalog")
require("trx.query")

---@class trx
---@field objects trx.objects

---Module for the object definitions a level is built from.
---
---An object is the pattern every item of that type is cut from: a wolf's
---radius, not this wolf's. Per-item state lives on the item - see `trx.items`.
---@trx.module 8 Object
---@class (exact) trx.objects: table<trx.catalog.objects|string, trx.objects.Object?>
---@trx.readonly query
---@field query trx.objects.ObjectQuery The identity query over every object definition. Narrow it and read it.
local M = h.module("objects")

---The mesh's number within the object it belongs to.
---@trx.base 0
---@alias trx.objects.MeshNum integer

-- Object handles are bare userdata. Their metatable is populated by the
-- h.handle declaration below, and by nothing else: a member of the C OBJECT
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

---An object definition.
---@class (exact) trx.objects.Object
---@trx.readonly anim_count, is_intelligent, loaded, mesh_count
---@field loaded boolean Whether the current level has this object at all. An object it never loaded still has a definition; this is how a script tells.
---@field is_intelligent boolean Whether the object thinks - a creature rather than a door.
---@field mesh_count integer How many meshes the object is built from.
---@field anim_count integer How many animations it has.
---@field radius integer Collision radius.
---@field shadow_size integer Size of the blob shadow drawn under it, and 0 for none.
---@field smartness integer How readily a creature of this type finds its way to Lara.
---@field pivot_length integer How far in front of itself the object turns about.
---@field semi_transparent boolean Whether the object is drawn see-through.
---@field name string? The name the game shows for the object. It is the first value in `trx.objects.Object.names`, or `nil` where the object has no name.
---@field names table Every name the object answers to, in the player's language. An object has more than one: a large medipack is also a `medipack` and a `big medi`. <!--noref: medipack-->
---@field default_names table The compile-time English names. A lookup tries these when the player's language has no matching name, so an English name still reaches the object in a translated install.
---@field properties table The object's own typed properties, which every item of the type inherits. Writing here changes the default for all of them; write to `trx.items.Item.properties` to change one item only. Iterable with `pairs()`. See [Objects](docs/trx/OBJECTS.md).
local Object = h.handle("objects.Object", "OBJECT", {
  fields = {
    loaded = "loaded",
    is_intelligent = "intelligent",
    mesh_count = "mesh_count",
    anim_count = "anim_count",
    radius = "radius",
    shadow_size = "shadow_size",
    smartness = "smartness",
    pivot_length = "pivot_length",
    semi_transparent = "semi_transparent",
  },
  writable = {
    "radius",
    "shadow_size",
    "smartness",
    "pivot_length",
    "semi_transparent",
  },
  extensions = {
    name = function(object)
      return object:get_names()[1]
    end,
    names = function(object)
      return object:get_names()
    end,
    default_names = function(object)
      return object:get_default_names()
    end,
    properties = make_properties,
  },
})

---Every name the object answers to, in the player's language. Prefer
---`trx.objects.Object.names`.
---@return string[]
function Object:get_names() end

---The compile-time English names. A lookup tries these when the player's
---language has no matching name. Prefer `trx.objects.Object.default_names`.
---@return string[]
function Object:get_default_names() end

---Puts the object in a family, so a query narrowed to that family finds it. A
---family a script mints is reached the same way as one the game ships.
---
---```lua
---local family = trx.catalog.mint(trx.catalog.Context.FAMILIES, "mymod:explosive")
---trx.objects.barrel:add_family("mymod:explosive")
---for _, id in ipairs(trx.objects.query:family("mymod:explosive"):ids()) do ... end
---```
---@param family string Which family, by the name it answers to.
function Object:add_family(family) end

---Links this object to another object. Use the relation name that the game
---uses, such as `gun_to_ammo`. <!--noref: gun_to_ammo--> This lets the game
---use relations between objects created by a script.
---
---```lua
---trx.objects[gun]:link("gun_to_ammo", ammo)
---```
---@param link string The relation to add, such as `gun_to_ammo`. <!--noref: gun_to_ammo-->
---@param other trx.catalog.objects The object to link.
function Object:link(link, other) end

---Takes the object out of a family.
---@param family string Which family, by the name it answers to.
function Object:remove_family(family) end

---Reads one of the object's properties. Prefer `object.properties.<name>`.
---@param name string Which property, as the object declares it.
---@return any? # The value, of the type the property is declared with.
function Object:get_property(name) end

---Writes one of the object's properties. Prefer
---`object.properties.<name> = ...`.
---@param name string Which property, as the object declares it.
---@param value any What to write, of the type the property is declared with.
function Object:set_property(name, value) end

---Names of every property this object declares.
---@return string[]
function Object:get_property_names() end

---Retrieves an object definition by id or by name.
---
---```lua
---local wolf = trx.objects.wolf
---wolf.properties.max_hit_points = 30
---```
---@param key trx.catalog.objects Object id, or its catalog name: `trx.objects["wolf"]`.
---@return trx.objects.Object? # `nil` if no such object exists.
function M.get(key)
  if type(key) == "number" then
    return raw.get(key)
  end
  if type(key) == "string" then
    local object_id = trx.catalog.objects[key]
    return object_id ~= nil and raw.get(object_id) or nil
  end
  return nil
end

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
  { "creature", "creature" },
  { "boss", "boss" },
  { "loyal", "loyal" },
  { "pickup", "pickup" },
  { "gun", "gun" },
  { "ammo", "ammo" },
  { "supply", "supply" },
  { "tool", "tool" },
  { "key", "key" },
  { "puzzle", "puzzle" },
  { "quest", "quest" },
  { "examine", "examine" },
  { "collectible", "collectible" },
  { "secret", "secret" },
  { "switch", "switch" },
  { "receptacle", "receptacle" },
  { "pushable", "pushable" },
  { "door", "door" },
  { "inventory_item", "inventory" },
  { "null_object", "null" },
  { "animation", "anim" },
}

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

---A `trx.query.Query` over every object the engine knows, with the narrowings
---below on top of the ones every query has. Objects answer to names, so it
---carries the name layer too - see `trx.query.NamedQuery`.
---
---The families do not cover `trx.objects.ObjectQuery:pickup` between them: a
---second state of something Lara already carries, such as a part-full
---waterskin, is in none of them.
---@class (exact) trx.objects.ObjectQuery: trx.query.NamedQuery
local ObjectQuery = h.class("objects.ObjectQuery", {
  extends = "query.NamedQuery",
})

local loaded = trx.query.narrowing(function()
  return function(_id, object)
    return object.loaded
  end
end)

---The level loaded the object, so items of it exist.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:loaded()
  return loaded(self)
end

local spawnable = trx.query.narrowing(function()
  return function(id, object)
    return object.loaded
      and not raw.is_type(id, "null")
      and not raw.is_type(id, "anim")
      and not raw.is_type(id, "inventory")
  end
end)

---The object is a thing in the world at all, rather than an inventory icon, an
---animation, or a null placeholder.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:spawnable()
  return spawnable(self)
end

local by_family = trx.query.narrowing(function(name)
  return function(id)
    return raw.is_type(id, name)
  end
end)

---Narrows to a family by name, which is how a query reaches a family a script
---mints. The families the game ships have a narrowing of their own.
---
---```lua
---trx.objects.query:family("mymod:explosive"):ids()
---```
---@param family string Which family, by the name it answers to.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:family(family)
  return by_family(self, family)
end

-- A creature that fights Lara rather than for her. The one family that is not
-- the engine's own, so it is spelled out here.
local enemy = trx.query.narrowing(enemy_test)

---A creature that fights Lara rather than for her.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:enemy()
  return enemy(self)
end

-- Every family is searchable, so a `by_name` of the family's own name matches
-- every member and a command reaches a family by name. Which families a query
-- answers to follows from what it kept, so one narrowed to what fights offers
-- no `pickup`.
local searchable = { { key = "enemy", pred = enemy_test() } }
local families = {}
for _, entry in ipairs(FAMILIES) do
  local name, kind = entry[1], entry[2]
  local test = family_test(kind)
  families[name] = trx.query.narrowing(test)
  searchable[#searchable + 1] = { key = name, pred = test() }
end

-- The group names are offered for completion in this order.
table.sort(searchable, function(a, b)
  return a.key < b.key
end)

---The object is a creature.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:creature()
  return families.creature(self)
end

---A creature the game treats as a boss, which the enemy health bar can be held
---to.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:boss()
  return families.boss(self)
end

---One of Lara's own: the butler, and Lara herself.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:loyal()
  return families.loyal(self)
end

---Something Lara can pick up.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:pickup()
  return families.pickup(self)
end

---A weapon.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:gun()
  return families.gun(self)
end

---Clips for a weapon.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:ammo()
  return families.ammo(self)
end

---A pickup Lara spends rather than keeps.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:supply()
  return families.supply(self)
end

---A pickup named for itself rather than filling a numbered slot: the crowbar,
---the lasersight, the binoculars, the waterskins, the leadbar.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:tool()
  return families.tool(self)
end

---A key, by the slot it fills.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:key()
  return families.key(self)
end

---A puzzle item, by the slot it fills.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:puzzle()
  return families.puzzle(self)
end

---A quest item, by the slot it fills. This is what carries the scion.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:quest()
  return families.quest(self)
end

---An examine item, by the slot it fills.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:examine()
  return families.examine(self)
end

---A collectible, by the slot it fills.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:collectible()
  return families.collectible(self)
end

---The trinket a secret trigger sits under.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:secret()
  return families.secret(self)
end

---A switch Lara throws.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:switch()
  return families.switch(self)
end

---A slot a puzzle item goes into.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:receptacle()
  return families.receptacle(self)
end

---A block Lara pushes and pulls.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:pushable()
  return families.pushable(self)
end

---A door.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:door()
  return families.door(self)
end

---An icon in the inventory rather than a thing in the world.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:inventory_item()
  return families.inventory_item(self)
end

---A placeholder that is never drawn.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:null_object()
  return families.null_object(self)
end

---An animation an object borrows rather than a thing of its own.
---@return trx.query.Query # The narrowed query.
function ObjectQuery:animation()
  return families.animation(self)
end

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

h.properties(M, "objects", {
  query = {
    get = function()
      return object_query
    end,
  },
})

---Defines setup for an object created by a script. The setup is applied at
---each level load because object records are rebuilt for each level.
---
---`control` runs once each frame for each active item. `initialise` runs when
---an item is created. Both functions receive the item.
---<!--noref: control--><!--noref: initialise-->
---
---```lua
---local blast = trx.catalog.mint(trx.catalog.Context.OBJECTS, "mymod:blast")
---trx.objects.declare(blast, {
---  radius = 128,
---  save_position = true,
---  initialise = function(item) item.hit_points = 60 end,
---  control = function(item) item.pos.y = item.pos.y - 8 end,
---})
---```
---@param object_id trx.catalog.objects The object created with `trx.catalog.mint`.
---@param spec table The object setup: `control`, `initialise`, `radius`, `shadow_size` and `save_position`. <!--noref: control--><!--noref: initialise--><!--noref: radius--><!--noref: shadow_size--><!--noref: save_position-->
---@type fun(object_id: trx.catalog.objects, spec: table)
M.declare = raw.declare

---Copies meshes and animations from another object. Use this when the new
---object has no models in the level, such as a custom projectile that uses the
---rocket model.
---
---```lua
---trx.objects.borrow_content(my_blast, trx.catalog.objects.rocket)
---```
---@param object_id trx.catalog.objects The object that receives the meshes and animations.
---@param source_id trx.catalog.objects The object that gives the meshes and animations.
---@return boolean # `false` if the level has no content for the source object.
---@type fun(object_id: trx.catalog.objects, source_id: trx.catalog.objects): boolean
M.borrow_content = raw.borrow_content

---Swaps meshes between two objects. With no mesh numbers, swaps all of them;
---with both, swaps just those two. One without the other raises.
---@param object_id1 trx.catalog.objects
---@param object_id2 trx.catalog.objects
---@param mesh_num1? trx.objects.MeshNum Mesh of the first.
---@param mesh_num2? trx.objects.MeshNum Mesh of the second.
---@type fun(object_id1: trx.catalog.objects, object_id2: trx.catalog.objects, mesh_num1?: trx.objects.MeshNum, mesh_num2?: trx.objects.MeshNum)
M.swap_mesh = raw.swap_mesh

---Swaps the sprites of two objects, which is how a pickup looks when 3D
---pickups are turned off. Raises if either object is drawn from meshes rather
---than a sprite.
---@param object_id1 trx.catalog.objects
---@param object_id2 trx.catalog.objects
---@type fun(object_id1: trx.catalog.objects, object_id2: trx.catalog.objects)
M.swap_sprite = raw.swap_sprite

---Indexing the module reaches an object definition, so `trx.objects.wolf` is
---the wolf. Keyed by object id or catalog name, not by position.
---
---```lua
---trx.objects.wolf.properties.max_hit_points = 30
---```
---@type table<trx.catalog.objects|string, trx.objects.Object?>
---@trx.key Object id, or its catalog name.
h.container("objects", { base = 0, by_name = true, get = M.get }, M)
