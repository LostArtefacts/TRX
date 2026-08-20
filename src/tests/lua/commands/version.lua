local h = require("harness")
local test = h.test

test("version reports what the build calls itself", function()
  assert(fake.run("version", "") == trx.console.Result.OK)
  assert(fake.calls().log.message == trx.game.trx_version)
end)

test("version answers away from a level", function()
  fake.set_current_level(nil)
  assert(fake.run("version", "") == trx.console.Result.OK)
end)

return h.report()
