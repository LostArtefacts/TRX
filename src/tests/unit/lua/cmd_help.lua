-- The help command, driven through the console. What it prints for a command is
-- what that command prints for `--help`; the two are composed from one place,
-- and this pins them together.

local h = require("harness")
local test = h.test

test("help for a command matches that command's --help", function()
  trx.console.register({
    name = "documented",
    help = "test/plain",
    aliases = { "doc" },
    args = function(parser)
      parser:positional("who", { optional = true })
    end,
    run = function() end,
  })

  assert(fake.run("documented", "--help") == trx.console.Result.OK)
  local via_flag = fake.calls().last_message
  assert(fake.run("help", "documented") == trx.console.Result.OK)
  local via_help = fake.calls().last_message

  assert(via_help == via_flag, "help must print what --help prints")
  assert(via_help:find("Plain text", 1, true), "the description is shown")
  assert(via_help:find("Aliases: doc", 1, true), "the aliases are shown")
end)

test("an alias reaches the same help as the command", function()
  trx.console.register({
    name = "aliased",
    help = "test/plain",
    aliases = { "al" },
    run = function() end,
  })
  assert(fake.run("aliased", "--help") == trx.console.Result.OK)
  local direct = fake.calls().last_message
  assert(fake.run("help", "al") == trx.console.Result.OK)
  assert(fake.calls().last_message == direct, "an alias reaches the same help")
end)

test("help lists the documented commands", function()
  trx.console.register({
    name = "listed",
    help = "test/plain",
    run = function() end,
  })
  assert(fake.run("help", "") == trx.console.Result.OK)
  local names = fake.calls().last_message
  assert(names:find("listed", 1, true), "a documented command is listed")
  assert(names:find("help", 1, true), "help lists itself")
end)

test("a command with no help stays out of the listing", function()
  trx.console.register({ name = "secret_cmd", run = function() end })
  fake.run("help", "")
  assert(
    not fake.calls().last_message:find("secret_cmd", 1, true),
    "a command with no help is not listed"
  )
end)

test("help falls back to the description of a command written in C", function()
  -- Registered through the raw bridge, so it carries a help id but no parser,
  -- the way a command declared in C does.
  trxc.console.register("c_style", "test/plain", function() end)
  assert(fake.run("help", "c_style") == trx.console.Result.OK)
  assert(
    fake.calls().last_message == "Plain text",
    "the fallback prints the registry's description"
  )
end)

test("help rejects an unknown command", function()
  assert(fake.run("help", "nope") == trx.console.Result.FAILURE)
  assert(fake.calls().last_message == "Unknown command: nope")
  assert(fake.calls().last_level == trx.log.LogLevel.ERROR)
end)

test("help has no help for a command with none", function()
  -- die carries no help id, so the console cannot describe it.
  trx.console.register({ name = "undocumented", run = function() end })
  assert(fake.run("help", "undocumented") == trx.console.Result.FAILURE)
  assert(fake.calls().last_message == "Unknown command: undocumented")
end)

return h.report()
