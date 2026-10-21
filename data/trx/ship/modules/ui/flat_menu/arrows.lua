-- The arrows that say a list of the flat menu system goes on past the room it
-- has, as the original TR4 prints them beside its list of levels.

local TEXT_SCALE = require("common.ui.flat_menu.text").SCALE

local M = {}

local COLOR = 6
-- Where the original prints the arrows, across its 4:3 screen of 640 pixels.
local LEFT_X = 32
local RIGHT_X = 592

M.UP = "\\{button up}"
M.DOWN = "\\{button down}"

local function colored(glyph)
  return ("\\{color %d}%s"):format(COLOR, glyph)
end

-- Returns the x of a point the original gives across its 640-pixel screen.
local function across(x)
  local h = trx.ui.canvas.height
  return trx.ui.canvas.width / 2 + (x - 320) * h * 4 / 3 / 640
end

-- Draws an arrow at each side of the screen, on the line that starts at `y`.
---@param glyph string
---@param y number
function M.beside(glyph, y)
  local text = colored(glyph)
  trx.ui.primitive.text(text, across(LEFT_X), y, TEXT_SCALE)
  trx.ui.primitive.text(text, across(RIGHT_X), y, TEXT_SCALE)
end

-- Draws an arrow in the middle of the screen, on the line that starts at `y`.
---@param glyph string
---@param y number
function M.centred(glyph, y)
  local text = colored(glyph)
  local w = trx.ui.primitive.measure_text(text, TEXT_SCALE)
  trx.ui.primitive.text(text, (trx.ui.canvas.width - w) / 2, y, TEXT_SCALE)
end

return M
