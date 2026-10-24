-- How the menus style their text: the colour and the size that each game
-- gives a heading, the name of an entry, a count and the like. The text says
-- what it is, and this module says how it looks.
--
-- A colour is named as the text markup names it: `#rrggbb`, or the shade at
-- the top of a glyph and the one at its bottom, with `pulse` for one that
-- brightens and dims. In TR4, a channel of `80` draws a glyph as it is, as the
-- original lights its font.
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
  -- Text that names nothing else: what the game draws where text says
  -- nothing about its look.
  PLAIN = "plain",
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
  -- Text that stands back from what is around it, such as a time that is not
  -- the best one.
  FAINT = "faint",
  -- The numbers of the debug readouts over the game.
  READOUT = "readout",
  -- Text that the player cannot use, such as a control the game does not
  -- offer.
  DIM = "dim",
  -- The option the player is on, where no frame marks it.
  PICKED = "picked",
  -- The heading of a flat menu.
  MENU_HEADING = "menu_heading",
  -- What a flat menu points out: the values of its statistics and its arrows.
  MENU_ACCENT = "menu_accent",
  -- An option of a flat menu, or of the ring's choice of a weapon, that the
  -- player is not on.
  MENU_OPTION = "menu_option",
  -- The name of what a flat menu shows.
  MENU_NAME = "menu_name",
  -- A row of a flat list that the player is not on, and the names of the
  -- statistics.
  MENU_ROW = "menu_row",
  -- The heading of a flat menu that warns, as the game over screen does.
  MENU_WARNING = "menu_warning",
}

---@class common.ui.accent.Style
---@field color? string The colour, as the text markup names it.
---@field small? boolean Whether the text is drawn small.

-- The colours each game draws its menus in. TR1 and TR2 draw them flat, TR3
-- grades them from the top of a glyph to its bottom, and TR4 lights them as
-- its font does.
local PALETTES = {
  [1] = {
    plain = "#ffffff",
    yellow = "#b0b000",
    grey = "#a0a0a0",
    red = "#ff6060",
    gold = "#c08040",
    green = "#b6d164",
    white = "#ffffff",
    dim = "#808080",
  },
  [3] = {
    plain = "#ffffff #808080",
    yellow = "#b0b000 #505000",
    grey = "#a0a0a0 #181818",
    red = "#ff6060 #180000",
    gold = "#c08040 #401000",
    green = "#b6d164 #b62013",
    white = "#ffffff",
    dim = "#808080",
  },
  [4] = {
    plain = "#808080",
    pulse = "#808080 pulse",
    red = "#800000 #400000",
    silver = "#808080 #101010",
    orange = "#c08040 #401000",
    gold = "#e0c000 #402000",
    dim = "#404040",
  },
}
PALETTES[2] = PALETTES[1]

-- How each game styles each role, by the names of its palette. A role that a
-- game leaves out is plain.
---@type table<integer, table<string, table|fun(): table>>
local STYLES = {
  [1] = {
    count = { small = true },
    counter = { small = true },
    readout = { small = true },
    dim = { color = "dim" },
    picked = { color = "yellow" },
    menu_heading = { color = "green" },
    menu_accent = { color = "green" },
    menu_option = { color = "gold" },
    menu_name = { color = "white" },
    menu_row = { color = "grey" },
    menu_warning = { color = "red" },
  },
  [3] = {
    heading = { color = "gold" },
    name = { color = "gold" },
    count = { color = "red" },
    counter = function()
      if trx.config.get("ui.menu_style") == "ps1" then
        return { color = "red" }
      end
      return {}
    end,
    faint = { color = "grey" },
    readout = { small = true },
    dim = { color = "dim" },
    picked = { color = "yellow" },
    menu_heading = { color = "green" },
    menu_accent = { color = "green" },
    menu_option = { color = "gold" },
    menu_name = { color = "white" },
    menu_row = { color = "grey" },
    menu_warning = { color = "red" },
  },
  [4] = {
    value = { color = "orange" },
    name = { color = "gold" },
    count = { color = "gold", small = true },
    counter = { small = true },
    readout = { small = true },
    dim = { color = "dim" },
    picked = { color = "pulse" },
    menu_heading = { color = "orange" },
    menu_accent = { color = "orange" },
    menu_option = { color = "silver" },
    menu_name = { color = "gold" },
    menu_row = { color = "plain" },
    menu_warning = { color = "red" },
  },
}
STYLES[2] = STYLES[1]

---@return common.ui.accent.Style
local function style_of(role)
  local styles = STYLES[trx.game.version] or {}
  local style = styles[role] or {}
  if type(style) == "function" then
    style = style()
  end
  local palette = PALETTES[trx.game.version] or {}
  return {
    color = style.color ~= nil and palette[style.color] or nil,
    small = style.small,
  }
end

-- Returns the colour of a role, as the text markup names it, or nil for a
-- role that takes the colour of the text around it.
---@param role string One of `M.Role`.
---@return string?
function M.color_of(role)
  if role == M.Role.PLAIN then
    local palette = PALETTES[trx.game.version] or {}
    return palette.plain
  end
  return style_of(role).color
end

-- Returns the text in a colour, as the text markup names it.
---@param text string
---@param color string
---@return string
function M.color(text, color)
  return ("\\{color %s}%s"):format(color, text)
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
    result = M.color(result, style.color)
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

-- Gives the game the colours it draws text in where text names none: the
-- plain colour, and the one that marks the option the player is on where no
-- frame does. Called once, as the shipped scripts load.
function M.setup()
  trx.ui.text_color = M.color_of(M.Role.PLAIN)
  trx.ui.picked_text_color = M.color_of(M.Role.PICKED)
end

return M
