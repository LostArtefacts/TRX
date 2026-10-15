-- A widget between the two arrows that say it can be stepped left and right.
-- An arrow that does not apply keeps its room, so the widget does not move as
-- the arrows come and go.
--
-- Usage:
--   local row_arrows = require("common.ui.widgets.row_arrows")
--   local arrows = row_arrows.new({ child = label, left = can_left })

local value = require("common.ui.value")

local M = {}

-- The gaps between the arrows and the widget.
M.Spacing = {
  TIGHT = 2,
  MEDIUM = 5,
  WIDE = 15,
}

---@class common.ui.RowArrows.settings
---@field child trx.ui.Widget The widget between the arrows.
---@field left? any Whether to show the left arrow, or a signal that holds it. `false` by default.
---@field right? any Whether to show the right arrow, or a signal that holds it. `false` by default.
---@field spacing? number The gap on each side of the widget. `common.ui.widgets.row_arrows.Spacing.MEDIUM` by default.

-- Creates the arrows around a widget.
---@param settings common.ui.RowArrows.settings
---@return trx.ui.Widget
function M.new(settings)
  return trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    align = trx.ui.HAlign.DISTRIBUTE,
    v_align = trx.ui.VAlign.CENTER,
    spacing = settings.spacing or M.Spacing.MEDIUM,
    children = {
      trx.ui.widgets.Label({
        text = "\\{button left}",
        hidden = ~value.signal(settings.left or false),
      }),
      settings.child,
      trx.ui.widgets.Label({
        text = "\\{button right}",
        hidden = ~value.signal(settings.right or false),
      }),
    },
  })
end

return M
