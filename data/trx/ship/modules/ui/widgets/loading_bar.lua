-- The loading bar that a loading screen shows at its foot, filled by a value
-- that moves, such as a tween, and shown while the player's loading bar
-- setting is on.
--
-- Usage:
--   local loading_bar = require("common.ui.widgets.loading_bar")
--   local bar = loading_bar.new({ progress = tween, width = 0.9, height = 12, y = 0.95 })
--   ctx:push({ root = bar, place = bar.place, modal = false })

local M = {}

-- A loading bar, which is a widget that also says where it goes.
---@class common.ui.LoadingBar: trx.ui.Widget
---@field place fun(w: number, h: number): number, number Where the bar goes, as trx.ui.layers.push takes it.

-- Creates a loading bar at the foot of the canvas. The settings are:
--   progress - how full the bar is, from 0 to 1, or a tween or a signal that
--              holds it
--   shown    - whether the bar is shown, or a signal that holds it
--   width    - the width, as a share of the canvas width
--   height   - the height, in canvas units
--   y        - where the middle of the bar stands, as a share of the canvas
--              height
function M.new(settings)
  local bar = trx.ui.widgets.Bar({
    type = trx.ui.BarType.PROGRESS,
    value = settings.progress,
    h = settings.height,
    smooth = true,
  })
  local width = trx.ui.signals.layout:map(function()
    return trx.ui.canvas.width * settings.width
  end)
  ---@type common.ui.LoadingBar
  local widget = trx.ui.widgets
    .Resize({ child = bar, w = width, shown = settings.shown })
    :release_with(width)
  function widget.place(w, h)
    return (trx.ui.canvas.width - w) / 2,
      trx.ui.canvas.height * settings.y - h / 2
  end
  return widget
end

return M
