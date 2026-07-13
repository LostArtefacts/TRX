-- The console API as a script actually sees it.
--
-- trx.console.log is an api.namespace: a table of functions that is itself
-- callable. This is where that shape meets a real bridge.

local h = require("harness")
local test, raises = h.test, h.raises

test("the log group is callable, and logs at INFO", function()
  trx.console.log("hello")
  local calls = fake.calls()
  assert(calls.log_count == 1, "calling the group did not reach the console")
  assert(calls.last_message == "hello")
  assert(calls.last_level == trx.log.LogLevel.INFO, "calling the group must log at INFO")
end)

test("each level logs at its own level", function()
  local cases = {
    { fn = trx.console.log.info, level = trx.log.LogLevel.INFO },
    { fn = trx.console.log.warn, level = trx.log.LogLevel.WARNING },
    { fn = trx.console.log.warning, level = trx.log.LogLevel.WARNING },
    { fn = trx.console.log.error, level = trx.log.LogLevel.ERROR },
    { fn = trx.console.log.debug, level = trx.log.LogLevel.DEBUG },
  }
  for _, case in ipairs(cases) do
    case.fn("msg")
    assert(fake.calls().last_level == case.level, "wrong level")
  end
  assert(fake.calls().log_count == #cases, "not every level reached the console")
end)

test("generic logs at the level it is handed", function()
  trx.console.log.generic(trx.log.LogLevel.ERROR, "boom")
  local calls = fake.calls()
  assert(calls.last_level == trx.log.LogLevel.ERROR)
  assert(calls.last_message == "boom")
end)

test("the log group carries no level enum of its own", function()
  -- LOG_LEVEL is declared once, as trx.log.LogLevel. A second copy hanging off
  -- the console would be a second thing to keep in step.
  assert(trx.console.log.LogLevel == nil)
end)

test("eval runs the command", function()
  trx.console.eval("play 1")
  local calls = fake.calls()
  assert(calls.eval_count == 1, "eval did not reach the console")
  assert(calls.last_command == "play 1")
end)

test("eval is quiet unless asked to be verbose", function()
  trx.console.eval("play 1")
  assert(fake.calls().verbose_during_eval == false, "eval must be quiet by default")

  trx.console.eval("play 1", { verbose = true })
  assert(fake.calls().verbose_during_eval == true, "verbose did not reach the console")
end)

test("eval puts the verbose flag back the way it found it", function()
  trx.console.eval("play 1", { verbose = true })
  assert(fake.calls().verbose_now == false, "eval left the console verbose")
end)

test("a command that fails raises, and says why", function()
  fake.set_eval_result(fake.CommandResult.FAILURE)
  raises(function()
    trx.console.eval("kill everything")
  end, "failure")

  fake.set_eval_result(fake.CommandResult.BAD_INVOCATION)
  raises(function()
    trx.console.eval("kill")
  end, "bad invocation")
end)

test("clear clears the console", function()
  trx.console.clear()
  assert(fake.calls().clear_count == 1)
end)

test("the raw bridge is not part of the surface", function()
  -- trxc.console.log is a plain function taking a level. The declaration turns
  -- it into a namespace, and the raw entry point must not leak alongside it.
  assert(type(trx.console.log) == "table", "log must be the namespace, not the raw function")
end)

return h.report()
