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
  trx.savegame.load(1)
  assert(fake.calls().loaded_param == 0) -- slot 1 is index 0
end)

test("reached_levels lists the numbered levels a save reaches", function()
  local nums = trx.savegame.reached_levels(1)
  assert(#nums == 1)
  assert(nums[1] == 1)
end)

test("reached_levels is nil for an empty slot", function()
  assert(trx.savegame.reached_levels(2) == nil)
end)

test("load raises for an empty slot", function()
  assert(not pcall(trx.savegame.load, 2))
  assert(fake.calls().loaded_param == -1)
end)

test("info reads what a slot holds", function()
  local info = trx.savegame.info(1)
  assert(info.level_title == "City of Vilcabamba")
  assert(info.counter == 7)
  assert(info.level_num == 2)
  assert(info.can_restart == true)
  assert(info.can_select_level == true)
  assert(info.has_story == true)
end)

test("info is nil for an empty slot", function()
  assert(trx.savegame.info(2) == nil)
end)

test("delete removes the save in a slot", function()
  assert(trx.savegame.delete(1, trx.savegame.Pool.QUICK) == true)
  local call = fake.calls().delete_save
  assert(call.count == 1)
  assert(call["slot.pool"] == trx.savegame.Pool.QUICK)
  assert(call["slot.index"] == 0)
end)

test(
  "delete reports nothing removed for a slot that does not exist",
  function()
    assert(trx.savegame.delete(0) == false)
    assert(fake.calls().delete_save.count == 0)
  end
)

test("play_story plays the story before a save", function()
  trx.savegame.play_story(1)
  assert(fake.calls().loaded_param == 0)
end)

test("play_story raises for an empty slot", function()
  assert(not pcall(trx.savegame.play_story, 2))
  assert(fake.calls().loaded_param == -1)
end)

test("play_story raises for a save with no story", function()
  assert(not pcall(trx.savegame.play_story, 1, trx.savegame.Pool.QUICK))
  assert(fake.calls().loaded_param == -1)
end)

test("total_count counts the saves", function()
  assert(trx.savegame.total_count() == 1)
end)

test("restart_available defaults to the running save", function()
  assert(trx.savegame.restart_available() == true)
end)

test("restart_available is false for a slot that does not exist", function()
  assert(trx.savegame.restart_available(1) == true)
  assert(trx.savegame.restart_available(0) == false)
end)

test("manual_allowed reports whether the level allows saving", function()
  assert(trx.savegame.manual_allowed == true)
end)

test("recent_slot names the slot that the game last used", function()
  local slot_num, pool = trx.savegame.recent_slot()
  assert(slot_num == 2)
  assert(pool == trx.savegame.Pool.NORMAL)
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
