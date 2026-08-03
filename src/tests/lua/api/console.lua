-- The console API as a script actually sees it.
--
-- trx.console.log is an api.namespace: a table of functions that is itself
-- callable. This is where that shape meets a real bridge.

local h = require("harness")
local test, raises = h.test, h.raises

test("the log group is callable, and logs at INFO", function()
  trx.console.log("hello")
  local calls = fake.calls()
  assert(calls.log.count == 1, "calling the group did not reach the console")
  assert(calls.log.message == "hello")
  assert(
    calls.log.level == trx.log.LogLevel.INFO,
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
    assert(fake.calls().log.level == case.level, "wrong level")
  end
  assert(
    fake.calls().log.count == #cases,
    "not every level reached the console"
  )
end)

test("generic logs at the level it is handed", function()
  trx.console.log.generic(trx.log.LogLevel.ERROR, "boom")
  local calls = fake.calls()
  assert(calls.log.level == trx.log.LogLevel.ERROR)
  assert(calls.log.message == "boom")
end)

test("the log group carries no level enum of its own", function()
  -- LOG_LEVEL is declared once, as trx.log.LogLevel. A second copy hanging off
  -- the console would be a second thing to keep in step.
  assert(trx.console.log.LogLevel == nil)
end)

test("eval runs the command", function()
  trx.console.eval("play 1")
  local calls = fake.calls()
  assert(calls.eval.count == 1, "eval did not reach the console")
  assert(calls.eval.cmdline == "play 1")
end)

test("eval is quiet unless asked to be verbose", function()
  trx.console.eval("play 1")
  assert(fake.calls().eval.verbose == false, "eval must be quiet by default")

  trx.console.eval("play 1", { verbose = true })
  assert(
    fake.calls().eval.verbose == true,
    "verbose did not reach the console"
  )
end)

test("eval puts the verbose flag back the way it found it", function()
  trx.console.eval("play 1", { verbose = true })
  assert(fake.is_verbose() == false, "eval left the console verbose")
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
  assert(fake.calls().clear.count == 1)
end)

local function echo_arg(parser)
  parser:rest("text", { optional = true })
end

test("a registered command runs when the player types it", function()
  local seen = nil
  trx.console.register({
    name = "greet",
    args = echo_arg,
    run = function(args)
      seen = args.text
    end,
  })

  assert(
    fake.run("greet", "world") == trx.console.Result.OK,
    "a command that says nothing is OK"
  )
  assert(seen == "world", "the arguments did not reach the handler")
end)

test("an alias reaches the command it stands for", function()
  local seen = nil
  trx.console.register({
    name = "primary",
    aliases = { "secondary", "third" },
    args = echo_arg,
    run = function(args)
      seen = args.text
    end,
  })

  assert(fake.run("primary", "a") == trx.console.Result.OK)
  assert(seen == "a")
  assert(fake.run("secondary", "b") == trx.console.Result.OK, "alias must run")
  assert(seen == "b")
  assert(fake.run("third", "c") == trx.console.Result.OK)
  assert(seen == "c")
end)

test("the arguments are trimmed", function()
  local seen = nil
  trx.console.register({
    name = "trimmed",
    args = echo_arg,
    run = function(args)
      seen = args.text
    end,
  })
  fake.run("trimmed", "   spaced   ")
  assert(seen == "spaced", "the handler was handed untrimmed arguments")
end)

test("a command answers to any case the player typed", function()
  local seen = nil
  trx.console.register({
    name = "shout",
    args = echo_arg,
    run = function(args)
      seen = args.text
    end,
  })

  assert(fake.run("SHOUT", "loudly") == trx.console.Result.OK)
  assert(seen == "loudly", "the command did not run for an upper case name")
end)

test("a command says how it went", function()
  trx.console.register({
    name = "picky",
    args = echo_arg,
    run = function(args)
      if args.text == nil then
        return trx.console.Result.BAD_INVOCATION, "picky what?"
      end
      return trx.console.Result.FAILURE, "could not"
    end,
  })

  assert(fake.run("picky", "") == trx.console.Result.BAD_INVOCATION)
  assert(fake.calls().log.message == "picky what?")
  assert(fake.calls().log.level == trx.log.LogLevel.ERROR)

  assert(fake.run("picky", "x") == trx.console.Result.FAILURE)
end)

test("a command with no args declared refuses one", function()
  trx.console.register({
    name = "nullary",
    run = function() end,
  })
  assert(fake.run("nullary", "") == trx.console.Result.OK)
  assert(
    fake.run("nullary", "stray") == trx.console.Result.FAILURE,
    "an argument to an argument-less command is refused"
  )
end)

test("a message from a command that worked is not an error", function()
  trx.console.register({
    name = "chatty",
    run = function()
      return trx.console.Result.OK, "did it"
    end,
  })
  fake.run("chatty", "")
  assert(fake.calls().log.message == "did it")
  assert(fake.calls().log.level == trx.log.LogLevel.INFO, "OK is not an error")
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

test("logging a non-string coerces it", function()
  trx.console.log(1000)
  assert(fake.calls().log.message == "1000")

  trx.console.log(true)
  assert(fake.calls().log.message == "true")
end)

test("logging a table pretty-prints it", function()
  trx.console.log({ hp = 1000, name = "lara" })
  local message = fake.calls().log.message
  assert(message:find("hp = 1000"), "a field is shown: " .. message)
  assert(message:find('name = "lara"'), "a string value is quoted")
  assert(message:find("{"), "a table prints as a table")
end)

test("a self-referential table does not loop forever", function()
  local t = {}
  t.self = t
  trx.console.log(t)
  assert(fake.calls().log.message:find("<cycle>"), "a cycle is marked")
end)

test("p is a global alias of trx.console.log", function()
  assert(p == trx.console.log, "p must resolve to the log group")
  p("via p")
  assert(fake.calls().log.message == "via p")
end)

test("run receives its arguments already read", function()
  local seen
  trx.console.register({
    name = "with_parser",
    args = function(parser)
      parser:positional("id", { type = "integer" })
    end,
    run = function(args)
      seen = args.id
    end,
  })
  assert(fake.run("with_parser", "42") == trx.console.Result.OK)
  assert(seen == 42, "run saw the parsed number, not the string")
end)

test("a line the parser rejects never reaches run", function()
  local reached = false
  trx.console.register({
    name = "strict_parser",
    args = function(parser)
      parser:positional("id", { type = "integer" })
    end,
    run = function()
      reached = true
    end,
  })
  assert(fake.run("strict_parser", "nope") == trx.console.Result.FAILURE)
  assert(not reached, "run must not run on a bad line")
end)

test(
  "the console completes a command's arguments through its parser",
  function()
    trx.console.register({
      name = "completing",
      args = function(parser)
        parser:positional("state", { choices = { "snow", "rain", "none" } })
      end,
      run = function() end,
    })
    local all, at_empty = fake.complete_args("completing", "")
    assert(#all == 3, "an empty argument offers every choice")
    assert(at_empty == 0, "an empty argument replaces nothing, at the start")
    local one, at_word = fake.complete_args("completing", "sn")
    assert(one[1] == "snow", "a prefix narrows the candidates")
    assert(at_word == 0, "the run to replace begins where the argument does")
  end
)

test("an alias reaches the same argument completer", function()
  trx.console.register({
    name = "aliased_parser",
    aliases = { "ap" },
    args = function(parser)
      parser:positional("state", { choices = { "snow", "rain" } })
    end,
    run = function() end,
  })
  local out = fake.complete_args("ap", "ra")
  assert(out[1] == "rain", "the alias completes as the command would")
end)

test("commands reports a command's help and aliases", function()
  trx.console.register({
    name = "described",
    help = "test/plain",
    aliases = { "desc", "d" },
    run = function() end,
  })
  local found
  for _, cmd in ipairs(trx.console.commands()) do
    if cmd.name == "described" then
      found = cmd
    end
  end
  assert(found ~= nil, "the command is reported")
  assert(
    found.aliases[1] == "desc" and found.aliases[2] == "d",
    "its aliases come back as a list"
  )
  assert(
    found.help:find("Plain text", 1, true),
    "its help has the description"
  )
  assert(
    found.help:find("Aliases: desc, d", 1, true),
    "its help shows the aliases"
  )
end)

test("a command shows its aliases in its --help", function()
  trx.console.register({
    name = "aliased_help",
    help = "test/plain",
    aliases = { "ah", "a_h" },
    run = function() end,
  })
  assert(fake.run("aliased_help", "--help") == trx.console.Result.OK)
  assert(
    fake.calls().log.message:find("Aliases: ah, a_h", 1, true),
    "the aliases are shown"
  )
end)

test("commands leaves out what a bare command has none of", function()
  trx.console.register({ name = "bare", run = function() end })
  local found
  for _, cmd in ipairs(trx.console.commands()) do
    if cmd.name == "bare" then
      found = cmd
    end
  end
  assert(found ~= nil)
  assert(found.help == nil, "no help is reported for a command that has none")
  assert(
    found.aliases == nil,
    "no aliases are reported for a command with none"
  )
end)

return h.report()
