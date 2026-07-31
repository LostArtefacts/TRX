-- What Lara carries, as a script sees it.

local h = require("harness")
local test, raises = h.test, h.raises

-- The fake counts what it is given without mapping a pickup to its icon, so
-- these name one id throughout.
local KEY = 42
local UZIS = trx.catalog.weapons.UZIS

test("give puts things in and count reads them back", function()
  assert(trx.inventory:count(KEY) == 0, "she starts with nothing")
  assert(not trx.inventory:has(KEY))

  assert(trx.inventory:give(KEY) == 1, "one went in")
  assert(trx.inventory:give(KEY, 3) == 3)
  assert(trx.inventory:count(KEY) == 4)
  assert(trx.inventory:has(KEY))
end)

test("take takes them back out and stops when she runs out", function()
  trx.inventory:give(KEY, 2)

  assert(trx.inventory:take(KEY) == 1)
  assert(trx.inventory:count(KEY) == 1)
  assert(trx.inventory:take(KEY, 5) == 1, "only what she had came out")
  assert(trx.inventory:count(KEY) == 0)
  assert(trx.inventory:take(KEY) == 0, "nothing left to take")
end)

test("a count below one is refused, and neither gives nor takes", function()
  for _, count in ipairs({ 0, -1 }) do
    raises(function()
      trx.inventory:give(KEY, count)
    end)
    raises(function()
      trx.inventory:take(KEY, count)
    end)
  end
  assert(trx.inventory:count(KEY) == 0)
end)

-- Inv_AddItem reaches the object table with whatever it is handed, so the
-- bridge turns an id the engine does not know into a raise of its own.
test("an object the engine does not know is refused", function()
  for _, name in ipairs({ "give", "take", "count", "icon_of", "can_add" }) do
    raises(function()
      trx.inventory[name](trx.inventory, 999999)
    end)
  end
end)

test("a level that does not carry the icon takes nothing", function()
  fake.set_can_add(false)

  assert(not trx.inventory:can_add(KEY))
  assert(trx.inventory:give(KEY, 3) == 0, "the give did nothing")
  assert(trx.inventory:count(KEY) == 0)

  fake.set_can_add(true)
  assert(trx.inventory:can_add(KEY))
end)

test("a weapon knows its pickup", function()
  assert(trx.weapons.object(UZIS) ~= nil)
end)

test("ammunition is counted in shots, whatever a shot costs", function()
  local SHOTGUN = trx.catalog.weapons.SHOTGUN

  assert(trx.inventory:shots(UZIS) == 0)
  trx.inventory:set_shots(UZIS, 50)
  assert(trx.inventory:shots(UZIS) == 50, "one round to the shot")

  trx.inventory:set_shots(SHOTGUN, 10)
  assert(
    trx.inventory:shots(SHOTGUN) == 10,
    "six rounds to the shot, and the count is still in shots"
  )

  assert(trx.weapons.shots_per_box(UZIS) == 10)
  assert(
    trx.weapons.shots_per_box(SHOTGUN) == 10,
    "a box is worth the same number of shots either way"
  )

  trx.inventory:set_shots(UZIS, 0)
  trx.inventory:set_shots(SHOTGUN, 0)
end)

test("a weapon goes in and comes back out", function()
  local uzi_item = trx.weapons.object(UZIS)

  assert(not trx.inventory:has_weapon(UZIS))
  assert(trx.inventory:give(uzi_item) == 1, "the level carries the icon")
  assert(trx.inventory:has_weapon(UZIS))
  assert(trx.inventory:take(uzi_item) == 1)
  assert(not trx.inventory:has_weapon(UZIS))
end)

test("the level says which weapons it allows", function()
  assert(not trx.weapons.is_available(UZIS), "off unless the level says")
  fake.set_weapon_available(UZIS, true)
  assert(trx.weapons.is_available(UZIS))
end)

test("anything that is not a weapon is refused", function()
  for _, value in ipairs({ 0, -1, 999 }) do
    raises(function()
      trx.inventory:shots(value)
    end)
  end
  raises(function()
    trx.inventory:set_shots(UZIS, -1)
  end)
end)

test("icon_of names the icon, not the entry", function()
  local icon = trx.inventory:icon_of(KEY)
  assert(icon == KEY, "a pickup with an icon of its own stands for itself")
  assert(
    trx.inventory:icon_of(KEY) == icon,
    "and answers whether or not she is carrying any"
  )
end)

test("the module counts and walks what she carries", function()
  assert(#trx.inventory == 0, "she starts with nothing")

  trx.inventory:give(KEY, 2)
  assert(#trx.inventory == 1, "one kind of thing, whatever the count")

  local entry = trx.inventory[1]
  assert(entry.object == KEY)
  assert(entry.count == 2)

  local seen = {}
  for pos, walked in pairs(trx.inventory) do
    seen[pos] = walked.object
  end
  assert(seen[1] == KEY, "walking counts from one")
  assert(seen[0] == nil and seen[2] == nil)

  trx.inventory:take(KEY, 2)
end)

test("an entry writes back to the inventory", function()
  trx.inventory:give(KEY)

  local entry = trx.inventory:entry(KEY)
  entry.count = 7
  assert(trx.inventory:count(KEY) == 7, "the write reached the inventory")
  assert(entry.count == 7, "and the entry reads it back")

  entry.count = 0
  assert(not trx.inventory:has(KEY), "zero takes it off her")

  trx.inventory:set_count(KEY, 0)
end)

test("an entry for something she has none of is nil", function()
  assert(trx.inventory:entry(KEY) == nil)
  assert(trx.inventory[1] == nil, "and so is a position past the end")
end)

test("a count cannot be set below zero", function()
  raises(function()
    trx.inventory:set_count(KEY, -1)
  end, "count must be 0 or more")
  raises(function()
    trx.inventory:give(KEY)
    trx.inventory:entry(KEY).count = -1
  end, "a count cannot be negative")
  trx.inventory:set_count(KEY, 0)
end)

return h.report()
