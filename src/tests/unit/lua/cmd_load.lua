-- /load and /ql, dispatched through the console. The fake save system keeps ten
-- taken slots in each pool, so this pins that a number, `q`, `q2` and `quick`
-- reach the right pool and index; the slot spelling is read by a match matcher.

local h = require("harness")
local test = h.test
local R = trx.console.Result
local Pool = trx.savegame.Pool

local function load(a)
  return fake.run("load", a or "")
end

local function ql(a)
  return fake.run("ql", a or "")
end

test("a number loads that normal slot", function()
  assert(load("3") == R.OK)
  local c = fake.calls()
  assert(c.load_index == 3 and c.load_pool == Pool.NORMAL)
end)

test("q loads the most recent quick save", function()
  assert(load("q") == R.OK)
  local c = fake.calls()
  assert(c.load_index == 1 and c.load_pool == Pool.QUICK)
end)

test("q2 loads the second quick save", function()
  assert(load("q2") == R.OK)
  local c = fake.calls()
  assert(c.load_index == 2 and c.load_pool == Pool.QUICK)
end)

test("quick, spelled out, reaches the quick pool", function()
  assert(load("quick") == R.OK)
  assert(fake.calls().load_pool == Pool.QUICK)
end)

test("an out-of-range slot fails, loading nothing", function()
  assert(load("999") == R.FAILURE)
  assert(fake.calls().load_count == 0)
end)

test("nonsense is refused before run", function()
  assert(load("blah") == R.FAILURE)
  assert(fake.calls().load_count == 0)
end)

test("quickload with no argument takes the most recent quick save", function()
  assert(ql("") == R.OK)
  local c = fake.calls()
  assert(c.load_index == 1 and c.load_pool == Pool.QUICK)
end)

test("quickload with a number takes that quick slot", function()
  assert(ql("3") == R.OK)
  local c = fake.calls()
  assert(c.load_index == 3 and c.load_pool == Pool.QUICK)
end)

test("quickload reads the q-spellings, not just a bare number", function()
  assert(ql("q2") == R.OK)
  assert(fake.calls().load_index == 2, "q2 is the second quick save")
  assert(ql("quick3") == R.OK)
  assert(fake.calls().load_index == 3, "quick3 is the third")
end)

test("a free slot is reported unavailable, loading nothing", function()
  fake.set_slot_free(3, Pool.NORMAL)
  assert(load("3") == R.FAILURE)
  assert(fake.calls().load_count == 0)
end)

test("a free quick slot loads nothing either", function()
  fake.set_slot_free(2, Pool.QUICK)
  assert(load("q2") == R.FAILURE)
  assert(fake.calls().load_count == 0)
end)

test("the quick pool's own size bounds a quick load", function()
  fake.set_slot_count(Pool.QUICK, 2)
  assert(load("q3") == R.FAILURE, "3 is past the two quick slots")
  assert(load("3") == R.OK, "but 3 is a valid normal slot")
end)

return h.report()
