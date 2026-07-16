-- The room API as a script actually sees it.
--
-- Everything under these assertions is real: the ROOM FIELD_DESC table, the
-- struct and enum bridges, trxc.rooms, and src/lua/rooms.lua itself. Only the
-- engine below that is fake (see fake_engine_rooms.c).
--
-- The fake level has four rooms. Room 0 and room 1 are a flip pair; rooms 2 and
-- 3 are ordinary.

local h = require("harness")
local test, raises = h.test, h.raises

test("rooms are indexed from 0, and counted", function()
  assert(#trx.rooms == 4, "length operator")
  assert(trx.rooms.count() == 4, "count()")

  assert(trx.rooms[0].num == 0, "the first room is 0")
  assert(trx.rooms[3].num == 3)
  assert(trx.rooms.get(1).num == 1, "get()")

  assert(trx.rooms[-1] == nil, "a negative index must be nil")
  assert(trx.rooms[4] == nil, "out of range must be nil")
  assert(trx.rooms["2"] == nil, "a numeric string is not a room number")

  -- Narrowed to the engine's index, 2^32 + 1 is 1. It must not come back as
  -- room 1.
  assert(trx.rooms[4294967297] == nil, "a wide index must not wrap into range")
end)

test("pairs walks the rooms in order, keyed from 0", function()
  local seen = {}
  for num, room in pairs(trx.rooms) do
    assert(room.num == num, "the key must be the room number")
    seen[#seen + 1] = num
  end
  assert(#seen == 4, "pairs must yield every room")
  assert(seen[1] == 0 and seen[4] == 3, "pairs must count from 0, in order")
end)

test("room flags read and write as booleans", function()
  local r = trx.rooms[0]

  -- An unset flag reads false, not nil: `if room.underwater == nil` must not be
  -- how a dry room is detected.
  assert(r.underwater == false, "an unset flag must be false, not nil")

  r.underwater = true
  assert(r.underwater == true, "the flag did not stick")
  assert(trx.rooms[0].underwater == true, "not written through to the room")
  assert(trx.rooms[1].underwater == false, "wrote to the wrong room")

  -- flags is a nested struct of plain bools, so this also proves offsetof
  -- reaches through it to the right member.
  r.wind, r.damaging, r.cold = true, true, true
  assert(r.wind and r.damaging and r.cold)
end)

test("room flags accept booleans only", function()
  raises(function()
    trx.rooms[0].wind = 1
  end)
  raises(function()
    trx.rooms[0].cold = nil
  end)
end)

test("flip_status is read-only and matches the enum", function()
  assert(trx.rooms.FlipStatus.NONE == 0)
  assert(trx.rooms.FlipStatus.UNFLIPPED == 1)
  assert(trx.rooms.FlipStatus.FLIPPED == 2)

  assert(trx.rooms[0].flip_status == trx.rooms.FlipStatus.UNFLIPPED)
  assert(trx.rooms[1].flip_status == trx.rooms.FlipStatus.FLIPPED)
  assert(trx.rooms[2].flip_status == trx.rooms.FlipStatus.NONE)

  raises(function()
    trx.rooms[0].flip_status = 2
  end, "read-only")
end)

test("the fn compat table is gone", function()
  assert(trx.rooms.fn == nil, "trx.rooms.fn should not exist")
  assert(trx.rooms.FlipStatus ~= nil, "FlipStatus must be reachable")
  assert(trx.rooms.get ~= nil and trx.rooms.flip ~= nil)
end)

test("computed members are derived in Lua", function()
  local r = trx.rooms[0]

  local b = r.bounds
  assert(b.min_x == 0 and b.max_x == 4096, "bounds")
  assert(b.min_z == 0 and b.max_z == 4096)

  -- internal_bounds excludes the outer ring of sectors, which is solid wall.
  local ib = r.internal_bounds
  assert(ib.min_x == b.min_x + 1024, "internal_bounds should be inset")
  assert(ib.max_z == b.max_z - 1024)
  assert(ib.min_y == b.min_y and ib.max_y == b.max_y, "y is not inset")

  assert(r.flipped_room.num == 1, "flipped_room must be a Room, not a number")
  assert(trx.rooms[2].flipped_room == nil, "a room with no pair gives nil")
end)

test("module functions reach the engine", function()
  trx.rooms.flip()
  assert(fake.calls().flip_map == 1, "flip() should reach Room_FlipMap")

  trx.rooms.flip_effect(3, 10)
  assert(fake.calls().flip_effect == 3)
  assert(fake.calls().flip_timer == 10)
end)

test("find_valid_pos nudges a position, or gives nil", function()
  local pos, num = trx.rooms.find_valid_pos({ x = 0, y = 0, z = 0 }, 0)
  assert(pos ~= nil and num == 0, "expected a valid position")
  assert(pos.y == 16, "the nudged position must come back")

  assert(trx.rooms.find_valid_pos({ x = -1, y = 0, z = 0 }, 0) == nil)
end)

-- The rooms of the next level sit where the old ones did, so a bounds check
-- alone would let a held handle name a different room.
test("a room handle goes stale at a level change", function()
  local r = trx.rooms[0]
  assert(r:is_valid())
  assert(r.num == 0)

  fake.load_next_level()
  assert(not r:is_valid(), "the handle should be stale after a level change")

  raises(function()
    return r.underwater
  end, "stale")
  raises(function()
    r.underwater = true
  end, "stale")

  -- A handle taken from the level that is loaded is fine.
  assert(trx.rooms[0]:is_valid())
end)

-- The point of declaring the surface: a member C can reach but nobody names
-- simply does not exist.
test("undeclared members are unreachable", function()
  local r = trx.rooms[0]
  for _, name in ipairs({
    "pos",
    "size",
    "ambient",
    "num_lights",
    "item_num",
    "effect_num",
    "water_scheme",
    "reverb_info",
    "alternate_group",
    "flags",
    "outside",
    "inside",
    "swamp",
  }) do
    assert(r[name] == nil, name .. " must not be reachable")
  end
end)

return h.report()
