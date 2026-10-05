-- The fixed-point trigonometry a script places things with. The assertions are
-- on the engine's own tables: landing where the engine would is the whole
-- point.

local h = require("harness")
local test = h.test
local raises = h.raises

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

test("a length says itself in sectors and back", function()
  assert(trx.math.from_sectors(8) == 8 * trx.math.WALL_L)
  assert(trx.math.from_sectors(0.5) == trx.math.WALL_L // 2)
  assert(trx.math.from_sectors(-2) == -2 * trx.math.WALL_L)
  assert(trx.math.to_sectors(trx.math.WALL_L) == 1)
  assert(trx.math.to_sectors(trx.math.WALL_L // 2) == 0.5)
  raises(function()
    trx.math.from_sectors("8")
  end)
  raises(function()
    trx.math.to_sectors({})
  end)
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

test("a color is built from channels or from hex text", function()
  local gold = trx.math.color("ffbf20")
  assert(gold.r == 255 and gold.g == 191 and gold.b == 32)
  assert(gold.hex == "ffbf20")
  assert(tostring(gold) == "ffbf20")
  assert(gold == trx.math.color(255, 191, 32), "channels decide equality")
  assert(trx.math.color("#ffbf20") == gold, "a leading hash is taken")
end)

test("a channel and the hex text write the same color", function()
  local color = trx.math.color(0, 0, 0)
  color.r = 51
  color.hex = "33e5ff"
  assert(color.r == 51 and color.g == 229 and color.b == 255)

  -- A fractional channel keeps its fraction, and the hex text rounds it.
  color.g = 191.25
  assert(color.g == 191.25)
  assert(color.hex == "33bfff")
end)

test("a color joins text as its hex", function()
  local gold = trx.math.color("ffbf20")
  assert("tint: " .. gold == "tint: ffbf20")
  assert(gold .. " is the tint" == "ffbf20 is the tint")
  assert(gold .. gold == "ffbf20ffbf20")
  assert(1 .. gold == "1ffbf20", "a number joins it as readily as text")
end)

test("a color refuses what is not one", function()
  h.raises(function()
    trx.math.color(1, 2)
  end)
  h.raises(function()
    trx.math.color("ff")
  end)
  h.raises(function()
    trx.math.color(0, 0, 0).r = "red"
  end)
  h.raises(function()
    trx.math.color(0, 0, 0).a = 1
  end)
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
