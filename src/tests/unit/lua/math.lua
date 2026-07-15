-- The fixed-point trigonometry a script places things with. The assertions are on
-- the engine's own tables: landing where the engine would is the whole point.

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

test("strict mode holds the angle to an integer", function()
  trx.api.strict(true)
  h.raises(function()
    trx.math.sin(1.5)
  end)
  trx.api.strict(false)
  assert(trx.math.sin(0) == 0.0)
end)

return h.report()
