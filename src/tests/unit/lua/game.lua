-- The game flow API as a script actually sees it.
--
-- The fake flow has three levels - a gym, then two ordinary ones - plus one
-- cutscene and one demo. The gym is the case worth having: it has no number, so
-- a level's `num` is not its place in the list.

local h = require("harness")
local test, raises = h.test, h.raises

test("the level list is a list of Level handles", function()
  local levels = trx.game.levels
  assert(#levels == fake.LEVEL_COUNT, "wrong number of levels")
  assert(levels[2].title == "Caves")
  assert(levels[3].title == "Vilcabamba")
end)

test("a level's num is its number, not its place in the list", function()
  local levels = trx.game.levels
  -- The gym is first in the table and has no number at all.
  assert(levels[1].type == trx.game.LevelType.GYM)
  assert(levels[1].num == 0, "a gym has no number")

  -- And the levels after it are numbered from 1, as the player sees them.
  assert(levels[2].num == 1)
  assert(levels[3].num == 2)
end)

test("a level carries what the game flow said about it", function()
  local caves = trx.game.levels[2]
  assert(caves.path == "level1.phd")
  assert(caves.script_path == "caves.lua")
  assert(caves.lara_outfit == "default")
  assert(caves.water_particles == true)
  assert(caves.unobtainable_pickups == 1)
  assert(caves.unobtainable_secrets == 2)
end)

test("a level is read-only", function()
  raises(function()
    trx.game.levels[2].title = "Nope"
  end)
  raises(function()
    trx.game.levels[2].num = 99
  end)
end)

test("a member of GF_LEVEL nobody declared is not reachable", function()
  -- The struct has a sequence, injections and item drops. None is declared.
  local caves = trx.game.levels[2]
  assert(caves.sequence == nil)
  assert(caves.injections == nil)
  assert(caves.item_drops == nil)
end)

test("cutscenes and demos are their own lists", function()
  assert(#trx.game.cutscenes == 1)
  assert(trx.game.cutscenes[1].title == "Cutscene 1")
  assert(trx.game.cutscenes[1].type == trx.game.LevelType.CUTSCENE)

  assert(#trx.game.demos == 1)
  assert(trx.game.demos[1].type == trx.game.LevelType.DEMO, "DEMO must exist as a level type")
end)

test("current_level is nil until a level is playing", function()
  assert(trx.game.current_level == nil)

  fake.set_current_level(2)
  assert(trx.game.current_level ~= nil, "no current level")
  assert(trx.game.current_level.title == "Caves")
end)

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
  assert(trx.game.trx_version == "TRX-test")
end)

test("play_level queues the level", function()
  fake.set_current_level(2)
  trx.game.play_level(3)
  local calls = fake.calls()
  assert(calls.play_level == 1, "play_level did not reach the game flow")
  -- Lua counts from 1, the game flow from 0.
  assert(calls.last_num == 2)
end)

test("play_level rejects a level that is not there", function()
  raises(function()
    trx.game.play_level(99)
  end)
  assert(fake.calls().play_level == 0)
end)

test("settings is gone", function()
  -- It was five aliases over trx.config, and it wrote through the destructive
  -- set. One way to reach a setting is enough.
  assert(trx.game.settings == nil)
end)

test("an undeclared name cannot be written onto the module", function()
  raises(function()
    trx.game.nonsense = 1
  end)
  assert(trx.game.nonsense == nil)
end)

return h.report()
