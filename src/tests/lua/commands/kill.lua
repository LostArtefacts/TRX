-- The kill cheat, through the console.
--
-- The fake level holds a wolf at the origin, next to Lara, and an enemy jeep
-- that a test places two sectors out.

local h = require("harness")
local test = h.test

local function kill(args)
  return fake.run("kill", args or "")
end

local function first_of(object_id)
  return trx.items.query:of_object(object_id):matches()[1]
end

test("kill all kills the hostiles", function()
  assert(kill("all") == trx.console.Result.OK)
  assert(first_of(fake.WOLF).hit_points == 0)
end)

test("kill all leaves the enemy jeep", function()
  fake.place_enemy_jeep()
  assert(not first_of(trx.catalog.objects.enemy_jeep).can_take_damage)
  assert(kill("all") == trx.console.Result.OK)
  assert(first_of(fake.WOLF).hit_points == 0)
  local jeep = first_of(trx.catalog.objects.enemy_jeep)
  assert(jeep.hit_points == 40, "the jeep should survive")
end)

test("kill leaves the enemy jeep when it is the nearest hostile", function()
  fake.place_enemy_jeep()
  assert(kill("all") == trx.console.Result.OK)
  assert(kill() == trx.console.Result.FAILURE)
  local jeep = first_of(trx.catalog.objects.enemy_jeep)
  assert(jeep.hit_points == 40, "the jeep should survive")
end)

return h.report()
