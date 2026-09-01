-- The object API as a script actually sees it.
--
-- An object is what an item is cut from. trx.objects[id] is a real handle over
-- the C OBJECT struct, so what is reachable is what objects.Object declares.

local h = require("harness")
local test, raises = h.test, h.raises

test("an object is reached by id", function()
  local wolf = trx.objects[fake.WOLF]
  assert(wolf ~= nil, "no wolf")
  assert(wolf.loaded == true)
  assert(wolf.is_intelligent == true, "a wolf thinks")
  assert(wolf.radius == 341)
  assert(wolf.mesh_count == 6)
end)

test("get() is the same thing as indexing", function()
  assert(trx.objects.get(fake.WOLF).radius == trx.objects[fake.WOLF].radius)
end)

test("an object the level never loaded still has a definition", function()
  local obj = trx.objects[fake.UNLOADED]
  assert(obj ~= nil, "an unloaded object must still be reachable")
  assert(obj.loaded == false, "and it must say so")
end)

test("an object that does not exist is nil", function()
  assert(trx.objects[9999] == nil)
  assert(trx.objects.get(9999) == nil)
end)

test("a vase does not think", function()
  assert(trx.objects[fake.VASE].is_intelligent == false)
end)

test("a writable field writes through", function()
  local wolf = trx.objects[fake.WOLF]
  wolf.radius = 500
  assert(
    trx.objects[fake.WOLF].radius == 500,
    "the write did not reach the object"
  )

  wolf.shadow_size = 64
  assert(trx.objects[fake.WOLF].shadow_size == 64)
end)

test("a field declared read-only cannot be written", function()
  raises(function()
    trx.objects[fake.WOLF].loaded = false
  end)
  raises(function()
    trx.objects[fake.WOLF].is_intelligent = false
  end)
end)

test("a member of OBJECT nobody declared is not reachable", function()
  -- The struct has mesh_idx, anim_idx, frame_base and a control function.
  -- None is declared, so none exists as far as a script is concerned.
  local wolf = trx.objects[fake.WOLF]
  assert(wolf.mesh_idx == nil)
  assert(wolf.anim_idx == nil)
  assert(wolf.control_func == nil)
end)

test(
  "properties are the object's own, and every item inherits them",
  function()
    local wolf = trx.objects[fake.WOLF]
    assert(wolf.properties.max_hit_points == 20, "the object's default")

    wolf.properties.max_hit_points = 30
    assert(
      trx.objects[fake.WOLF].properties.max_hit_points == 30,
      "the default did not move"
    )
  end
)

test("properties iterate", function()
  local seen = {}
  for name, value in pairs(trx.objects[fake.WOLF].properties) do
    seen[name] = value
  end
  assert(seen.max_hit_points == 20, "pairs() did not yield the property")
end)

test("a property nobody declared reads nil and cannot be written", function()
  local wolf = trx.objects[fake.WOLF]
  assert(wolf.properties.nonsense == nil)
  raises(function()
    wolf.properties.nonsense = 1
  end)
end)

