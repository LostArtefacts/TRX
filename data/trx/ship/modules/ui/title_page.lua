-- Whether the title screen shows its main menu, rather than a list or a dialog
-- that one of its choices opens. The flat menu and the ring both report it,
-- and it holds true while no title menu is open. The original TR4 hides the
-- logo of the title screen everywhere but on the main menu.
--
-- Usage:
--   local title_page = require("common.ui.title_page")
--   local logo = trx.ui.widgets.Image({ visible = title_page.shows_main })

local M = {}

M.shows_main = trx.signal.new(true)

return M
