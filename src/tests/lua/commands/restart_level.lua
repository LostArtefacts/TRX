-- /restartlevel, dispatched through the console. The fake flow has Caves at
-- ordinal 1.

local h = require("harness")
local test = h.test
local R = trx.console.Result

test("restarts the current level", function()
  fake.set_current_level(2)
  assert(fake.run("restartlevel", "") == R.OK)
  local c = fake.calls()
  assert(c.restart_level.count == 1)
  assert(c.restart_level.num == 1)
end)

test("restart is an alias", function()
  fake.set_current_level(2)
  assert(fake.run("restart", "") == R.OK)
  assert(fake.calls().restart_level.count == 1)
end)

test("is unavailable outside a level", function()
  assert(fake.run("restartlevel", "") == R.UNAVAILABLE)
  assert(fake.calls().restart_level.count == 0)
end)

test("is unavailable when the save cannot restart", function()
  fake.set_current_level(2)
  fake.set_restart_available(false)
  assert(fake.run("restartlevel", "") == R.UNAVAILABLE)
  assert(fake.calls().restart_level.count == 0)
end)

return h.report()
