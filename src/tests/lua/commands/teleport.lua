-- The teleport command, through the console.
--
-- What is pinned here is which objects a typed name may reach. Where Lara ends
-- up is the engine's, and the fake level has no floors to put her on.

local h = require("harness")
local test = h.test

local function tp(args)
  return fake.run("tp", args or "")
end

local function message()
  return fake.calls().log.message or ""
end

-- Where Lara stands after a teleport, so a test can say which item she went to.
-- A teleport puts Lara beside what she was sent to, not on it, so nearness is
-- what says which one she reached. The crystal stands sectors away from the
-- rest.
local function near(object_id)
  local pos = trx.items.query:of_object(object_id):first().pos
  return math.abs(trx.lara.item.pos.x - pos.x) <= trx.math.WALL_L
end

-- Nearer the crystal than the vase or the scion: a teleport takes the nearest
-- of what a name reached, so this is where a crystal swept up with the pickups
-- would show.
local function stand_by_the_crystal()
  trx.lara.teleport({ x = 2800, y = 0, z = 0 })
end

-- Nearer the scion, which is a pickup with a control routine of its own. Two
-- sectors off, not one: a teleport that lands within a sector is taken as Lara
-- already being there, and moves her on to the next thing.
local function stand_by_the_scion()
  trx.lara.teleport({ x = 6 * 1024, y = 0, z = 0 })
end

-- The level holds one vase and one crystal, both pickups.
test("a group name does not reach the savegame crystal", function()
  -- Standing on one saves the game where the mode allows it, so a teleport that
  -- swept it up with the other pickups would save rather than move her.
  stand_by_the_crystal()

  assert(tp("pickup") == trx.console.Result.OK, message())
  assert(near(fake.VASE), "pickup should have reached the vase")
  assert(not near(fake.CRYSTAL), "and never the crystal")
end)

-- A scion is a pickup that the generic pickup code does not collect, and it is
-- still one of the pickups a group name reaches.
test("a group name reaches a pickup with a routine of its own", function()
  stand_by_the_scion()

  assert(tp("pickup") == trx.console.Result.OK, message())
  assert(near(fake.SCION), "pickup should have reached the scion")
end)

test("its own name still reaches it", function()
  stand_by_the_crystal()

  assert(tp("crystal") == trx.console.Result.OK, message())
  assert(near(fake.CRYSTAL), "the crystal is a target when it is named")
end)

return h.report()
