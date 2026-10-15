-- A setting that a shipped widget takes as a value or as a signal that holds
-- one, as the engine's own widgets take theirs.
--
-- Usage:
--   local value = require("common.ui.value")
--   local now = value.read(settings.color)
--   local hidden = ~value.signal(settings.left)

local M = {}

local function is_signal(candidate)
  return type(candidate) == "table" and candidate.map ~= nil
end

-- What the setting holds now.
---@param setting any A value, or a signal that holds one.
---@return any
function M.read(setting)
  if is_signal(setting) then
    return setting:get()
  end
  return setting
end

-- The setting as a signal, which a plain value becomes one that never moves.
---@param setting any A value, or a signal that holds one.
---@return trx.signal.Signal
function M.signal(setting)
  if is_signal(setting) then
    return setting
  end
  return trx.signal.new(setting)
end

-- A signal that holds what the signal at `index` holds, out of `signals`.
---@param index trx.signal.Signal Which signal, counted from 1.
---@param signals trx.signal.Signal[]
---@return trx.signal.Signal
function M.pick(index, signals)
  ---@type any[]
  local args = { index }
  for _, signal in ipairs(signals) do
    args[#args + 1] = signal
  end
  args[#args + 1] = function(at, ...)
    return (select(at, ...))
  end
  return trx.signal.combine(table.unpack(args))
end

-- A signal of its own that follows `source` until the listener is detached.
-- What is built on the copy stops following `source` with it.
---@param source trx.signal.Signal
---@return trx.signal.Signal copy
---@return trx.signal.Listener listener
function M.mirror(source)
  local copy = trx.signal.new(source:get())
  return copy, source:on(function(now)
    copy:set(now)
  end)
end

return M
