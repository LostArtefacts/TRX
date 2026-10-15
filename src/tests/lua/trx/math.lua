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

test("rotate turns a vector as the engine turns a model", function()
  local ahead = { x = 0, y = 0, z = 1024 }
  local turned = trx.math.rotate(ahead, { x = 0, y = DEG_90, z = 0 })
  assert(turned.x == 1024 and turned.y == 0 and turned.z == 0, "yaw")
  local pitched = trx.math.rotate(ahead, { x = DEG_90, y = 0, z = 0 })
  assert(pitched.x == 0 and pitched.z == 0 and pitched.y ~= 0, "pitch")
  local still = trx.math.rotate(ahead, { x = 0, y = 0, z = 0 })
  assert(still.x == 0 and still.y == 0 and still.z == 1024, "no turn")
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

test("an angle wraps into the engine's range", function()
  assert(trx.math.wrap_angle(32768) == -32768)
  assert(trx.math.wrap_angle(-32769) == 32767)
  assert(trx.math.wrap_angle(70000) == 4464)
end)

test("integer division drops the fraction towards zero", function()
  assert(trx.math.idiv(7, 2) == 3)
  assert(trx.math.idiv(-7, 2) == -3)
  assert(trx.math.idiv(7, -2) == -3)
  assert(trx.math.idiv(-8, 2) == -4)
end)

test("a blend rounds halves away from zero", function()
  assert(trx.math.lerp(0, 10, 0.5) == 5)
  assert(trx.math.lerp(0, 3, 0.5) == 2)
  assert(trx.math.lerp(0, -3, 0.5) == -2)
end)

test("an angle blends the short way round", function()
  local from, to = -trx.math.DEG_90 * 2 + 100, trx.math.DEG_90 * 2 - 100
  assert(trx.math.lerp_angle(from, to, 0.5) == -32768)
  assert(trx.math.lerp_angle(0, trx.math.DEG_90, 0.5) == trx.math.DEG_90 // 2)
end)

test("a color goes to OKLCH and back unchanged", function()
  for _, hex in ipairs({ "ffffff", "000000", "ff0000", "33e5ff", "804020" }) do
    local color = trx.math.color(hex)
    local back = trx.math.from_oklch(trx.math.to_oklch(color))
    assert(back.hex == hex, hex .. " came back as " .. back.hex)
  end
end)

test("OKLCH reads red and white as the space defines them", function()
  local l, c, h_ = trx.math.to_oklch(trx.math.color("ff0000"))
  assert(math.abs(l - 0.628) < 0.001, l)
  assert(math.abs(c - 0.2577) < 0.001, c)
  assert(math.abs(h_ - 29.23) < 0.01, h_)
  local white_l, white_c = trx.math.to_oklch(trx.math.color("ffffff"))
  assert(math.abs(white_l - 1) < 0.001 and white_c < 0.001)
end)

return h.report()
