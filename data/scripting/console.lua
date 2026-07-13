local raw = trxc.console
local api = trx.api

-- The same LOG_LEVEL the logging module declares. One enum, one declaration:
-- trx.console.log.generic takes a trx.log.LogLevel.
local LogLevel = trxc.log.LogLevel

api.module("console", {
  order = 5,
  description = "Module for interacting with the developer console.\n\n"
    .. "`trx.console.log` writes to the console overlay in-game, where `trx.log` writes only to "
    .. "the terminal and the log file.",
})

-- The C side walks two stack frames up to find the caller, so every entry point
-- here keeps exactly one wrapper between the script and raw.log. Extra arguments
-- are concatenated with spaces by C; the contract is one message.
local function at(level)
  return function(message)
    raw.log(level, message)
  end
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
    { name = "command", type = "string", description = "Command to run, as the player would type it." },
    { name = "opts", type = "table", optional = true, description = "`verbose`: show the command's output." },
  },
  examples = { [[trx.console.eval("play 1", { verbose = true })]] },
  impl = raw.eval,
})

api.define("console.clear", {
  description = "Clears the console.",
  impl = raw.clear,
})
