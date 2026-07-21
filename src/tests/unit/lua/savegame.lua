local h = require("harness")
local test = h.test

test("the pools are the reflected enum", function()
  assert(trx.savegame.Pool.NORMAL == 0)
  assert(trx.savegame.Pool.QUICK == 1)
end)

test("slot_count defaults to the normal pool", function()
  assert(trx.savegame.slot_count() == 3)
  assert(trx.savegame.slot_count(trx.savegame.Pool.NORMAL) == 3)
end)

test("the quick pool counts only the slots on screen", function()
  assert(trx.savegame.slot_count(trx.savegame.Pool.QUICK) == 2)
end)

test("is_free reports which normal slots hold a save", function()
  assert(not trx.savegame.is_free(1))
  assert(trx.savegame.is_free(2))
  assert(trx.savegame.is_free(3))
end)

test("load starts the saved game in a slot", function()
  trx.savegame.load(2)
  assert(fake.calls().loaded_param == 1) -- slot 2 is index 1
end)

test("save writes to a numbered normal slot", function()
  assert(trx.savegame.save(2) == true)
  local calls = fake.calls()
  assert(calls.saved_pool == trx.savegame.Pool.NORMAL)
  assert(calls.saved_index == 1) -- slot 2 is index 1
end)

test("a quick save with no index uses the next slot in rotation", function()
  assert(trx.savegame.save(nil, trx.savegame.Pool.QUICK) == true)
  assert(fake.calls().saved_pool == trx.savegame.Pool.QUICK)
  assert(fake.calls().saved_index == 3) -- the rotating slot
end)

test("a quick save with an index respects it", function()
  assert(trx.savegame.save(1, trx.savegame.Pool.QUICK) == true)
  local calls = fake.calls()
  assert(calls.saved_pool == trx.savegame.Pool.QUICK)
  assert(calls.saved_index == 0) -- visual slot 1 is index 0
end)

return h.report()
