-- The accent colour of the menus: the orange that the original TR4 prints its
-- headings and the values of its options in. The other games keep their text
-- as it is.
--
-- Usage:
--   local accent = require("common.ui.accent")
--   trx.ui.widgets.Label({ text = accent.text("On") })
--   window.new({ title = accent.signal(title_signal) })

local value = require("common.ui.value")

local M = {}

-- The index of the orange in the colour markup of the TR4 font.
local TR4_ORANGE = 6

-- Returns the text in the accent colour.
---@param text string
---@return string
function M.text(text)
  if trx.game.version ~= 4 then
    return text
  end
  return ("\\{color %d}%s"):format(TR4_ORANGE, text)
end

-- Returns a signal that holds the text in the accent colour, as the text or
-- the signal that holds it changes.
---@param text string|trx.signal.Signal
---@return trx.signal.Signal
function M.signal(text)
  return value.signal(text):map(M.text)
end

return M
