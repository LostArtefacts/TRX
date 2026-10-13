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

local function is_tr1()
  return trx.game.tr_version == 1
end

-- The room between a dialog's frame and what it holds.
function M.outer_pad()
  return is_tr1() and 2 or 3
end

-- The room on each side of the body of a dialog.
function M.body_pad_x()
  return is_tr1() and 8 or 4
end

-- The room above and below a dialog's heading.
function M.title_pad_y()
  return is_tr1() and 2 or 1
end

-- The gap between two rows of a list.
function M.row_spacing()
  return is_tr1() and 2 or 3
end

-- Rounds a number to single precision. The engine lays its own dialogs out in
-- single precision, and a box that lands on half a pixel rounds one way or
-- the other depending on the precision it was worked out in, so a layout that
-- has to land where the engine's would rounds every step this way.
function M.f32(value)
  return (string.unpack("<f", string.pack("<f", value)))
end

return M
