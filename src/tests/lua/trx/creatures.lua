-- The creature API as a script actually sees it.
--
-- hostile_allies is the one read/write api.property in the surface, so this is
-- where the setter path is exercised against a real bridge.

local h = require("harness")
local test, raises = h.test, h.raises

test("hostile_allies reads and writes through to the engine", function()
  assert(trx.creatures.hostile_allies == false, "allies start friendly")

  trx.creatures.hostile_allies = true
  assert(trx.creatures.hostile_allies == true, "the write did not stick")

  trx.creatures.hostile_allies = false
  assert(trx.creatures.hostile_allies == false, "the write did not stick")
end)

test("an undeclared field cannot be written onto the module", function()
  raises(function()
    trx.creatures.hostile_enemies = true
  end)
  assert(trx.creatures.hostile_enemies == nil)
end)

test("add_ally names the object it was given", function()
  trx.creatures.add_ally(42)
  local calls = fake.calls()
  assert(calls.add_ally.count == 1, "add_ally did not reach the engine")
  assert(calls.add_ally.obj_id == 42, "the wrong object was made an ally")
  assert(calls.add_ally_target.count == 0, "add_ally must not target anything")
end)

test("add_ally_target names the object it was given", function()
  trx.creatures.add_ally_target(7)
  local calls = fake.calls()
  assert(
    calls.add_ally_target.count == 1,
    "add_ally_target did not reach the engine"
  )
  assert(calls.add_ally_target.obj_id == 7)
  assert(calls.add_ally.count == 0, "add_ally_target must not make an ally")
end)

-- Creature_AddAlly indexes the object table with what it is given.
test("an object the game has no such id for is refused", function()
  raises(function()
    trx.creatures.add_ally(-1)
  end)
  -- Narrowed to the engine's id, 2^32 + 42 is 42.
  raises(function()
    trx.creatures.add_ally(4294967338)
  end)
  raises(function()
    trx.creatures.add_ally_target(4294967338)
  end)
  local calls = fake.calls()
  assert(calls.add_ally.count == 0 and calls.add_ally_target.count == 0)
end)

test("the raw bridge is not part of the surface", function()
  -- The C table has are_allies_hostile/set_allies_hostile. The declaration
  -- turns them into one property, and the raw pair must not leak alongside it.
  assert(trx.creatures.are_allies_hostile == nil)
  assert(trx.creatures.set_allies_hostile == nil)
end)

return h.report()
