-- The loading bar that a loading screen shows at its foot, filled by a value
-- that moves, such as a tween, and shown while the player's loading bar
-- setting is on.
--
-- Usage:
--   local loading_bar = require("common.ui.widgets.loading_bar")
--   local bar = loading_bar.new({ progress = tween, width = 0.9, height = 12, y = 0.95 })
--   ctx:push({ root = bar.widget, place = bar.place })

local M = {}

-- A loading bar: its widget, and where it goes.
---@class common.ui.LoadingBar
---@field widget trx.ui.Widget The bar, to push or to place in another widget.
---@field place fun(w: number, h: number): number, number Where the bar goes, as trx.ui.layers.push takes it.

---@class common.ui.LoadingBar.settings
---@field progress any How full the bar is, from 0 to 1, or a tween or a signal that holds it.
---@field shown? any Whether the bar is shown, or a signal that holds it.
---@field width number The width, as a share of the canvas width.
---@field height number The height, in canvas units.
---@field y number Where the middle of the bar stands, as a share of the canvas height.

-- Creates a loading bar at the foot of the canvas.
---@param settings common.ui.LoadingBar.settings
---@return common.ui.LoadingBar
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
  return {
    widget = trx.ui.widgets.Resize({
      child = bar,
      w = width,
      shown = settings.shown,
    }),
    place = function(w, h)
      return (trx.ui.canvas.width - w) / 2,
        trx.ui.canvas.height * settings.y - h / 2
    end,
  }
end

return M
