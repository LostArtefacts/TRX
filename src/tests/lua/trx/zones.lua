-- The zone API as a script actually sees it.
--
-- The item world is fake, and Lara is item 0 in it. fake.control() runs one
-- logical frame the way the game phase does.

local h = require("harness")
local test, raises = h.test, h.raises

local BOX_MIN = { x = 2048, y = -512, z = -512 }
local BOX_MAX = { x = 3072, y = 512, z = 512 }
local INSIDE = { x = 2500, y = 0, z = 0 }
local OUTSIDE = { x = 0, y = 0, z = 0 }

local function log_hooks(zone, log)
  zone:on_enter(function()
    log[#log + 1] = "enter"
  end)
  zone:on_tick(function()
    log[#log + 1] = "tick"
  end)
  zone:on_exit(function()
    log[#log + 1] = "exit"
  end)
end

test("a box fires enter, tick and exit as Lara moves", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX)
  local log = {}
  log_hooks(zone, log)

  fake.control()
  assert(#log == 0, "an empty zone must report nothing")

  trx.items[0].pos = INSIDE
  fake.control()
  assert(
    table.concat(log, ",") == "enter,tick",
    "entering must report enter, then tick"
  )

  fake.control()
  assert(
    table.concat(log, ",") == "enter,tick,tick",
    "staying is a tick a frame"
  )

  trx.items[0].pos = OUTSIDE
  fake.control()
  assert(table.concat(log, ",") == "enter,tick,tick,exit")
end)

test("a sphere measures from the middle", function()
  local zone = trx.zones.sphere({ x = 0, y = 0, z = 0 }, 1024)
  local log = {}
  log_hooks(zone, log)

  trx.items[0].pos = { x = 1025, y = 0, z = 0 }
  fake.control()
  assert(#log == 0, "a point past the radius is outside")

  trx.items[0].pos = { x = 1024, y = 0, z = 0 }
  fake.control()
  assert(table.concat(log, ",") == "enter,tick", "the radius itself is inside")

  -- Radius counts in every direction, not along the axes.
  trx.items[0].pos = { x = 1024, y = 1024, z = 0 }
  fake.control()
  assert(
    table.concat(log, ",") == "enter,tick,exit",
    "a corner is further out"
  )

  assert(zone.type == "sphere")
  assert(zone.radius == 1024)
  assert(zone.min == nil and zone.max == nil, "a sphere has no corners")
end)

test("a sphere refuses a negative radius", function()
  raises(function()
    trx.zones.sphere(OUTSIDE, -1)
  end, "radius")
end)

test("the corners of a box may come in any order", function()
  local zone = trx.zones.box(BOX_MAX, BOX_MIN)
  assert(zone:contains_point(INSIDE), "swapped corners span the same box")
  assert(not zone:contains_point(OUTSIDE))
  assert(zone.min.x == 2048 and zone.max.x == 3072)
  assert(zone.centre == nil and zone.radius == nil, "a box has no middle")
end)

test("only Lara sets a zone off unless it watches every item", function()
  local lara_only, items = 0, 0
  trx.zones.box(BOX_MIN, BOX_MAX):on_enter(function()
    lara_only = lara_only + 1
  end)
  trx.zones.box(BOX_MIN, BOX_MAX, { watch = "items" }):on_enter(function()
    items = items + 1
  end)

  trx.items[1].pos = INSIDE
  fake.control()
  assert(lara_only == 0, "a zone watching Lara answered for another item")
  assert(items == 1, 'watch = "items" must see any item')
end)

test("a zone rejects a watch it does not know", function()
  raises(function()
    trx.zones.box(BOX_MIN, BOX_MAX, { watch = "everyone" })
  end, 'watch must be "lara" or "items"')
end)

test("exits report before enters, across zones", function()
  local a = trx.zones.box(BOX_MIN, BOX_MAX)
  local b = trx.zones.box(
    { x = 4096, y = -512, z = -512 },
    { x = 5120, y = 512, z = 512 }
  )
  local log = {}
  a:on_exit(function()
    log[#log + 1] = "exit a"
  end)
  b:on_enter(function()
    log[#log + 1] = "enter b"
  end)

  trx.items[0].pos = INSIDE
  fake.control()
  trx.items[0].pos = { x = 4500, y = 0, z = 0 }
  fake.control()
  assert(table.concat(log, ",") == "exit a,enter b")
end)

test("the hooks hand over the item the moment is about", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX, { watch = "items" })
  local seen = nil
  zone:on_enter(function(item)
    seen = item
  end)

  trx.items[1].pos = INSIDE
  fake.control()
  assert(seen == trx.items[1], "the handler did not receive the item")
end)

test("occupants are who was inside as of the last frame", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX, { watch = "items" })
  assert(#zone:occupants() == 0)

  trx.items[0].pos = INSIDE
  trx.items[1].pos = INSIDE
  fake.control()

  local inside = zone:occupants()
  assert(#inside == 2, "both items should be inside")
  assert(inside[1] == trx.items[0] and inside[2] == trx.items[1])
  assert(zone:contains_item(trx.items[0]))

  trx.items[1].pos = OUTSIDE
  fake.control()
  assert(#zone:occupants() == 1)
  assert(not zone:contains_item(trx.items[1]))
end)

test(
  "contains_item answers for the item, contains_point for a place",
  function()
    local zone = trx.zones.box(BOX_MIN, BOX_MAX, { watch = "items" })
    trx.items[1].pos = INSIDE

    assert(zone:contains_point(INSIDE))
    assert(zone:contains_item(trx.items[1]))

    -- An item the world does not hold is nowhere, though the place it stood is
    -- still inside the zone.
    fake.destroy(1)
    assert(zone:contains_point(INSIDE), "the place has not moved")
    assert(not zone:contains_item(trx.items[1]), "but the item is gone")
  end
)

test("an item destroyed inside a zone leaves it", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX, { watch = "items" })
  local log = {}
  zone:on_exit(function(item)
    log[#log + 1] = item
  end)

  trx.items[1].pos = INSIDE
  fake.control()
  assert(#zone:occupants() == 1)

  fake.destroy(1)
  assert(#log == 1, "a destroyed occupant must be reported as leaving")
  assert(#zone:occupants() == 0, "and must not be an occupant afterwards")

  -- Nothing is left over to report on the next frame.
  fake.control()
  assert(#log == 1)
end)

-- An item number is a slot the world hands out again, so an occupant kept past
-- its destruction would name the item that took the slot.
test("a disabled zone forgets a destroyed occupant without a word", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX, { watch = "items" })
  local exits = 0
  zone:on_exit(function()
    exits = exits + 1
  end)

  trx.items[1].pos = INSIDE
  fake.control()
  zone:disable()
  fake.destroy(1)
  assert(exits == 0, "a disabled zone must stay quiet")
  assert(#zone:occupants() == 0, "and must not still hold what was destroyed")

  zone:enable()
  fake.control()
  assert(exits == 0, "nothing is owed for it once the zone is back on")
end)

test("a zone removed by an exit handler does not hide the next one", function()
  local first = trx.zones.box(BOX_MIN, BOX_MAX, { watch = "items" })
  local second = trx.zones.box(BOX_MIN, BOX_MAX, { watch = "items" })
  local reached = false
  first:on_exit(function()
    first:remove()
  end)
  second:on_exit(function()
    reached = true
  end)

  trx.items[1].pos = INSIDE
  fake.control()
  fake.destroy(1)
  assert(reached, "the second zone must hear about the destruction as well")
end)

test("disabling suspends a zone without forgetting who is inside", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX)
  local log = {}
  zone:on_enter(function()
    log[#log + 1] = "enter"
  end)
  zone:on_exit(function()
    log[#log + 1] = "exit"
  end)

  trx.items[0].pos = INSIDE
  fake.control()
  zone:disable()
  assert(zone.enabled == false)
  trx.items[0].pos = OUTSIDE
  fake.control()
  assert(table.concat(log, ",") == "enter", "a disabled zone must stay quiet")

  zone:enable()
  fake.control()
  assert(
    table.concat(log, ",") == "enter,exit",
    "re-enabling must notice who left meanwhile"
  )
end)

test("enabled is a field as well as a pair of verbs", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX)
  assert(zone.enabled == true)
  zone.enabled = false
  assert(zone.enabled == false)
  zone:enable()
  assert(zone.enabled == true)
end)

test("clear_occupants makes a zone fire again for who never left", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX)
  local enters = 0
  zone:on_enter(function()
    enters = enters + 1
  end)

  trx.items[0].pos = INSIDE
  fake.control()
  fake.control()
  assert(enters == 1, "staying inside is not entering again")

  zone:clear_occupants()
  assert(#zone:occupants() == 0)
  fake.control()
  assert(enters == 2, "forgetting who is inside must let them enter again")
end)

test("a removed zone goes stale, stays quiet, and takes its hooks", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX)
  local calls = 0
  zone:on_enter(function()
    calls = calls + 1
  end)
  local others = 0
  trx.events.on_zone_enter(function()
    others = others + 1
  end)

  zone:remove()
  assert(not zone:is_valid(), "the handle must go stale")
  raises(function()
    return zone.enabled
  end, "stale")
  raises(function()
    return zone.num
  end, "stale")
  assert(#trx.zones == 0, "a removed zone must leave the module")

  trx.items[0].pos = INSIDE
  fake.control()
  assert(calls == 0, "a removed zone kept firing")

  -- The hook the zone carried is detached, so a later zone's transitions do not
  -- wake it.
  trx.zones.box(BOX_MIN, BOX_MAX)
  fake.control()
  assert(calls == 0, "a removed zone's hook answered for another zone")
  assert(others == 1, "a listener of its own must still hear about it")
end)

test("the module is a container, counted from one", function()
  assert(#trx.zones == 0)
  local first = trx.zones.box(BOX_MIN, BOX_MAX)
  local second = trx.zones.sphere(OUTSIDE, 512, { name = "bell" })

  assert(#trx.zones == 2 and trx.zones.count() == 2)
  assert(trx.zones[1] == first, "the first zone made is the first one indexed")
  assert(trx.zones[2] == second)
  assert(trx.zones[3] == nil)
  assert(trx.zones[0] == nil, "zones count from one")

  assert(trx.zones["bell"] == second, "a zone is reachable by its name")
  assert(trx.zones.get("bell") == second)
  assert(trx.zones["nobody"] == nil)
  assert(second.name == "bell" and first.name == nil)

  local walked = {}
  for key, zone in pairs(trx.zones) do
    walked[#walked + 1] = key
    assert(zone == trx.zones[key], "the key must reach the zone")
  end
  assert(#walked == 2 and walked[1] == 1 and walked[2] == 2)

  assert(first.num == 1 and second.num == 2, "a zone says where it sits")

  first:remove()
  assert(#trx.zones == 1 and trx.zones[1] == second, "the places shift up")
  assert(second.num == 1, "and a zone says so")
end)

test("a zone rejects a name that is not one", function()
  raises(function()
    trx.zones.box(BOX_MIN, BOX_MAX, { name = 7 })
  end, "name")
end)

test("a name names one zone", function()
  local gate = trx.zones.box(BOX_MIN, BOX_MAX, { name = "gate" })
  raises(function()
    trx.zones.sphere(INSIDE, 512, { name = "gate" })
  end, "already there")
  assert(#trx.zones == 1, "the second zone must not have been made")

  -- The name comes free with the zone that held it.
  gate:remove()
  local again = trx.zones.box(BOX_MIN, BOX_MAX, { name = "gate" })
  assert(trx.zones["gate"] == again)
end)

-- The fake rooms are four sectors wide and start a sector apart, so a position
-- lies in several at once and the tile takes the first that claims it. Item 1
-- stands a sector out, in room 1, which is the sector this tile covers.
test("a tile is one sector, in its own room", function()
  local plate = trx.zones.tile({ x = 1024, y = 0, z = 0 }, { watch = "items" })
  assert(plate.type == "tile")
  assert(plate.room_num == 0)
  assert(plate.min.x == 1024 and plate.max.x == 2047)
  assert(
    not plate:contains_point({ x = 2100, y = 0, z = 200 }),
    "the next sector"
  )

  local log = {}
  plate:on_enter(function()
    log[#log + 1] = "enter"
  end)

  fake.control()
  assert(
    #log == 0,
    "the same sector column in another room must not set off the tile"
  )

  trx.items[0].pos = { x = 1500, y = -1024, z = 800 }
  fake.control()
  assert(
    table.concat(log, ",") == "enter",
    "standing anywhere on the sector must set it off"
  )
end)

test("a tile outside the level is nil", function()
  assert(trx.zones.tile({ x = -5000, y = 0, z = 0 }) == nil)
  assert(#trx.zones == 0, "nothing should have been made")
end)

test("the global events fire for every zone, with the zone", function()
  local a = trx.zones.box(BOX_MIN, BOX_MAX, { name = "a" })
  local b = trx.zones.box(BOX_MIN, BOX_MAX, { name = "b" })
  local log = {}
  trx.events.on_zone_enter(function(zone, item)
    log[#log + 1] = zone.name
    assert(item == trx.items[0], "the item must come with the zone")
  end)
  trx.events.on_zone_exit(function(zone)
    log[#log + 1] = "out of " .. zone.name
  end)

  trx.items[0].pos = INSIDE
  fake.control()
  assert(table.concat(log, ",") == "a,b", "both zones must report")
  assert(a ~= b)

  trx.items[0].pos = OUTSIDE
  fake.control()
  assert(table.concat(log, ",") == "a,b,out of a,out of b")
end)

test("a zone hook detaches through trx.events", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX)
  local calls = 0
  local id = zone:on_enter(function()
    calls = calls + 1
  end)
  assert(trx.events.detach(id) == true, "the id must be an events id")

  trx.items[0].pos = INSIDE
  fake.control()
  assert(calls == 0, "a detached zone hook kept firing")
end)

test("a level change takes the zones and their handlers", function()
  local zone
  local calls = 0
  fake.as_level_script(function()
    zone = trx.zones.box(BOX_MIN, BOX_MAX)
    zone:on_enter(function()
      calls = calls + 1
    end)
  end)

  trx.items[0].pos = INSIDE
  fake.control()
  assert(calls == 1)

  fake.end_level()
  assert(#trx.zones == 0, "the zones must go with the level")
  assert(not zone:is_valid(), "a handle from the past level must be stale")

  trx.items[0].pos = OUTSIDE
  trx.zones.box(BOX_MIN, BOX_MAX)
  trx.items[0].pos = INSIDE
  fake.control()
  assert(calls == 1, "the past level's hook must not fire")
end)

-- The level script runs while the level loads, and the level starts afterwards.
test("the level starting leaves the script's zones alone", function()
  local calls = 0
  fake.as_level_script(function()
    trx.zones.box(BOX_MIN, BOX_MAX):on_enter(function()
      calls = calls + 1
    end)
  end)

  fake.game_start()
  assert(#trx.zones == 1, "the level's zones must outlive its first frame")

  trx.items[0].pos = INSIDE
  fake.control()
  assert(calls == 1, "and must still be watched")
end)

test("a level change detaches what a zone carried", function()
  local listener = trx.zones.box(BOX_MIN, BOX_MAX):on_enter(function() end)
  fake.end_level()
  assert(
    trx.events.detach(listener) == false,
    "the hook must have gone with the zone"
  )
end)

-- A flyby camera is not an item, so it sets off hooks of its own and leaves the
-- item ones alone. The fake camera stands in room 0, as the box zones do.
test("a flyby sets off its own hooks, and no others", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX, { watch = "items" })
  local log = {}
  zone:on_enter(function()
    log[#log + 1] = "enter"
  end)
  zone:on_tick(function()
    log[#log + 1] = "tick"
  end)
  zone:on_flyby_enter(function()
    log[#log + 1] = "flyby in"
  end)
  zone:on_flyby_exit(function()
    log[#log + 1] = "flyby out"
  end)

  fake.flyby(INSIDE)
  fake.control()
  assert(
    table.concat(log, ",") == "flyby in",
    "a flyby must set off the flyby hooks alone"
  )

  fake.control()
  assert(table.concat(log, ",") == "flyby in", "and only as it crosses in")

  fake.flyby({ x = 0, y = 0, z = 0 })
  fake.control()
  assert(table.concat(log, ",") == "flyby in,flyby out")

  assert(#zone:occupants() == 0, "a camera is not an occupant")
end)

test("a sequence ending inside a zone leaves it", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX)
  local log = {}
  zone:on_flyby_enter(function()
    log[#log + 1] = "in"
  end)
  zone:on_flyby_exit(function()
    log[#log + 1] = "out"
  end)

  fake.flyby(INSIDE)
  fake.control()
  fake.flyby()
  fake.control()
  assert(
    table.concat(log, ",") == "in,out",
    "the camera stopped being anywhere"
  )
end)

test(
  "a flyby hook takes nothing, and the global one takes the zone",
  function()
    local zone = trx.zones.box(BOX_MIN, BOX_MAX, { name = "hall" })
    local handed, named = nil, nil
    zone:on_flyby_enter(function(...)
      handed = select("#", ...)
    end)
    trx.events.on_zone_flyby_enter(function(fired)
      named = fired.name
    end)

    fake.flyby(INSIDE)
    fake.control()
    assert(handed == 0, "a flyby hook has nothing to hand over")
    assert(named == "hall", "the global event names the zone")
  end
)

test("a tile answers for a flyby by room, as it does for an item", function()
  local plate = trx.zones.tile({ x = 3500, y = 0, z = 200 })
  local enters = 0
  plate:on_flyby_enter(function()
    enters = enters + 1
  end)

  -- The right sector, in another of the rooms that cover it.
  fake.flyby({ x = 3500, y = 0, z = 200 }, 2)
  fake.control()
  assert(enters == 0, "another room must not set off the tile")

  fake.flyby({ x = 3500, y = 0, z = 200 }, 0)
  fake.control()
  assert(enters == 1)
end)

test("a disabled zone ignores a flyby too", function()
  local zone = trx.zones.box(BOX_MIN, BOX_MAX)
  local enters = 0
  zone:on_flyby_enter(function()
    enters = enters + 1
  end)

  zone:disable()
  fake.flyby(INSIDE)
  fake.control()
  assert(enters == 0, "a disabled zone must stay quiet")

  zone:enable()
  fake.control()
  assert(enters == 1, "and notice the camera once it is back on")
end)

return h.report()
