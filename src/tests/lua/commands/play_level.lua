-- /play, dispatched through the console. The fake flow has two numbered levels
-- (Caves at 1, Vilcabamba at 2) and a gym at ordinal 0 titled "Lara's Home". The
-- play verb starts a level the level-select way, so `last_num` is the ordinal it
-- reached: this pins that a number, a title (spaces and all) and the gym each
-- resolve to the right one.

local h = require("harness")
local test = h.test
local R = trx.console.Result

local function play(args)
  return fake.run("play", args or "")
end

test("a number starts that level", function()
  assert(play("1") == R.OK)
  local c = fake.calls()
  assert(c.play_gym == 1, "a game was started")
  assert(c.last_num == 1, "the first level")
end)

test("a title starts its level, matched by name", function()
  assert(play("vilcabamba") == R.OK)
  assert(fake.calls().last_num == 2, "the second level")
end)

test("the word gym starts the gym", function()
  assert(play("gym") == R.OK)
  assert(fake.calls().last_num == 0, "the gym sits at ordinal zero")
end)

test("ordinal zero starts the gym", function()
  assert(play("0") == R.OK)
  assert(fake.calls().last_num == 0)
end)

test("the gym's title, spaces and all, starts the gym", function()
  assert(play("lara's home") == R.OK)
  assert(fake.calls().last_num == 0, "the two-word title did not resolve")
end)

test("an out-of-range number fails, starting nothing", function()
  assert(play("999") == R.FAILURE)
  assert(fake.calls().play_gym == 0)
end)

test("a negative ordinal fails, starting nothing", function()
  assert(play("-5") == R.FAILURE)
  assert(fake.calls().play_gym == 0)
end)

test("no argument is rejected before run", function()
  assert(play("") == R.FAILURE)
  assert(fake.calls().play_gym == 0)
end)

return h.report()
