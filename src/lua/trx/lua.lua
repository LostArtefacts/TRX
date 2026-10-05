local raw = trxc.lua
local h = require("trx.internal.helpers")

---@class trx
---@field lua trx.lua

---Evaluating Lua at runtime: a string of code, or a file on disk. Both run in
---the same state as every other script.
---@trx.module 37
---@class (exact) trx.lua
local M = h.module("lua")

-- The bridge reports a failure as two values; the public shape is one.
local function wrap(kind, message)
  if kind == nil then
    return nil
  end
  return { kind = kind, message = message }
end

---What went wrong while running Lua.
---@trx.record
---@class trx.lua.Error
---@field kind string Either `"syntax"` or `"runtime"`.
---@field message string The error text.

---Evaluates a string of Lua code, as the `/lua` console command does.
---
---```lua
---trx.lua.eval_expr("trx.console.log('hello')")
---```
---@param code string Any chunk of Lua, not only an expression.
---@return trx.lua.Error? # `nil` when the code ran to completion, and what went wrong otherwise. A failure comes back as a value rather than raising, so the caller decides what it means.
function M.eval_expr(code)
  return wrap(raw.eval_expr(code))
end

---Runs a Lua file, the way a level script is run. A file that cannot be read
---reports as a `"runtime"` failure.
---
---```lua
---trx.lua.eval_file("data/ship/scripts/extra.lua")
---```
---@param path string Path of the file.
---@return trx.lua.Error?
function M.eval_file(path)
  return wrap(raw.eval_file(path))
end
