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

return M
