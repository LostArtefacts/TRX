local raw = trxc.console
local api = trx.api

-- trx.console.log.generic takes a trx.log.LogLevel, and the log module is what
-- declares it.
local LogLevel = trx.log.LogLevel

api.module("console", {
  order = 5,
  description = "Module for interacting with the developer console.\n\n"
    .. "`trx.console.log` writes to the console overlay in-game, where `trx.log` writes only to "
    .. "the terminal and the log file.",
})

local function at(level)
  return function(message)
    raw.log(level, message)
  end
end

local Result = api.enum("console.Result", {
  backing = "COMMAND_RESULT",
  description = "How a console command went. What a command's `run` gives back.",
  values = {
    OK = "It worked.",
    FAILURE = "It ran and could not do what was asked.",
    UNAVAILABLE = "It cannot run here - no level is loaded, or the game is in a menu.",
    BAD_INVOCATION = "The player typed it wrong.",
  },
})

local is_result = {}
for _, value in pairs(Result) do
  is_result[value] = true
end

api.namespace("console.log", {
  description = "Logs a line to the developer console. Calling the group itself logs at `INFO`.",
  params = { { name = "message", type = "string" } },
  examples = { [[trx.console.log("hello")]] },
  call = at(LogLevel.INFO),
})

api.define("console.log.generic", {
  description = "Logs at a level chosen at runtime.",
  params = {
    { name = "level", type = "integer", enum = "log.LogLevel" },
    { name = "message", type = "string" },
  },
  impl = function(level, message)
    raw.log(level, message)
  end,
})

api.define("console.log.info", {
  description = "Logs an informational message.",
  params = { { name = "message", type = "string" } },
  impl = at(LogLevel.INFO),
})

api.define("console.log.warn", {
  description = "Logs a warning.",
  params = { { name = "message", type = "string" } },
  impl = at(LogLevel.WARNING),
})

api.define("console.log.warning", {
  description = "Logs a warning. An alias of `trx.console.log.warn`.",
  params = { { name = "message", type = "string" } },
  impl = at(LogLevel.WARNING),
})

api.define("console.log.error", {
  description = "Logs an error.",
  params = { { name = "message", type = "string" } },
  impl = at(LogLevel.ERROR),
})

api.define("console.log.debug", {
  description = "Logs a debug message.",
  params = { { name = "message", type = "string" } },
  impl = at(LogLevel.DEBUG),
})

api.define("console.eval", {
  description = "Runs a string as a developer console command. Raises if the command fails.\n\n"
    .. "Output is silenced by default and appears only in the terminal and the log file. Pass "
    .. "`{ verbose = true }` to show it in the console as a command typed by the player would.",
  params = {
    {
      name = "command",
      type = "string",
      description = "Command to run, as the player would type it.",
    },
    {
      name = "opts",
      type = "table",
      optional = true,
      description = "`verbose`: show the command's output.",
    },
  },
  examples = { [[trx.console.eval("play 1", { verbose = true })]] },
  impl = raw.eval,
})

api.define("console.register", {
  description = "Registers a console command written in Lua.\n\n"
    .. "`run` is called with whatever the player typed after the command word, trimmed. What it "
    .. "gives back is a `trx.console.Result`, and returning nothing means `OK`. It may return a "
    .. "message after that, which is logged to the console - as an error, for any result but `OK`.\n\n"
    .. "A command lives for the whole run, so it can only be registered from a global script. A "
    .. "level script raises if it calls this: it runs again every time its level is loaded.",
  params = {
    {
      name = "spec",
      type = "table",
      description = "`name`: the word the player types. `help`: a game string key for the help "
        .. "text, optional. `run`: the function.",
    },
  },
  examples = {
    [[trx.console.register({
  name = "greet",
  run = function(args)
    if args == "" then
      return trx.console.Result.BAD_INVOCATION, "greet who?"
    end
    trx.console.log("hello " .. args)
  end,
})]],
  },
  impl = function(spec)
    assert(type(spec) == "table", "trx.console.register expects a table")
    assert(
      type(spec.name) == "string",
      "trx.console.register: name must be a string"
    )
    assert(
      spec.help == nil or type(spec.help) == "string",
      "trx.console.register: help must be a string"
    )
    assert(
      type(spec.run) == "function",
      "trx.console.register: run must be a function"
    )

    raw.register(spec.name, spec.help, function(args)
      local result, message = spec.run((args or ""):match("^%s*(.-)%s*$"))
      -- Only returning nothing means OK; `or` would take a `false` for it too.
      if result == nil then
        result = Result.OK
      end
      assert(
        is_result[result],
        spec.name .. ": run must give back a trx.console.Result"
      )
      if message ~= nil then
        if result == Result.OK then
          trx.console.log.info(message)
        else
          trx.console.log.error(message)
        end
      end
      return result
    end)
  end,
})

api.define("console.clear", {
  description = "Clears the console.",
  impl = raw.clear,
})
