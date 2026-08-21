-- What a script keeps across a save.
--
-- The round trip is the whole point of the module, so most of this drives a
-- value through it rather than reading the surface back.

local h = require("harness")
local test = h.test

local function round_trip()
  assert(fake.round_trip(), "the store did not survive a save")
end

test("a store is a table a script writes into", function()
  fake.clear_level()
  trx.store.level.count = 3
  assert(trx.store.level.count == 3)
end)

test("the tables keep their identity", function()
  local held = trx.store.level
  trx.store.level.count = 1
  round_trip()
  assert(rawequal(held, trx.store.level), "the level store was replaced")
  assert(held.count == 1, "the held table did not follow the load")
end)

test("a save carries values of every kind it takes", function()
  fake.clear_level()
  local s = trx.store.level
  s.number = 12
  s.fraction = 1.5
  s.text = "iris"
  s.yes = true
  s.no = false
  round_trip()

  assert(trx.store.level.number == 12)
  assert(trx.store.level.fraction == 1.5)
  assert(trx.store.level.text == "iris")
  assert(trx.store.level.yes == true)
  assert(trx.store.level.no == false)
end)

test("a save carries nested tables and their key types", function()
  fake.clear_level()
  trx.store.level.nested = { [1] = "one", key = { deep = 7 } }
  round_trip()

  local nested = trx.store.level.nested
  assert(type(nested) == "table", "the nested table did not survive")
  assert(nested[1] == "one", "a number key came back as something else")
  assert(nested.key.deep == 7, "the deeper table did not survive")
end)

test("the two scopes are separate", function()
  fake.clear_level()
  fake.clear_game()
  trx.store.level.who = "level"
  trx.store.game.who = "game"
  round_trip()

  assert(trx.store.level.who == "level")
  assert(trx.store.game.who == "game")
end)

test("clearing a scope leaves the other one alone", function()
  fake.clear_level()
  fake.clear_game()
  trx.store.level.who = "level"
  trx.store.game.who = "game"

  fake.clear_level()
  assert(trx.store.level.who == nil, "the level store was not emptied")
  assert(trx.store.game.who == "game", "the game store was emptied with it")
end)

test("a value a save cannot hold is dropped rather than fatal", function()
  fake.clear_level()
  trx.store.level.kept = 1
  trx.store.level.dropped = function() end
  round_trip()

  assert(trx.store.level.kept == 1, "the rest of the store went with it")
  assert(trx.store.level.dropped == nil, "a function came back from a save")
end)

test("a table held twice comes back as one table", function()
  fake.clear_level()
  local shared = { count = 1 }
  trx.store.level.here = shared
  trx.store.level.there = shared
  round_trip()

  local here = trx.store.level.here
  local there = trx.store.level.there
  assert(rawequal(here, there), "one table came back as two")
  here.count = 2
  assert(there.count == 2, "the two halves no longer share a table")
end)

test("a loop comes back whole", function()
  fake.clear_level()
  local loop = { name = "held" }
  loop.self = loop
  trx.store.level.loop = loop
  round_trip()

  local restored = trx.store.level.loop
  assert(type(restored) == "table", "the whole table went with the loop")
  assert(restored.name == "held", "what the table held went with the loop")
  assert(rawequal(restored.self, restored), "the loop did not close")
end)

test("a table that holds the store itself comes back", function()
  fake.clear_level()
  trx.store.level.root = trx.store.level
  round_trip()

  assert(
    rawequal(trx.store.level.root, trx.store.level),
    "the store did not come back as itself"
  )
end)

test("a save does not grow the store it round-trips", function()
  fake.clear_level()
  local shared = { count = 1 }
  trx.store.level.here = shared
  trx.store.level.there = shared
  round_trip()
  round_trip()

  local seen = 0
  for _ in pairs(trx.store.level) do
    seen = seen + 1
  end
  assert(seen == 2, "the store gained keys across two saves")
end)

return h.report()
