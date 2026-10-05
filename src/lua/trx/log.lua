local raw = trxc.log
local h = require("trx.internal.helpers")

---@class trx
---@field log trx.log

---Logs a message to the terminal and to `TRX.log` in the installation
---directory. <!--noref: TRX.log--> Each call records the Lua script's
---filename, function name and line number.
---@trx.module 31 Logging
---@class (exact) trx.log
local M = h.module("log")

---Severity of a log message. Pass one to `trx.log.generic`.
---@enum trx.log.LogLevel
local LogLevel = {
  DEBUG = "Diagnostic detail, of interest while writing a script.",
  INFO = "Ordinary progress message.",
  WARNING = "Something is wrong, but the script can carry on.",
  ERROR = "Something failed.",
}
M.LogLevel = h.enum("log.LogLevel", "LOG_LEVEL", LogLevel)

local function at(level)
  return function(message)
    raw.log(level, message)
  end
end

---Logs a message at a level chosen at runtime, for when the level is computed
---rather than written literally.
---
---```lua
---local level = ok and trx.log.LogLevel.INFO or trx.log.LogLevel.ERROR
---trx.log.generic(level, "finished")
---```
---@param level trx.log.LogLevel
---@param message string The line to log.
function M.generic(level, message)
  raw.log(level, message)
end

---Logs an informational message.
---
---```lua
---trx.log.info("hello from lua")
---```
---@param message string The line to log.
---@type fun(message: string)
M.info = at(M.LogLevel.INFO)

---Logs a warning.
---@param message string The line to log.
---@type fun(message: string)
M.warn = at(M.LogLevel.WARNING)

---Logs a warning. An alias of `trx.log.warn`.
---@param message string The line to log.
---@type fun(message: string)
M.warning = at(M.LogLevel.WARNING)

---Logs an error.
---@param message string The line to log.
---@type fun(message: string)
M.error = at(M.LogLevel.ERROR)

---Logs a debug message.
---@param message string The line to log.
---@type fun(message: string)
M.debug = at(M.LogLevel.DEBUG)
