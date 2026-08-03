-- The fixed-point trigonometry a script places things with. The assertions are
-- on the engine's own tables: landing where the engine would is the whole
-- point.

local h = require("harness")
local test = h.test

local DEG_90 = trx.math.DEG_90

test("the angle constants divide a turn the way the engine does", function()
  assert(DEG_90 * 4 == 0x10000, "four quarter turns make a full turn")
  assert(trx.math.DEG_45 * 8 == 0x10000)
  -- 65536/360 is not whole, so a degree is 182 and 360 of them fall 16 short.
  assert(trx.math.DEG_1 == 182)
  assert(trx.math.DEG_1 * 360 == 0x10000 - 16)
  assert(trx.math.WALL_L == 1024)
end)

test("sin and cos run the circle", function()
  assert(trx.math.sin(0) == 0.0)
  assert(trx.math.cos(0) == 1.0)
  assert(math.abs(trx.math.sin(DEG_90) - 1.0) < 0.001)
  assert(math.abs(trx.math.cos(DEG_90)) < 0.001)
  assert(math.abs(trx.math.sin(2 * DEG_90)) < 0.001)
  assert(math.abs(trx.math.cos(2 * DEG_90) + 1.0) < 0.001)
end)

test("a full turn wraps to where it started", function()
  assert(trx.math.sin(4 * DEG_90) == trx.math.sin(0))
  assert(trx.math.cos(4 * DEG_90) == trx.math.cos(0))
end)

-- The engine's angles run clockwise from +z, and atan takes z first.
test("atan takes z before x", function()
  assert(trx.math.atan(0, 0) == 0)
  assert(trx.math.atan(1024, 0) == 0, "straight along +z is angle zero")
  assert(
    trx.math.atan(0, 1024) == DEG_90,
    "straight along +x is a quarter turn"
  )
  assert(trx.math.atan(1024, 1024) == trx.math.DEG_45)
end)

test("round_to_sector snaps to the corner west and north", function()
  local corner = trx.math.round_to_sector({ x = 2500, y = -300, z = 1024 })
  assert(corner.x == 2048 and corner.z == 1024)
  assert(corner.y == -300, "a sector is a column, so the height is untouched")

  -- The same corner for anywhere in the sector, and the sector boundary belongs
  -- to the sector it opens.
  assert(trx.math.round_to_sector({ x = 3071, y = 0, z = 0 }).x == 2048)
  assert(trx.math.round_to_sector({ x = 3072, y = 0, z = 0 }).x == 3072)
end)

test("round_to_sector rounds down either side of the origin", function()
  assert(trx.math.round_to_sector(0) == 0)
  assert(trx.math.round_to_sector(1023) == 0)
  assert(
    trx.math.round_to_sector(-1) == -1024,
    "west of the origin rounds west"
  )
  assert(trx.math.round_to_sector(-1024) == -1024)
  assert(trx.math.round_to_sector(-1025) == -2048)
end)

test("round_to_sector takes a coordinate as readily as a position", function()
  local pos = { x = -2500, y = 0, z = 2500 }
  local corner = trx.math.round_to_sector(pos)
  assert(corner.x == trx.math.round_to_sector(pos.x))
  assert(corner.z == trx.math.round_to_sector(pos.z))

  -- A float position still leaves the corner whole, so it can be read back as a
  -- world coordinate.
  assert(trx.math.round_to_sector(2500.5) == 2048)
  assert(math.type(trx.math.round_to_sector(2500.5)) == "integer")
end)

test("strict mode holds the angle to an integer", function()
  trx.api.strict(true)
  h.raises(function()
    trx.math.sin(1.5)
  end)
  trx.api.strict(false)
  assert(trx.math.sin(0) == 0.0)
end)

return h.report()
