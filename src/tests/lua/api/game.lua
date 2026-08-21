-- The game flow API as a script actually sees it.
--
-- The fake flow has three levels - a gym, then two ordinary ones - plus one
-- cutscene and one demo. The gym is the case worth having: it has no number, so
-- a level's `num` is not its place in the list.

local h = require("harness")
local test, raises = h.test, h.raises

-- The list holds the levels the game numbers, which is what count_levels
-- counts. The gym sits in the table ahead of them and is not one of them, so
-- the list and the table disagree on where every level after it sits.
test("the level list is the levels the game numbers", function()
  local levels = trx.game.levels
  assert(#levels == fake.NUMBERED_LEVEL_COUNT, "wrong number of levels")
  assert(levels[1].title == "Caves")
  assert(levels[2].title == "Vilcabamba", "the last level must be reachable")
  assert(
    levels[1].type == trx.game.LevelType.NORMAL,
    "the gym is not in the list"
  )
end)

test("a level's num is its place in the game flow, not in the list", function()
  local levels = trx.game.levels
  assert(levels[1].num == 1)
  assert(levels[2].num == 2)
end)

test("a level carries what the game flow said about it", function()
  local caves = trx.game.levels[1]
  assert(caves.path == "level1.phd")
  assert(caves.script_path == "caves.lua")
  assert(caves.lara_outfit == "default")
  assert(caves.water_particles == true)
  assert(caves.unobtainable_pickups == 1)
  assert(caves.unobtainable_secrets == 2)
end)

test("a level's key reads through", function()
  assert(trx.game.levels[1].key == "level1")
  assert(trx.game.levels[2].key == "level2")
  assert(trx.game.cutscenes[1].key == nil, "a level with no file has no key")
end)

test("a level is read-only", function()
  raises(function()
    trx.game.levels[1].title = "Nope"
  end)
  raises(function()
    trx.game.levels[1].num = 99
  end)
end)

test("a member of GF_LEVEL nobody declared is not reachable", function()
  -- The struct has a sequence, injections and item drops. None is declared.
  local caves = trx.game.levels[1]
  assert(caves.sequence == nil)
  assert(caves.injections == nil)
  assert(caves.item_drops == nil)
end)

test("cutscenes and demos are their own lists", function()
  assert(#trx.game.cutscenes == 1)
  assert(trx.game.cutscenes[1].title == "Cutscene 1")
  assert(trx.game.cutscenes[1].type == trx.game.LevelType.CUTSCENE)

  assert(#trx.game.demos == 1)
  assert(
    trx.game.demos[1].type == trx.game.LevelType.DEMO,
    "DEMO must exist as a level type"
  )
end)

test("current_level is nil until a level is playing", function()
  assert(trx.game.current_level == nil)

  fake.set_current_level(2)
  assert(trx.game.current_level ~= nil, "no current level")
  assert(trx.game.current_level.title == "Caves")
end)

-- Neither is in the list the game numbers, and the title level is not even in a
-- table, so both are only reachable as the level being played.
test(
  "the gym and the title level still resolve as the current level",
  function()
    fake.set_current_level(1)
    local gym = trx.game.current_level
    assert(gym ~= nil, "the gym must resolve")
    assert(gym.type == trx.game.LevelType.GYM)
    assert(gym.num == 0, "a gym has no number")

    fake.set_current_title()
    local title = trx.game.current_level
    assert(title ~= nil, "the title level must resolve")
    assert(title.type == trx.game.LevelType.TITLE)
    assert(title.title == "Title")
  end
)

test("the enums come from C", function()
  assert(trx.game.LevelTable.MAIN ~= nil)
  assert(trx.game.LevelTable.CUTSCENES ~= nil)
  assert(trx.game.LevelTable.DEMOS ~= nil)
  assert(trx.game.LevelTable.TITLE ~= nil)

  assert(trx.game.LevelType.TITLE ~= nil)
  assert(trx.game.LevelType.DEMO ~= nil)
  assert(trx.game.LevelType.NORMAL ~= trx.game.LevelType.GYM)
end)

test("version reads through", function()
  assert(trx.game.version == 1)
  assert(trx.game.TRX_VERSION == "TRX-test")
end)

test("play_level queues the level the list named", function()
  fake.set_current_level(2)

  -- The second level of the list is the third entry of the table, because the
  -- gym sits ahead of them. What reaches the game flow is the level's own num.
  trx.game.play_level(2)
  local calls = fake.calls()
  assert(calls.play_level.count == 1, "play_level did not reach the game flow")
  assert(calls.play_level.num == trx.game.levels[2].num)
  assert(calls.play_level.num == 2)
end)

test("play_level rejects a level that is not there", function()
  raises(function()
    trx.game.play_level(99)
  end)
  raises(function()
    trx.game.play_level(0)
  end)
  -- A number too wide for the engine's would otherwise wrap into range and
  -- start the level it landed on.
  raises(function()
    trx.game.play_level(4294967297)
  end)
  assert(fake.calls().play_level.count == 0)
end)

-- The gym has no ordinal, so play_level cannot name it; play_gym is the only
-- way to reach it, and it queues the gym's own num.
test("play_gym queues the gym", function()
  trx.game.play_gym()
  local calls = fake.calls()
  assert(calls.play_gym.count == 1, "play_gym did not reach the game flow")
  assert(calls.play_gym.num == 0, "the gym's own num reaches the game flow")
  assert(
    calls.play_level.count == 0,
    "the gym is not one of the numbered levels"
  )
end)

test("play_gym raises when the game has no gym", function()
  fake.set_gym_present(false)
  raises(function()
    trx.game.play_gym()
  end)
  assert(fake.calls().play_gym.count == 0)
end)

test("is_loaded says whether a level is up", function()
  assert(trx.game.is_loaded == false)
  fake.set_current_level(2)
  assert(trx.game.is_loaded == true)
end)

test("a cutscene is loaded, but not playable", function()
  fake.set_current_level(2)
  assert(trx.game.is_playable == true)

  fake.set_in_cutscene(true)
  assert(trx.game.is_loaded == true, "the level is still loaded")
  assert(trx.game.is_playable == false, "but the game is not taking input")
end)

test("a setting is reached through trx.config, and nowhere else", function()
  assert(trx.game.settings == nil)
end)

test("an undeclared name cannot be written onto the module", function()
  raises(function()
    trx.game.nonsense = 1
  end)
  assert(trx.game.nonsense == nil)
end)

test("screenshot reaches the engine, with a path or without", function()
  trx.game.screenshot()
  trx.game.screenshot("shot.png")
end)

test("end_level reaches the engine", function()
  trx.game.end_level()
  assert(fake.calls().end_level.count == 1)
end)

test("a plain run is not a new game plus one", function()
  assert(trx.game.is_ngplus == false)
  fake.set_ngplus(true)
  assert(trx.game.is_ngplus == true)
end)

return h.report()