test("an object knows the names it answers to", function()
  -- More than one, and not all of them sharing a word: a vase is also a large
  -- vase and a big urn, the way a large medipack is also a big medipack.
  local names = trx.objects[fake.VASE].default_names
  assert(#names == 3, "the vase should answer to three names")
  assert(names[1] == "vase")
  assert(names[2] == "large vase")
  assert(names[3] == "big urn")
end)

test("by_name matches the way a player types it", function()
  local ids = trx.objects.query:by_name("wolf"):ids()
  assert(#ids == 1, "the wolf did not match")
  assert(ids[1] == fake.WOLF)
end)

test("by_name matches a name the object also answers to", function()
  local ids = trx.objects.query:by_name("large vase"):ids()
  assert(#ids >= 1, "the vase did not match its second name")
  assert(ids[1] == fake.VASE)
end)

test("a name nobody has matches nothing", function()
  assert(#trx.objects.query:by_name("wombat"):ids() == 0)
end)

test("an id comes back once, however many of its names matched", function()
  -- The vase answers to two names and is a pickup besides, so it is three
  -- candidates. It is still one object.
  local ids = trx.objects.query:by_name("vase"):ids()
  local seen = 0
  for _, id in ipairs(ids) do
    if id == fake.VASE then
      seen = seen + 1
    end
  end
  assert(seen == 1, "the vase came back " .. seen .. " times")
end)

local function id_set(ids)
  local set = {}
  for _, id in ipairs(ids) do
    set[id] = true
  end
  return set
end

test("spawnable keeps the things that exist in the world", function()
  -- The wolf, the pickups and the sprite object are loaded; nothing else in the
  -- fake is. Every other id the catalog names reads as present but unloaded.
  local set = id_set(trx.objects.query:spawnable():ids())
  assert(
    set[fake.WOLF] and set[fake.VASE] and set[fake.KEY] and set[fake.SPRITE]
  )
  assert(not set[fake.UNLOADED], "an unloaded object is not spawnable")
end)

test("a family method narrows to that family", function()
  local ids = trx.objects.query:pickup():ids()
  local set = id_set(ids)
  assert(set[fake.VASE] and set[fake.KEY])
  assert(not set[fake.WOLF], "the wolf is not a pickup")
  assert(trx.objects.query:pickup():count() == #ids)
end)

test("creature and loyal narrow to what fights", function()
  local ids = trx.objects.query:creature():ids()
  assert(#ids == 1 and ids[1] == fake.WOLF)
  -- The fake level has no allies, so every creature in it is an enemy.
  assert(trx.objects.query:loyal():count() == 0)
  assert(trx.objects.query:enemy():ids()[1] == fake.WOLF)
end)

test("the families a level's fixtures belong to", function()
  local q = trx.objects.query
  assert(q:switch():ids()[1] == fake.SWITCH)
  assert(q:receptacle():ids()[1] == fake.RECEPTACLE)
  assert(q:door():ids()[1] == fake.DOOR)
  assert(q:door():count() == 1)
end)

test("where narrows by a test of the caller's own", function()
  local ids = trx.objects.query
    :loaded()
    :where(function(id, object)
      return object.is_intelligent
    end)
    :ids()
  assert(#ids == 1 and ids[1] == fake.WOLF)
end)

test("~ excludes a family", function()
  -- Loaded, and not a pickup: the wolf and the sprite object, neither pickup.
  local q = trx.objects.query
  local set = id_set((q:loaded() & ~q:pickup()):ids())
  assert(set[fake.WOLF] and set[fake.SPRITE])
  assert(not set[fake.VASE] and not set[fake.KEY])
end)

test("| unions two families", function()
  local q = trx.objects.query
  -- The fake has no null objects, so the union is just the pickups.
  local ids = (q:pickup() | q:null_object()):ids()
  assert(#ids == q:pickup():count())
  local set = id_set(ids)
  assert(set[fake.VASE] and set[fake.KEY])
end)

test("a chain and the same narrowings by & agree", function()
  local q = trx.objects.query
  local chained = id_set(q:spawnable():pickup():ids())
  local anded = id_set((q:spawnable() & q:pickup()):ids())
  assert(chained[fake.VASE] and chained[fake.KEY])
  assert(anded[fake.VASE] and anded[fake.KEY])
end)

test("matches() hands back handles, first() one or nil", function()
  local pickup = trx.objects.query:pickup():first()
  assert(pickup ~= nil and pickup.loaded == true)
  assert(
    #trx.objects.query:pickup():matches() == trx.objects.query:pickup():count()
  )
  assert(trx.objects.query:null_object():first() == nil, "no null objects")
end)

test("narrowing a query does not change the base", function()
  local base = trx.objects.query
  local before = base:count()
  base:pickup():ids()
  assert(base:count() == before, "the base query was mutated")
  assert(base:count() > 1, "the base matches everything")
end)

test("by_name ranks what the rest of the query kept", function()
  -- Spawnable, then matched by name: the vase survives both.
  local ids = trx.objects.query:spawnable():by_name("vase"):ids()
  assert(ids[1] == fake.VASE)
end)

test("a searchable family answers to its own name", function()
  -- "pickup" is a group name: it matches every pickup, not one object.
  local q = trx.objects.query
  local ids = q:by_name("pickup"):ids()
  assert(#ids == q:pickup():count(), "every pickup matches the group name")
  local set = id_set(ids)
  assert(set[fake.VASE] and set[fake.KEY])
  assert(not set[fake.WOLF], "the wolf is not a pickup")
end)

test("every family answers to its own name", function()
  local q = trx.objects.query
  assert(q:by_name("creature"):ids()[1] == fake.WOLF)
  assert(q:by_name("enemy"):ids()[1] == fake.WOLF)
  assert(q:by_name("switch"):ids()[1] == fake.SWITCH)
  assert(q:by_name("receptacle"):ids()[1] == fake.RECEPTACLE)
  assert(q:by_name("door"):ids()[1] == fake.DOOR)
end)

test(
  "names() lists what the matches answer to, group names included",
  function()
    local names = {}
    for _, name in ipairs(trx.objects.query:spawnable():names()) do
      names[name] = true
    end
    assert(names["wolf"], "an own name")
    assert(names["vase"] and names["large vase"], "every own name")
    assert(names["key"])
    assert(names["pickup"], "the group name, because a pickup is present")
  end
)

test("names() offers the group names first", function()
  -- A completer takes the list in order, so a group name must not sit behind
  -- the object names it ties with on score.
  local names = trx.objects.query:spawnable():names()
  local groups = {}
  for _, name in ipairs({
    "creature",
    "enemy",
    "loyal",
    "pickup",
    "gun",
    "ammo",
    "supply",
    "tool",
    "key",
    "puzzle",
    "quest",
    "examine",
    "collectible",
    "secret",
    "switch",
    "receptacle",
    "door",
    "inventory_item",
    "null_object",
    "animation",
  }) do
    groups[name] = true
  end
  local seen_own = false
  for _, name in ipairs(names) do
    if groups[name] then
      assert(not seen_own, name .. " came after an object name")
    else
      seen_own = true
    end
  end
  assert(seen_own, "the object names are missing")
end)

test("a query offers only the group names it kept", function()
  local names = {}
  for _, name in ipairs(trx.objects.query:creature():names()) do
    names[name] = true
  end
  assert(names["creature"] and names["enemy"])
  assert(not names["pickup"], "no pickup survived, so it is not a name")
end)

test("best() is one for a name, the whole group for a group name", function()
  local q = trx.objects.query
  -- A name only one object answers to yields a single best.
  local one = q:by_name("wolf"):best()
  assert(#one == 1 and one[1] == fake.WOLF)

  -- A group name: every member ties for the top score.
  local group = id_set(q:by_name("pickup"):best())
  assert(group[fake.VASE] and group[fake.KEY])
end)

test("best() with no by_name is every match", function()
  local ids = trx.objects.query:pickup():ids()
  local best = trx.objects.query:pickup():best()
  assert(#best == #ids, "unranked best is the whole set")
end)

test("declare states what an object of a script's own does", function()
  local id = trx.catalog.mint(trx.catalog.Context.OBJECTS, "mymod:blast")
  assert(id ~= nil)
  trx.objects.declare(id, {
    radius = 128,
    save_position = true,
    control = function(_item) end,
  })

  -- An object is declared once, so a second declaration says so rather than
  -- quietly taking the place of the first.
  raises(function()
    trx.objects.declare(id, { control = function(_item) end })
  end, "already declared")
end)

test("a declaration takes functions where it says functions", function()
  local id = trx.catalog.mint(trx.catalog.Context.OBJECTS, "mymod:other")
  raises(function()
    trx.objects.declare(id, { control = 5 })
  end)
end)

test("borrow_content dresses an object from another", function()
  assert(trx.objects.borrow_content(fake.UNLOADED, fake.WOLF))
  assert(fake.calls().borrow_content.count == 1, "the ask did not land")

  assert(
    not trx.objects.borrow_content(fake.WOLF, fake.UNLOADED),
    "there is nothing to take from an object the level left out"
  )
end)

test("swap_mesh reaches the engine", function()
  trx.objects.swap_mesh(fake.WOLF, fake.VASE)
  assert(
    fake.calls().swap_mesh.count == 1,
    "the whole-object swap did not land"
  )

  trx.objects.swap_mesh(fake.WOLF, fake.VASE, 1, 2)
  assert(
    fake.calls().swap_mesh.count == 2,
    "the single-mesh swap did not land"
  )
end)

test("swap_mesh will not reach past an object's meshes", function()
  raises(function()
    trx.objects.swap_mesh(fake.WOLF, fake.VASE, 0, 1000000)
  end)
  raises(function()
    trx.objects.swap_mesh(fake.WOLF, fake.VASE, -1, 0)
  end)
  assert(
    fake.calls().swap_mesh.count == 0,
    "a rejected swap must not reach the engine"
  )
end)

test("swap_mesh takes both mesh numbers or neither", function()
  raises(function()
    trx.objects.swap_mesh(fake.WOLF, fake.VASE, 1)
  end)
end)

test("swap_sprite reaches the engine", function()
  trx.objects.swap_sprite(fake.KEY, fake.SPRITE)
  assert(fake.calls().swap_sprite.count == 1, "the sprite swap did not land")
end)

test("swap_sprite refuses an object drawn from meshes", function()
  raises(function()
    trx.objects.swap_sprite(fake.SPRITE, fake.WOLF)
  end)
  assert(
    fake.calls().swap_sprite.count == 0,
    "a rejected swap must not reach the engine"
  )
end)

test("swap_sprite passes over an object the level did not load", function()
  trx.objects.swap_sprite(fake.SPRITE, fake.UNLOADED)
  assert(fake.calls().swap_sprite.count == 0, "there is nothing to swap")
end)

test("an object id outside the table is refused", function()
  raises(function()
    trx.objects.swap_mesh(999999, fake.VASE)
  end)
end)

test("a script mints a family and puts an object in it", function()
  local name = "mymod:explosive"
  assert(trx.catalog.mint(trx.catalog.Context.FAMILIES, name) ~= nil)
  assert(
    trx.objects.query:family(name):count() == 0,
    "a family starts with nobody in it"
  )

  trx.objects[fake.VASE]:add_family(name)
  local ids = trx.objects.query:family(name):ids()
  assert(#ids == 1 and ids[1] == fake.VASE)

  trx.objects[fake.VASE]:remove_family(name)
  assert(trx.objects.query:family(name):count() == 0)
end)

test("a family no name answers to is refused", function()
  raises(function()
    trx.objects.query:family("mymod:nothing"):ids()
  end)
end)

return h.report()
