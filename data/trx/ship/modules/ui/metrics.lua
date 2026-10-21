-- The geometry of the game's dialogs, in canvas units at the default text
-- size. TR1 pads its dialogs differently from the later games.
--
-- Usage:
--   local metrics = require("common.ui.metrics")
--   local pad = metrics.outer_pad()

local M = {}

-- The height of the arrows that say a list goes on past its edge.
M.HINT_HEIGHT = 7

-- The most rows of settings a dialog shows at once.
M.MAX_DIALOG_ROWS = 16

-- The room on each side of a row of a short list of choices, such as a
-- question.
M.CHOICE_ROW_PAD = 20

-- The widths measured so far, for the text scale they were measured at. The
-- dialogs measure the same few texts many times over, such as on and off.
local widths = {}
local widths_scale = nil

-- How wide a text is, in canvas units at the default text size.
function M.text_width(text)
  local scale = trx.ui.text_scale
  if scale ~= widths_scale then
    widths = {}
    widths_scale = scale
  end
  local width = widths[text]
  if width == nil then
    width = trx.ui.primitive.measure_text(text, 1.0) / scale
    widths[text] = width
  end
  return width
end

-- The paddings below keep text clear of a dialog's frame, so a dialog drawn
-- without frames takes none of them.

-- The room between a dialog's frame and what it holds.
function M.outer_pad()
  if not trx.ui.frames_shown then
    return 0
  end
  return trx.game.tr_version == 1 and 2 or 3
end

-- The room on each side of the body of a dialog.
function M.body_pad_x()
  if not trx.ui.frames_shown then
    return 0
  end
  return trx.game.tr_version == 1 and 8 or 4
end

-- The room above and below the body of a dialog.
function M.body_pad_y()
  return trx.ui.frames_shown and 4 or 0
end

-- The room on each side of a dialog's heading, past the body's own.
function M.title_pad_x()
  return trx.ui.frames_shown and 10 or 0
end

-- The room above and below a dialog's heading.
function M.title_pad_y()
  if not trx.ui.frames_shown then
    return 0
  end
  return trx.game.tr_version == 1 and 2 or 1
end

-- The height that a dialog's frame takes around its rows: its padding, its
-- heading where `title_spacing` gives the gap under one, and the scroll arrows
-- where it scrolls.
function M.chrome_height(title_spacing, scrolls)
  ---@type number
  local height = 2 * (M.outer_pad() + M.body_pad_y())
  if title_spacing ~= nil then
    height = height + M.line_height() + 2 * M.title_pad_y() + title_spacing
  end
  if scrolls then
    height = height + 2 * M.HINT_HEIGHT
  end
  return height
end

-- The height that a line of text takes as the font measures it, in canvas
-- units at the default text size.
function M.line_height()
  local _, h = trx.ui.primitive.measure_text("0", 1.0)
  return h / trx.ui.text_scale
end

-- The distance between the lines of a passage of text, in canvas units at the
-- default text size. In TR4, it is shorter than the height of one line.
function M.line_pitch()
  local _, h = trx.ui.primitive.measure_text("0\n0", 1.0)
  return h / trx.ui.text_scale - M.line_height()
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
