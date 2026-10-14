-- The geometry of the game's dialogs, in canvas units at the default text
-- size. TR1 pads its dialogs differently from the later games.
--
-- Usage:
--   local metrics = require("common.ui.metrics")
--   local pad = metrics.outer_pad()

local M = {}

-- The height of one line of text.
M.TEXT_HEIGHT = 15

-- The height of the arrows that say a list goes on past its edge.
M.HINT_HEIGHT = 7

-- The room above and below the body of a dialog.
M.BODY_PAD_Y = 4

-- The room on each side of a dialog's heading, past the body's own.
M.TITLE_PAD_X = 10

-- The room on each side of a row of a short list of choices, such as a
-- question.
M.CHOICE_ROW_PAD = 20

-- The room between a dialog's frame and what it holds.
function M.outer_pad()
  return trx.game.tr_version == 1 and 2 or 3
end

-- The room on each side of the body of a dialog.
function M.body_pad_x()
  return trx.game.tr_version == 1 and 8 or 4
end

-- The room above and below a dialog's heading.
function M.title_pad_y()
  return trx.game.tr_version == 1 and 2 or 1
end

-- The height that a dialog's frame takes around its rows: its padding, its
-- heading where `title_spacing` gives the gap under one, and the scroll arrows
-- where it scrolls.
function M.chrome_height(title_spacing, scrolls)
  ---@type number
  local height = 2 * (M.outer_pad() + M.BODY_PAD_Y)
  if title_spacing ~= nil then
    height = height + M.TEXT_HEIGHT + 2 * M.title_pad_y() + title_spacing
  end
  if scrolls then
    height = height + 2 * M.HINT_HEIGHT
  end
  return height
end

-- The gap between two rows of a list.
function M.row_spacing()
  return trx.game.tr_version == 1 and 2 or 3
end

-- How far a rule between the rows of a list reaches past each side of it.
function M.rule_overhang()
  return trx.game.tr_version >= 2 and 7 or 10
end

-- How far the frame of the row under the cursor reaches above and below it.
function M.frame_overhang()
  return trx.game.tr_version == 1 and 1 or 0
end

return M
