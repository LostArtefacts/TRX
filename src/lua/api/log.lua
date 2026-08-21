local raw = trxc.log
local api = trx.api

api.module("log", {
  order = 30,
  title = "Logging",
  description = "Logs a message to the terminal and to `TRX.log` in the installation directory. "
    .. "<!--noref: TRX.log--> "
    .. "Each call records the Lua script's filename, function name and line number.",
})

local LogLevel = api.enum("log.LogLevel", {
  backing = "LOG_LEVEL",
  description = "Severity of a log message. Pass one to `trx.log.generic`.",
  values = {
    DEBUG = "Diagnostic detail, of interest while writing a script.",
    INFO = "Ordinary progress message.",
    WARNING = "Something is wrong, but the script can carry on.",
    ERROR = "Something failed.",
  },
})

local function at(level)
  return function(message)
    raw.log(level, message)
  end
end

api.define("log.generic", {
  description = "Logs a message at a level chosen at runtime, for when the level is computed "
    .. "rather than written literally.",
  params = {
    { name = "level", type = "log.LogLevel" },
    { name = "message", type = "string", description = "The line to log." },
  },
  examples = {
    [[local level = ok and trx.log.LogLevel.INFO or trx.log.LogLevel.ERROR
trx.log.generic(level, "finished")]],
  },
  impl = function(level, message)
    raw.log(level, message)
  end,
})

api.define("log.info", {
  description = "Logs an informational message.",
  params = {
    { name = "message", type = "string", description = "The line to log." },
  },
  examples = { [[trx.log.info("hello from lua")]] },
  impl = at(LogLevel.INFO),
})

api.define("log.warn", {
  description = "Logs a warning.",
  params = {
    { name = "message", type = "string", description = "The line to log." },
  },
  impl = at(LogLevel.WARNING),
})

api.define("log.warning", {
  description = "Logs a warning. An alias of `trx.log.warn`.",
  params = {
    { name = "message", type = "string", description = "The line to log." },
  },
  impl = at(LogLevel.WARNING),
})

api.define("log.error", {
  description = "Logs an error.",
  params = {
    { name = "message", type = "string", description = "The line to log." },
  },
  impl = at(LogLevel.ERROR),
})

api.define("log.debug", {
  description = "Logs a debug message.",
  params = {
    { name = "message", type = "string", description = "The line to log." },
  },
  impl = at(LogLevel.DEBUG),
})
