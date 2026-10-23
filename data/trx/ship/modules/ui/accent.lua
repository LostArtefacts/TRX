-- How the menus style their text: the colour and the size that each game
-- gives a heading, the name of an entry, a count and the like. The text says
-- what it is, and this module says how it looks.
--
-- Usage:
--   local accent = require("common.ui.accent")
--   trx.ui.widgets.Label({ text = accent.text("On") })
--   ring.set_heading(accent.text(title, accent.Role.HEADING))
--   window.new({ title = accent.signal(title_signal) })

local value = require("common.ui.value")

local M = {}

-- What a piece of text is to the menus.
M.Role = {
  -- The value of an option, and the headings of the option dialogs.
  VALUE = "value",
  -- The heading over the inventory ring and the globe.
  HEADING = "heading",
  -- The name of an entry, as the caption under the ring shows it.
  NAME = "name",
  -- How many there are of an entry on the ring.
  COUNT = "count",
  -- The ammunition count over the game.
  COUNTER = "counter",
}

---@class common.ui.accent.Style
---@field color? integer The index of the colour in the font's colour markup.
---@field small? boolean Whether the text is drawn small.

-- How each game styles each role. A role that a game leaves out is plain.
---@type table<integer, table<string, common.ui.accent.Style|fun(): common.ui.accent.Style>>
local STYLES = {
  [1] = {
    count = { small = true },
    counter = { small = true },
  },
  [2] = {
    count = { small = true },
    counter = { small = true },
  },
  [3] = {
    heading = { color = 5 },
    name = { color = 5 },
    count = { color = 3 },
    counter = function()
      if trx.config.get("ui.menu_style") == "ps1" then
        return { color = 3 }
      end
      return {}
    end,
  },
  [4] = {
    value = { color = 6 },
    name = { color = 8 },
    count = { small = true },
    counter = { small = true },
  },
}

local function style_of(role)
  local styles = STYLES[trx.game.version] or {}
  local style = styles[role] or {}
  if type(style) == "function" then
    return style()
  end
  return style
end

-- Returns the text styled as the role says, the value of an option by
-- default.
---@param text string|number
---@param role? string One of `M.Role`.
---@return string
function M.text(text, role)
  local style = style_of(role or M.Role.VALUE)
  local result = tostring(text)
  if style.color ~= nil then
    result = ("\\{color %d}%s"):format(style.color, result)
  end
  if style.small then
    result = "\\{small}" .. result
  end
  return result
end

-- Returns a signal that holds the text styled as the role says, as the text
-- or the signal that holds it changes.
---@param text string|trx.signal.Signal
---@param role? string One of `M.Role`.
---@return trx.signal.Signal
function M.signal(text, role)
  return value.signal(text):map(function(t)
    return M.text(t, role)
  end)
end

return M
