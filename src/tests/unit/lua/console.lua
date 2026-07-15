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
  assert(
    calls.last_level == trx.log.LogLevel.INFO,
    "calling the group must log at INFO"
  )
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
  assert(
    fake.calls().log_count == #cases,
    "not every level reached the console"
  )
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
  assert(
    fake.calls().verbose_during_eval == false,
    "eval must be quiet by default"
  )

  trx.console.eval("play 1", { verbose = true })
  assert(
    fake.calls().verbose_during_eval == true,
    "verbose did not reach the console"
  )
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

  fake.set_eval_result(fake.CommandResult.UNAVAILABLE)
  raises(function()
    trx.console.eval("heal")
  end, "unavailable")
end)

test("clear clears the console", function()
  trx.console.clear()
  assert(fake.calls().clear_count == 1)
end)

test("a registered command runs when the player types it", function()
  local seen = nil
  trx.console.register({
    name = "greet",
    run = function(args)
      seen = args
    end,
  })

  assert(
    fake.run("greet", "world") == trx.console.Result.OK,
    "a command that says nothing is OK"
  )
  assert(seen == "world", "the arguments did not reach the handler")
end)

test("the arguments are trimmed", function()
  local seen = nil
  trx.console.register({
    name = "trimmed",
    run = function(args)
      seen = args
    end,
  })
  fake.run("trimmed", "   spaced   ")
  assert(seen == "spaced", "the handler was handed untrimmed arguments")
end)

test("a command answers to whatever case the player typed", function()
  local seen = nil
  trx.console.register({
    name = "shout",
    run = function(args)
      seen = args
    end,
  })

  assert(fake.run("SHOUT", "loudly") == trx.console.Result.OK)
  assert(seen == "loudly", "the command did not run for an upper case name")
end)

test("a command says how it went", function()
  trx.console.register({
    name = "picky",
    run = function(args)
      if args == "" then
        return trx.console.Result.BAD_INVOCATION, "picky what?"
      end
      return trx.console.Result.FAILURE, "could not"
    end,
  })

  assert(fake.run("picky", "") == trx.console.Result.BAD_INVOCATION)
  assert(fake.calls().last_message == "picky what?")
  assert(fake.calls().last_level == trx.log.LogLevel.ERROR)

  assert(fake.run("picky", "x") == trx.console.Result.FAILURE)
end)

test("a message from a command that worked is not an error", function()
  trx.console.register({
    name = "chatty",
    run = function()
      return trx.console.Result.OK, "did it"
    end,
  })
  fake.run("chatty", "")
  assert(fake.calls().last_message == "did it")
  assert(
    fake.calls().last_level == trx.log.LogLevel.INFO,
    "OK is not an error"
  )
end)

test("a command that raises is a failure, not a crash", function()
  trx.console.register({
    name = "broken",
    run = function()
      error("boom")
    end,
  })
  assert(fake.run("broken", "") == trx.console.Result.FAILURE)
end)

test("a result that is not a Result is a failure", function()
  trx.console.register({
    name = "confused",
    run = function()
      return "failure"
    end,
  })
  assert(fake.run("confused", "") == trx.console.Result.FAILURE)
end)

-- The bridge takes the handler's answer straight from Lua, and lua_tointeger
-- reads a table as 0, which is OK. Reached here through the raw bridge, past
-- the check trx.console.register makes.
test("the bridge refuses an answer that is not a result", function()
  trxc.console.register("raw_table", nil, function()
    return {}
  end)
  assert(fake.run("raw_table", "") == trx.console.Result.FAILURE)

  trxc.console.register("raw_number", nil, function()
    return 99
  end)
  assert(fake.run("raw_number", "") == trx.console.Result.FAILURE)
end)

test("false is not a way of saying OK", function()
  trx.console.register({
    name = "denier",
    run = function()
      return false
    end,
  })
  assert(fake.run("denier", "") == trx.console.Result.FAILURE)
end)

test("a command carries its help string", function()
  trx.console.register({
    name = "helpful",
    help = "console/cmd/helpful/help",
    run = function() end,
  })
  assert(fake.help_id("helpful") == "console/cmd/helpful/help")
end)

test("a name the console could not dispatch is refused", function()
  raises(function()
    trx.console.register({ name = "two words", run = function() end })
  end)
  raises(function()
    trx.console.register({ name = "", run = function() end })
  end)
end)

test("a command cannot be registered twice", function()
  trx.console.register({ name = "once", run = function() end })
  raises(function()
    trx.console.register({ name = "once", run = function() end })
  end)
end)

-- A global script runs again after a mod switch, so the command it registered
-- last time must be gone by then, or the second run raises on it.
test("a command can be registered again after a reload", function()
  trx.console.register({ name = "twice", run = function() end })
  fake.reload()
  assert(not fake.is_registered("twice"), "the reload left the command behind")
  trx.console.register({ name = "twice", run = function() end })
  assert(fake.is_registered("twice"))
end)

-- A level script runs again every time its level is loaded, so a command
-- registered from one would work once and then raise on the reload, taking the
-- rest of the script with it.
test("a level script cannot register a command", function()
  raises(function()
    fake.as_level_script(function()
      trx.console.register({ name = "leveller", run = function() end })
    end)
  end, "level script")
  assert(
    not fake.is_registered("leveller"),
    "the command was registered anyway"
  )
end)

test("register wants a name and a function", function()
  raises(function()
    trx.console.register({ run = function() end })
  end)
  raises(function()
    trx.console.register({ name = "nofn" })
  end)
end)

test("the raw bridge is not part of the surface", function()
  -- trxc.console.log is a plain function taking a level. The declaration turns
  -- it into a namespace, and the raw entry point must not leak alongside it.
  assert(
    type(trx.console.log) == "table",
    "log must be the namespace, not the raw function"
  )
end)

return h.report()
