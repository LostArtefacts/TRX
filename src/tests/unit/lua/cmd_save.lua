-- /save and /qs, dispatched through the console. A level has to be loaded to
-- save, so each test sets one; the save system fake records where the save went.
-- The slot spelling is read by a match matcher: a number, `q`, `q2`, `quick`.

local h = require("harness")
local test = h.test
local R = trx.console.Result
local Pool = trx.savegame.Pool

local function save(a)
  fake.set_current_level(1)
  return fake.run("save", a or "")
end

test("a number saves to that normal slot", function()
  assert(save("3") == R.OK)
  local c = fake.calls()
  assert(c.save_index == 3 and c.save_pool == Pool.NORMAL)
end)

test("q saves to the next quick slot", function()
  assert(save("q") == R.OK)
  local c = fake.calls()
  assert(c.save_index == -1, "no index means the next quick slot")
  assert(c.save_pool == Pool.QUICK)
end)

test("q2 saves to the second quick slot", function()
  assert(save("q2") == R.OK)
  local c = fake.calls()
  assert(c.save_index == 2 and c.save_pool == Pool.QUICK)
end)

test("quick, spelled out, reaches the quick pool", function()
  assert(save("quick") == R.OK)
  assert(fake.calls().save_pool == Pool.QUICK)
end)

test("an out-of-range slot is a bad invocation", function()
  assert(save("999") == R.BAD_INVOCATION)
  assert(fake.calls().save_count == 0)
end)

test("nonsense is refused before run", function()
  assert(save("blah") == R.FAILURE)
  assert(fake.calls().save_count == 0)
end)

test("quicksave with no argument saves the next quick slot", function()
  fake.set_current_level(1)
  assert(fake.run("qs", "") == R.OK)
  local c = fake.calls()
  assert(c.save_index == -1 and c.save_pool == Pool.QUICK)
end)

test("saving needs a level", function()
  fake.set_current_level(nil)
  assert(fake.run("save", "1") == R.UNAVAILABLE)
  assert(fake.calls().save_count == 0)
end)

test("a quick save with no free slot reports the failure", function()
  fake.set_save_fails(true)
  assert(save("q") == R.FAILURE)
end)

test("an out-of-range quick slot is a bad invocation", function()
  assert(save("q999") == R.BAD_INVOCATION)
  assert(fake.calls().save_count == 0)
end)

test("quick slot zero is a bad invocation", function()
  assert(save("q0") == R.BAD_INVOCATION)
  assert(fake.calls().save_count == 0)
end)

return h.report()
