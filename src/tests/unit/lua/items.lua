-- The item API as a script actually sees it.
--
-- Everything under these assertions is real: the ITEM FIELD_DESC table, the
-- struct and enum bridges, trxc.items, and src/lua/items.lua itself. Only the
-- engine below that is fake (see fake_engine_items.c). So these tests pin the
-- public surface rather than a restatement of it - rename a C member, drop a
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

  -- Narrowed to the engine's index, 2^32 + 1 is 1. It must not come back as
  -- item 1.
  assert(trx.items[4294967297] == nil, "a wide index must not wrap into range")
  assert(trx.items[-1] == nil)

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
  -- hit_points is 16-bit.
  raises(function()
    trx.items[1].hit_points = 99999
  end)

  trx.items[1].hit_points = 5
  assert(trx.items[1].hit_points == 5)
end)

test("the pickup mode enum is reached through trx.items", function()
  assert(trx.items.PickupMode.NORMAL == 0)
  assert(trx.items.PickupMode.PLINTH_LOW == 1)
  assert(trx.items.PickupMode.PLINTH_HIGH == 2)
  assert(trx.pickup == nil, "trx.pickup must not exist")
end)

-- The enum is held behind an empty table so every write goes through
-- __newindex. pairs() hands the caller everything __pairs returns, so what it
-- returns must not be that table.
test("pairs() does not hand out the table behind an enum", function()
  local _, state = pairs(trx.items.Status)
  pcall(function()
    state.ACTIVE = 999
  end)
  assert(
    trx.items.Status.ACTIVE == 1,
    "an enum was written to through pairs()"
  )

  local seen = {}
  for name, value in pairs(trx.items.Status) do
    seen[name] = value
  end
  assert(
    seen.INACTIVE == 0 and seen.ACTIVE == 1,
    "pairs() must still yield the constants"
  )
  assert(seen.DEACTIVATED == 2 and seen.INVISIBLE == 3)
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

-- An index alone would rebind to whatever item recycled the slot; the
-- generation counter is what makes the handle go stale instead.
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

-- Every lookup mints a fresh handle, so equality has to compare what the two
-- handles point at rather than the userdata itself.
test("two handles to the same item are equal", function()
  assert(
    trx.items[1] == trx.items[1],
    "the same item twice must compare equal"
  )
  assert(trx.items[1] == trx.items.get(1))
  assert(trx.items[1] ~= trx.items[2], "different items must not")

  local it = trx.items[1]
  assert(it ~= 1 and it ~= "wolf" and it ~= nil)

  -- The slot is what a stale handle still names, and the generation is what
  -- tells the two occupants apart.
  it:kill()
  local fresh = trx.items[1]
  assert(it ~= fresh, "a stale handle must not equal a live one")
end)

-- pairs() hands the iterator to the script, so it is reachable with whatever
-- the script cares to pass it.
test("the field iterator refuses a value that is not an item", function()
  local it = trx.items[1]

  local seen = {}
  for k, v in pairs(it) do
    seen[k] = v
  end
  assert(seen.timer ~= nil, "pairs() should yield the declared fields")

  local iter = pairs(it)
  raises(function()
    iter({}, nil)
  end)
  raises(function()
    iter(nil, nil)
  end)
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
  local live = trx.items.spawn(
    WOLF,
    { x = 0, y = 0, z = 0 },
    0,
    { activate = true }
  )
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

  -- A position table missing a coordinate names the coordinate and the
  -- argument it came in on, not the stack slot the field landed at.
  raises(function()
    trx.items.spawn(WOLF, { x = 0, z = 0 })
  end, "y")
  raises(function()
    trx.items.spawn(WOLF, { x = 0, y = 0, z = 0.5 })
  end, "z")

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
  assert(
    #trx.items.find({ object_id = WOLF, room_num = 2 }) == 0,
    "both must match"
  )

  assert(trx.items.first({ object_id = VASE }) ~= nil)
  -- Nil, not an empty table: a table is truthy, so `if trx.items.first(q) then`
  -- would take the branch even when nothing matched.
  assert(trx.items.first({ object_id = 99 }) == nil, "no match must be nil")

  -- An unknown query key is logged and ignored, not fatal.
  assert(#trx.items.find({ nonsense = 1 }) == 2, "an unknown key is ignored")
  assert(#trx.items.find() == 0 and trx.items.first() == nil)
end)

-- A method reaches C directly, so strict mode has to put its wrapper in front
-- of the C function rather than around a Lua one.
test("strict mode checks a method's arguments", function()
  local it = trx.items[1]
  trx.api.strict(true)

  local ok = pcall(function()
    -- A colon call with a bad argument.
    it:distance_to("over there")
  end)
  trx.api.strict(false)
  assert(not ok, "strict mode let a bad argument through to a method")

  -- And it still works with a good one, strict or not.
  trx.api.strict(true)
  local distance = it:distance_to({ x = 0, y = 0, z = 0 })
  trx.api.strict(false)
  assert(distance == 0)
  assert(it:distance_to({ x = 0, y = 0, z = 0 }) == 0)
end)

-- The handle is the method's first argument, and a dot where a colon was meant
-- passes whatever came first instead.
test("strict mode catches a method called with a dot", function()
  local it = trx.items[1]
  trx.api.strict(true)
  local ok = pcall(function()
    return it.distance_to({ x = 0, y = 0, z = 0 })
  end)
  trx.api.strict(false)
  assert(not ok, "strict mode let a method run without its handle")
end)

-- ITEM has many members the declaration does not name.
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
