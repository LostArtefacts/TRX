-- The item API as a script actually sees it.
--
-- Everything under these assertions is real: the ITEM FIELD_DESC table, the
-- struct and enum bridges, trxc.items, and src/lua/items.lua itself. Only the
-- engine below that is fake (see fakes/items.c). So these tests pin the
-- public surface rather than a restatement of it - rename a C member, drop a
-- declaration, mark a field writable that must not be, and the assertion that
-- names it fails.

local h = require("harness")
local test, raises = h.test, h.raises

local WOLF, VASE, UNLOADED = fake.WOLF, fake.VASE, fake.UNLOADED

test("items are indexed from 0, and by name", function()
  assert(#trx.items == 2, "length operator")
  assert(trx.items.count() == 2, "count()")
  assert(trx.items[0] ~= nil and trx.items[1] ~= nil)
  assert(trx.items[2] == nil, "out of range must be nil")

  -- Narrowed to the engine's index, 2^32 + 1 is 1. It must not come back as
  -- item 1.
  assert(trx.items[4294967297] == nil, "a wide index must not wrap into range")
  assert(trx.items[-1] == nil)

  trx.items[0].name = "wolfie"
  assert(trx.items["wolfie"] ~= nil, "lookup by name")
  assert(trx.items.get("wolfie").name == "wolfie")
  assert(trx.items["nobody"] == nil, "an unknown name must be nil")

  -- A name already in use raises rather than silently rebinding.
  raises(function()
    trx.items[1].name = "wolfie"
  end)
end)

test("pairs walks the items in order, keyed from 0", function()
  local seen = {}
  for num, item in pairs(trx.items) do
    assert(item == trx.items[num], "the key must be the item index")
    seen[#seen + 1] = num
  end
  assert(#seen == 2, "pairs must yield every item")
  assert(seen[1] == 0 and seen[2] == 1, "pairs must count from 0, in order")
end)

test("fields read and write through to the struct", function()
  local it = trx.items[0]

  it.pos = { x = 512, y = 0, z = 256 }
  assert(it.pos.x == 512 and it.pos.z == 256, "pos did not stick")
  assert(trx.items[0].pos.x == 512, "not written through to the item")

  it.rot = { x = 0, y = 16384, z = 0 }
  assert(it.rot.y == 16384)

  -- An angle counts in cycles, so a half turn on top of it wraps round.
  it.rot = { x = 0, y = it.rot.y + 32768, z = 0 }
  assert(it.rot.y == -16384, "the rotation should have wrapped")

  it.timer = 30
  assert(it.timer == 30)

  -- Rooms count from 0, matching the engine and the level editor.
  assert(it.room_num == 0)
end)

-- These encode an engine invariant. Writing them directly would let a script
-- wedge the item, so the declaration withholds them even though the C member is
-- plain and the struct would happily take the write.
test("read-only fields refuse writes", function()
  for _, name in ipairs({
    "touch_bits",
    "object_id",
    "room_num",
    "is_simulated",
    "is_present",
    "max_hit_points",
  }) do
    raises(function()
      trx.items[0][name] = 1
    end, "read-only")
  end
end)

test("a field that does not exist raises rather than being created", function()
  raises(function()
    trx.items[0].no_such_field = 1
  end, "unknown")
end)

test("a value the field cannot hold raises instead of truncating", function()
  -- hit_points is 16-bit.
  raises(function()
    trx.items[0].hit_points = 99999
  end)

  trx.items[0].hit_points = 5
  assert(trx.items[0].hit_points == 5)
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
  local _, state = pairs(trx.items.PickupMode)
  pcall(function()
    state.NORMAL = 999
  end)
  assert(
    trx.items.PickupMode.NORMAL == 0,
    "an enum was written to through pairs()"
  )

  local seen = {}
  for name, value in pairs(trx.items.PickupMode) do
    seen[name] = value
  end
  assert(
    seen.NORMAL == 0 and seen.PLINTH_LOW == 1,
    "pairs() must still yield the constants"
  )
  assert(seen.PLINTH_HIGH == 2)
end)

test("activate() moves the item into play", function()
  local it = trx.items[0]
  assert(it.is_simulated == false)
  assert(it.is_in_play == false, "not live until simulated")

  it:activate()
  assert(it.is_simulated == true, "activate() did not take")
  assert(it.is_in_play == true, "a running, visible item is in play")
end)

-- Item 0 is a creature. Putting it on the active list is not enough: without its
-- AI it stands there and ignores Lara, which is not what activate() promises.
test("activating a creature enables its AI, by force", function()
  trx.items[0]:activate()
  assert(fake.calls().enable_baddie_ai == 1, "a creature needs its AI enabled")
  assert(
    fake.calls().enable_baddie_ai_forced == true,
    "a named activation takes a slot by force"
  )
end)

test("activating an inert object does not touch AI", function()
  local vase = trx.items[1]
  vase:activate()
  assert(vase.is_simulated == true)
  assert(fake.calls().enable_baddie_ai == 0, "a vase has no AI to enable")
end)

test("deactivate stops the item and takes a creature's AI away", function()
  local wolf = trx.items[0]
  wolf:activate()
  wolf:deactivate()
  assert(
    wolf.is_simulated == false,
    "deactivate did not take it off the active list"
  )
  assert(not wolf.is_finished, "it goes back to inactive, not finished")
  assert(
    fake.calls().disable_baddie_ai == 1,
    "the creature's AI was left running"
  )
end)

test("an item knows its own index, counted from 0", function()
  assert(trx.items[0].index == 0)
  assert(trx.items[1].index == 1)
end)

-- A door reads its trigger before it acts, which activate() alone does not set.
test("trigger sets the item running and its trigger reads active", function()
  local door = trx.items[1]
  assert(door.is_triggered == false)

  door:trigger()
  assert(door.is_simulated == true, "a full trigger starts the item")
  assert(door.is_triggered == true, "and its trigger now says go")
  assert(door.trigger_mask == 31, "a lone trigger carries every code bit")
end)

test(
  "an antitrigger takes the trigger back but leaves the item running",
  function()
    local door = trx.items[1]
    door:trigger()
    door:trigger({ type = trx.items.TriggerType.ANTITRIGGER })
    assert(door.is_triggered == false, "the trigger no longer says go")
    assert(
      door.is_simulated == true,
      "the item is left running so it can animate shut"
    )
  end
)

test("a partial trigger waits for the rest", function()
  local door = trx.items[1]
  door:trigger({ mask = 1 })
  assert(door.is_simulated == false, "one code bit is not enough to start it")
  assert(door.is_triggered == false)
end)

test("a mask outside the five bits is refused", function()
  raises(function()
    trx.items[1]:trigger({ mask = 99 })
  end)
end)

test("an unknown trigger type is refused", function()
  raises(function()
    trx.items[1]:trigger({ type = 99 })
  end)
end)

local ANTITRIGGER = 4 -- trx.items.TriggerType.ANTITRIGGER

test("on_trigger passes the item and the trigger's fundamentals", function()
  local seen_item, seen_trigger
  trx.events.on_trigger(function(item, trigger)
    seen_item = item
    seen_trigger = trigger
  end)

  fake.fire_trigger(1, ANTITRIGGER, 7, 2.5, true)

  assert(seen_item == trx.items[1], "on_trigger did not receive the item")
  assert(seen_trigger.type == trx.items.TriggerType.ANTITRIGGER)
  assert(seen_trigger.mask == 7)
  assert(seen_trigger.timer == 2.5)
  assert(seen_trigger.one_shot == true)
end)

test("item:on_trigger fires only for that item, with the trigger", function()
  local count, mask = 0, nil
  trx.items[1]:on_trigger(function(item, trigger)
    count = count + 1
    assert(item == trx.items[1], "the wrong item reached the handler")
    mask = trigger.mask
  end)

  fake.fire_trigger(0, 0, 31, 0, false)
  assert(count == 0, "a trigger on another item must not fire this one")
  fake.fire_trigger(1, 0, 31, 0, false)
  assert(count == 1, "a trigger on this item must fire it")
  assert(mask == 31, "the trigger fundamentals did not reach the handler")
end)

test("on_hit passes the item and the damage it took", function()
  local seen_item, seen_damage
  trx.events.on_hit(function(item, damage)
    seen_item = item
    seen_damage = damage
  end)

  fake.fire_hit(1, 25)

  assert(seen_item == trx.items[1], "on_hit did not receive the item")
  assert(seen_damage == 25, "on_hit did not receive the damage")
end)

test("item:on_hit fires only for that item, with the damage", function()
  local count, damage = 0, nil
  trx.items[1]:on_hit(function(item, taken)
    count = count + 1
    assert(item == trx.items[1], "the wrong item reached the handler")
    damage = taken
  end)

  fake.fire_hit(0, 10)
  assert(count == 0, "damage to another item must not fire this one")
  fake.fire_hit(1, 10)
  assert(count == 1, "damage to this item must fire it")
  assert(damage == 10, "the damage did not reach the handler")
end)

test("on_kill passes the item damage brought down", function()
  local seen
  trx.events.on_kill(function(item)
    seen = item
  end)

  fake.fire_kill(1)

  assert(seen == trx.items[1], "on_kill did not receive the item")
end)

test("item:on_kill reports after item:on_hit on the fatal blow", function()
  local order = {}
  trx.items[1]:on_hit(function()
    order[#order + 1] = "hit"
  end)
  trx.items[1]:on_kill(function()
    order[#order + 1] = "kill"
  end)

  fake.fire_hit(1, 25)
  assert(#order == 1 and order[1] == "hit", "a survivable hit must not kill")

  fake.fire_hit(1, 25)
  fake.fire_kill(1)
  assert(#order == 3, "the fatal blow must fire both")
  assert(order[2] == "hit" and order[3] == "kill", "hit reports before kill")
end)

test("on_show and on_hide pass the item that changed visibility", function()
  local shown, hidden
  trx.events.on_show(function(item)
    shown = item
  end)
  trx.events.on_hide(function(item)
    hidden = item
  end)

  fake.fire_visibility(1, true)
  fake.fire_visibility(1, false)

  assert(shown == trx.items[1], "on_show did not receive the item")
  assert(hidden == trx.items[1], "on_hide did not receive the item")
end)

test("item:on_show and item:on_hide fire only for that item", function()
  local shows, hides = 0, 0
  trx.items[1]:on_show(function(item)
    shows = shows + 1
    assert(item == trx.items[1], "the wrong item reached on_show")
  end)
  trx.items[1]:on_hide(function(item)
    hides = hides + 1
    assert(item == trx.items[1], "the wrong item reached on_hide")
  end)

  fake.fire_visibility(0, true)
  fake.fire_visibility(0, false)
  assert(shows == 0 and hides == 0, "another item's visibility must not fire")
  fake.fire_visibility(1, true)
  fake.fire_visibility(1, false)
  assert(shows == 1, "this item becoming visible must fire on_show")
  assert(hides == 1, "this item becoming hidden must fire on_hide")
end)

test("on_finish and item:on_finish pass the item that finished", function()
  local seen
  trx.events.on_finish(function(item)
    seen = item
  end)
  local count = 0
  trx.items[1]:on_finish(function(item)
    count = count + 1
    assert(item == trx.items[1], "the wrong item reached on_finish")
  end)

  fake.fire_finish(0)
  assert(count == 0, "another item finishing must not fire this one")
  fake.fire_finish(1)
  assert(seen == trx.items[1], "on_finish did not receive the item")
  assert(count == 1, "this item finishing must fire it")
end)

test("on_enter_sim and on_leave_sim pass the item that changed", function()
  local entered, left
  trx.events.on_enter_sim(function(item)
    entered = item
  end)
  trx.events.on_leave_sim(function(item)
    left = item
  end)

  fake.fire_sim(1, true)
  fake.fire_sim(1, false)

  assert(entered == trx.items[1], "on_enter_sim did not receive the item")
  assert(left == trx.items[1], "on_leave_sim did not receive the item")
end)

test(
  "item:on_enter_sim and item:on_leave_sim fire only for that item",
  function()
    local ins, outs = 0, 0
    trx.items[1]:on_enter_sim(function()
      ins = ins + 1
    end)
    trx.items[1]:on_leave_sim(function()
      outs = outs + 1
    end)

    fake.fire_sim(0, true)
    fake.fire_sim(0, false)
    assert(ins == 0 and outs == 0, "another item's simulation must not fire")
    fake.fire_sim(1, true)
    fake.fire_sim(1, false)
    assert(ins == 1, "this item starting simulation must fire on_enter_sim")
    assert(outs == 1, "this item stopping simulation must fire on_leave_sim")
  end
)

test("on_activate and on_deactivate pass the item", function()
  local activated, deactivated
  trx.events.on_activate(function(item)
    activated = item
  end)
  trx.events.on_deactivate(function(item)
    deactivated = item
  end)

  fake.fire_activation(1, true)
  fake.fire_activation(1, false)

  assert(activated == trx.items[1], "on_activate did not receive the item")
  assert(deactivated == trx.items[1], "on_deactivate did not receive the item")
end)

test(
  "item:on_activate and item:on_deactivate fire only for that item",
  function()
    local ons, offs = 0, 0
    trx.items[1]:on_activate(function()
      ons = ons + 1
    end)
    trx.items[1]:on_deactivate(function()
      offs = offs + 1
    end)

    fake.fire_activation(0, true)
    fake.fire_activation(0, false)
    assert(ons == 0 and offs == 0, "another item's activation must not fire")
    fake.fire_activation(1, true)
    fake.fire_activation(1, false)
    assert(ons == 1, "this item's activation must fire on_activate")
    assert(offs == 1, "this item's deactivation must fire on_deactivate")
  end
)

test("on_destroy and item:on_destroy pass the item being removed", function()
  local seen
  trx.events.on_destroy(function(item)
    seen = item
  end)
  local count = 0
  trx.items[1]:on_destroy(function(item)
    count = count + 1
    assert(item == trx.items[1], "the wrong item reached on_destroy")
  end)

  fake.fire_destroy(0)
  assert(count == 0, "another item being removed must not fire this one")
  fake.fire_destroy(1)
  assert(seen == trx.items[1], "on_destroy did not receive the item")
  assert(count == 1, "this item being removed must fire it")
end)

test("on_enter_world and on_leave_world pass the item that changed", function()
  local entered, left
  trx.events.on_enter_world(function(item)
    entered = item
  end)
  trx.events.on_leave_world(function(item)
    left = item
  end)

  fake.fire_world(1, true)
  fake.fire_world(1, false)

  assert(entered == trx.items[1], "on_enter_world did not receive the item")
  assert(left == trx.items[1], "on_leave_world did not receive the item")
end)

test(
  "item:on_enter_world and item:on_leave_world fire only for that item",
  function()
    local ins, outs = 0, 0
    trx.items[1]:on_enter_world(function()
      ins = ins + 1
    end)
    trx.items[1]:on_leave_world(function()
      outs = outs + 1
    end)

    fake.fire_world(0, true)
    fake.fire_world(0, false)
    assert(
      ins == 0 and outs == 0,
      "another item entering the world must not fire"
    )
    fake.fire_world(1, true)
    fake.fire_world(1, false)
    assert(ins == 1, "this item entering the world must fire on_enter_world")
    assert(outs == 1, "this item leaving the world must fire on_leave_world")
  end
)

-- An index alone would rebind to whatever item recycled the slot; the
-- generation counter is what makes the handle go stale instead.
test("a handle to a destroyed item goes stale", function()
  local it = trx.items[0]
  assert(it:is_valid())

  it:destroy()
  assert(not it:is_valid(), "the handle should be stale after destroy()")
  assert(fake.calls().destroy == 1)

  -- Reading a stale handle raises rather than addressing whatever took over.
  raises(function()
    return it.hit_points
  end, "stale")
end)

-- Every lookup mints a fresh handle, so equality has to compare what the two
-- handles point at rather than the userdata itself.
test("two handles to the same item are equal", function()
  assert(
    trx.items[0] == trx.items[0],
    "the same item twice must compare equal"
  )
  assert(trx.items[0] == trx.items.get(0))
  assert(trx.items[0] ~= trx.items[1], "different items must not")

  local it = trx.items[0]
  assert(it ~= 1 and it ~= "wolf" and it ~= nil)

  -- The slot is what a stale handle still names, and the generation is what
  -- tells the two occupants apart.
  it:destroy()
  local fresh = trx.items[0]
  assert(it ~= fresh, "a stale handle must not equal a live one")
end)

-- pairs() hands the iterator to the script, so it is reachable with whatever
-- the script cares to pass it.
test("the field iterator refuses a value that is not an item", function()
  local it = trx.items[0]

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
  assert(w.is_simulated == false, "inert until asked")
  assert(not w.is_in_play)
  assert(#trx.items == 3, "the pool should have grown")

  -- opts.activate brings a creature fully to life: active list plus AI.
  local live = trx.items.spawn(
    WOLF,
    { x = 0, y = 0, z = 0 },
    0,
    { activate = true }
  )
  assert(live.is_simulated == true, "the activate option was ignored")
  assert(live.is_in_play == true)
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

test("die runs the object's death handling; destroy does not", function()
  trx.items[0]:die()
  assert(fake.calls().creature_die == 1, "die() should reach Creature_Die")
  assert(
    not fake.calls().creature_die_explode,
    "die() does not burst by default"
  )

  fake.reset()
  trx.items[0]:die(true)
  assert(
    fake.calls().creature_die_explode,
    "die(true) should burst the meshes"
  )

  fake.reset()
  trx.items[0]:destroy()
  assert(fake.calls().creature_die == 0, "destroy() just removes the item")
end)

test("take_damage takes the item's hit points", function()
  fake.reset()
  local it = trx.items[0]
  local before = it.hit_points

  it:take_damage(5)
  assert(fake.calls().take_damage == 1, "take_damage should reach the engine")
  assert(
    fake.calls().take_damage_amount == 5,
    "the damage should pass through"
  )
  assert(it.hit_points == before - 5, "the hit points should come down")

  it:take_damage(it.hit_points)
  assert(it.hit_points == 0, "the last blow should empty the hit points")
end)

test("shatter bursts the meshes on their own", function()
  fake.reset()
  trx.items[0]:shatter()
  assert(fake.calls().shatter == 1, "shatter() should reach Item_Shatter")
  assert(fake.calls().shatter_damage == 0, "no splash damage by default")
  assert(fake.calls().creature_die == 0, "shatter() is not a death")

  trx.items[0]:shatter(5)
  assert(fake.calls().shatter_damage == 5, "the damage should pass through")
end)

test("is_one_shot reads and sets the trigger flag", function()
  local it = trx.items[0]
  assert(it.is_one_shot == false)
  it.is_one_shot = true
  assert(it.is_one_shot == true, "the flag did not stick")
  it.is_one_shot = false
  assert(it.is_one_shot == false)
end)

test("properties overlay the object's defaults", function()
  local it = trx.items[0]

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
  local it = trx.items[0]

  assert(it.room.num == it.room_num, "room must be a Room, not a number")
  assert(it:distance_to({ x = 0, y = 0, z = 0 }) == 0)
  assert(trx.items[1]:distance_to({ x = 0, y = 0, z = 0 }) == 1024)

  assert(it.is_alive == true, "a wolf with hit points is alive")
  assert(trx.items[1].is_alive == false, "a vase is not alive")
  assert(it.is_killed == false)
end)

test("query first is a handle or nil, never an empty table", function()
  assert(trx.items.query:of_object(VASE):first() ~= nil)
  -- Nil, not an empty table: a table is truthy, so `if q:first() then` would
  -- take the branch even when nothing matched.
  assert(trx.items.query:of_object(99):first() == nil, "no match must be nil")
end)

-- A method reaches C directly, so strict mode has to put its wrapper in front
-- of the C function rather than around a Lua one.
test("strict mode checks a method's arguments", function()
  local it = trx.items[0]
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
  local it = trx.items[0]
  trx.api.strict(true)
  local ok = pcall(function()
    return it.distance_to({ x = 0, y = 0, z = 0 })
  end)
  trx.api.strict(false)
  assert(not ok, "strict mode let a method run without its handle")
end)

-- ITEM has many members the declaration does not name.
test("undeclared members are unreachable", function()
  local it = trx.items[0]
  for _, name in ipairs({
    "box_num",
    "floor",
    "next_item",
    "next_simulated",
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

test("query of_object narrows by object, taken by id or by name", function()
  assert(#trx.items.query:of_object(WOLF):matches() == 1, "by id")
  assert(trx.items.query:of_object(WOLF):matches()[1].object_id == WOLF)
  -- A name resolves through the object query, the way a player would type it.
  assert(#trx.items.query:of_object("wolf"):matches() == 1, "by name")
  assert(#trx.items.query:of_object("vase"):matches() == 1)
end)

test("query of_object matches nothing for a name nobody has", function()
  assert(trx.items.query:of_object("wombat"):count() == 0)
end)

test("query in_room narrows by room", function()
  assert(#trx.items.query:in_room(1):matches() == 1, "the vase is in room 1")
  assert(trx.items.query:in_room(1):matches()[1].object_id == VASE)
  assert(trx.items.query:in_room(0):count() == 1, "the wolf is in room 0")
end)

test("query ids() are item numbers", function()
  local ids = trx.items.query:of_object(VASE):ids()
  assert(#ids == 1 and ids[1] == 1, "the vase is item 1")
end)

test("query active tracks the active list", function()
  assert(trx.items.query:simulated():count() == 0, "nothing starts active")
  assert((~trx.items.query:simulated()):count() == 2)

  trx.items[0]:activate()
  assert(trx.items.query:simulated():count() == 1, "the wolf woke up")
  assert(trx.items.query:simulated():matches()[1].object_id == WOLF)
end)

test("query | unions, & intersects", function()
  local q = trx.items.query
  assert((q:of_object(WOLF) | q:of_object(VASE)):count() == 2)
  assert(
    (q:of_object(WOLF) & q:in_room(1)):count() == 0,
    "the wolf is in room 0"
  )
end)

test("a query cannot cross domains", function()
  raises(function()
    local _ = trx.items.query:simulated() & trx.objects.query:pickup()
  end)
end)

return h.report()
