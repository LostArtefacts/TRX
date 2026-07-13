-- The item API as a script actually sees it.
--
-- Everything under these assertions is real: the ITEM FIELD_DESC table, the
-- struct and enum bridges, trxc.items, and data/scripting/items.lua itself. Only
-- the engine below that is fake (see fake_engine_items.c). So these tests pin
-- the public surface rather than a restatement of it - rename a C member, drop a
-- declaration, mark a field writable that must not be, and the assertion that
-- names it fails.

local h = require("harness")
local test, raises = h.test, h.raises

local WOLF, VASE, UNLOADED = fake.WOLF, fake.VASE, fake.UNLOADED

test("items are indexed from 1, and by name", function()
  assert(#trx.items == 2, "length operator")
  assert(trx.items.count() == 2, "count()")
  assert(trx.items[1] ~= nil and trx.items[2] ~= nil)
  assert(trx.items[99] == nil, "out of range must be nil")

  trx.items[1].name = "wolfie"
  assert(trx.items["wolfie"] ~= nil, "lookup by name")
  assert(trx.items.get("wolfie").name == "wolfie")
  assert(trx.items["nobody"] == nil, "an unknown name must be nil")

  -- A name already in use raises rather than silently rebinding.
  raises(function()
    trx.items[2].name = "wolfie"
  end)
end)

test("fields read and write through to the struct", function()
  local it = trx.items[1]

  it.pos = { x = 512, y = 0, z = 256 }
  assert(it.pos.x == 512 and it.pos.z == 256, "pos did not stick")
  assert(trx.items[1].pos.x == 512, "not written through to the item")

  it.rot = { x = 0, y = 16384, z = 0 }
  assert(it.rot.y == 16384)

  it.timer = 30
  assert(it.timer == 30)

  -- Rooms are 1-based to scripts; the engine counts from 0.
  assert(it.room_num == 1)
end)

-- These encode an engine invariant. Writing them directly would let a script
-- wedge the item, so the declaration withholds them even though the C member is
-- plain and the struct would happily take the write.
test("read-only fields refuse writes", function()
  for _, name in ipairs({
    "flags",
    "status",
    "touch_bits",
    "object_id",
    "room_num",
    "is_active",
    "max_hit_points",
  }) do
    raises(function()
      trx.items[1][name] = 1
    end, "read-only")
  end
end)

test("a field that does not exist raises rather than being created", function()
  raises(function()
    trx.items[1].no_such_field = 1
  end, "unknown")
end)

test("a value the field cannot hold raises instead of truncating", function()
  -- hit_points is 16-bit. Silently truncating is what the old proxy did.
  raises(function()
    trx.items[1].hit_points = 99999
  end)

  trx.items[1].hit_points = 5
  assert(trx.items[1].hit_points == 5)
end)

test("pickup_mode's enum lives with the items, not in a module of its own", function()
  -- It types an item property, so it belongs here. trx.pickup is gone.
  assert(trx.items.PickupMode.NORMAL == 0)
  assert(trx.items.PickupMode.PLINTH_LOW == 1)
  assert(trx.items.PickupMode.PLINTH_HIGH == 2)
  assert(trx.pickup == nil, "trx.pickup must not exist")
end)

test("status matches the enum, and activate() moves it", function()
  assert(trx.items.Status.INACTIVE == 0)
  assert(trx.items.Status.ACTIVE == 1)
  assert(trx.items.Status.DEACTIVATED == 2)
  assert(trx.items.Status.INVISIBLE == 3)

  local it = trx.items[1]
  assert(it.status == trx.items.Status.INACTIVE)
  assert(it.is_active == false)

  it:activate()
  assert(it.is_active == true, "activate() did not take")
end)

-- The generation counter earning its keep: an index alone would silently rebind
-- to whatever item recycled the slot.
test("a handle to a killed item goes stale", function()
  local it = trx.items[1]
  assert(it:is_valid())

  it:kill()
  assert(not it:is_valid(), "the handle should be stale after kill()")
  assert(fake.calls().kill == 1)

  -- Reading a stale handle raises rather than addressing whatever took over.
  raises(function()
    return it.hit_points
  end, "stale")
end)

test("spawn creates an item and validates its arguments", function()
  local w = trx.items.spawn(WOLF, { x = 100, y = 0, z = 100 })
  assert(w ~= nil, "spawn returned nil")
  assert(w.object_id == WOLF)
  assert(w.pos.x == 100 and w.pos.z == 100)
  assert(w.status == trx.items.Status.INACTIVE, "inert until asked")
  assert(w.is_active == false)
  assert(#trx.items == 3, "the pool should have grown")

  -- opts.activate brings a creature fully to life: active list plus AI.
  local live = trx.items.spawn(WOLF, { x = 0, y = 0, z = 0 }, 0, { activate = true })
  assert(live.is_active == true, "the activate option was ignored")
  assert(live.status == trx.items.Status.ACTIVE)
  assert(fake.calls().enable_baddie_ai == 1, "a creature needs its AI enabled")
end)

test("a spawn that raises does not leave an item behind", function()
  raises(function()
    trx.items.spawn(UNLOADED, { x = 0, y = 0, z = 0 })
  end, "not loaded")
  raises(function()
    trx.items.spawn(WOLF, { x = -1, y = 0, z = 0 })
  end, "outside")

  -- The rotation is read before a slot is taken, so a bad one cannot leak a
  -- half-initialised item into the pool.
  raises(function()
    trx.items.spawn(WOLF, { x = 0, y = 0, z = 0 }, "north")
  end)

  assert(#trx.items == 2, "a failed spawn must not consume a slot")
end)

test("explode runs the object's death handling; kill does not", function()
  trx.items[1]:explode()
  assert(fake.calls().creature_die == 1, "explode() should reach Creature_Die")
  assert(fake.calls().creature_die_explode, "and it should explode")

  fake.reset()
  trx.items[1]:kill()
  assert(fake.calls().creature_die == 0, "kill() just removes the item")
end)

test("properties overlay the object's defaults", function()
  local it = trx.items[1]

  -- With no override, a read falls back to the object.
  assert(it.properties.max_hit_points == 20, "the object's default")

  it.properties.max_hit_points = 50
  assert(it.properties.max_hit_points == 50, "the override did not stick")
  assert(it.max_hit_points == 50, "the field must agree with the property")

  -- Raising hit_points past the maximum raises the maximum with it.
  it.hit_points = 80
  assert(it.properties.max_hit_points == 80, "max_hit_points should follow")

  local seen = {}
  for k, v in pairs(it.properties) do
    seen[k] = v
  end
  assert(seen.max_hit_points == 80, "pairs() missed the property")

  raises(function()
    it.properties.no_such_property = 1
  end, "unknown")
end)

test("computed members and methods are declared", function()
  local it = trx.items[1]

  assert(it.room.num == it.room_num, "room must be a Room, not a number")
  assert(it:distance_to({ x = 0, y = 0, z = 0 }) == 0)
  assert(trx.items[2]:distance_to({ x = 0, y = 0, z = 0 }) == 1024)

  assert(it.is_alive == true, "a wolf with hit points is alive")
  assert(trx.items[2].is_alive == false, "a vase is not alive")
  assert(it.is_killed == false)
end)

test("find and first query by object and room", function()
  local wolves = trx.items.find({ object_id = WOLF })
  assert(#wolves == 1, "expected one wolf, got " .. #wolves)
  assert(wolves[1].object_id == WOLF)

  assert(#trx.items.find({ room_num = 2 }) == 1, "by room")
  assert(#trx.items.find({ object_id = WOLF, room_num = 2 }) == 0, "both must match")

  assert(trx.items.first({ object_id = VASE }) ~= nil)
  -- Nil, not an empty table: a table is truthy, so `if trx.items.first(q) then`
  -- would take the branch even when nothing matched.
  assert(trx.items.first({ object_id = 99 }) == nil, "no match must be nil")

  -- An unknown query key is logged and ignored, not fatal.
  assert(#trx.items.find({ nonsense = 1 }) == 2, "an unknown key is ignored")
  assert(#trx.items.find() == 0 and trx.items.first() == nil)
end)

-- The point of declaring the surface: a member C can reach but nobody names
-- simply does not exist. ITEM has nineteen of them.
test("undeclared members are unreachable", function()
  local it = trx.items[1]
  for _, name in ipairs({
    "box_num",
    "floor",
    "next_item",
    "next_active",
    "gen",
    "anim_num",
    "frame_num",
    "ai_bits",
    "after_death",
    "shade",
    "idx",
  }) do
    assert(it[name] == nil, name .. " must not be reachable")
  end

  -- A handle carries no keys of its own, either.
  raises(function()
    it.my_own_key = 1
  end)
end)

return h.report()
