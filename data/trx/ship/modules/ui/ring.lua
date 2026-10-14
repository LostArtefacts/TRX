-- What the screens of an inventory ring entry share: the entry the player
-- picked comes to rest before its screen shows or reads any input.
--
-- Usage:
--   local ring = require("common.ui.ring")
--   local resting = ring.resting()
--   ctx:on_end(function() resting:stop() end)
--   ctx:push({ root = ..., on_input = ring.while_resting(control) })

local M = {}

local function is_resting()
  return trx.inventory_ring.signals.rest_frame:get() ~= nil
end

-- Returns a signal that holds whether the entry rests open.
function M.resting()
  return trx.inventory_ring.signals.rest_frame:map(function(frame)
    return frame ~= nil
  end)
end

-- Returns an input handler that runs `control` only while the entry rests
-- open, and reads no input while the entry moves.
function M.while_resting(control)
  return function(layer, keys)
    if is_resting() then
      control(layer, keys)
    end
  end
end

return M
