-- The loading bar that a loading screen shows at its foot, filled by a value
-- that moves, such as a tween, and shown while the player's loading bar
-- setting is on.
--
-- Usage:
--   local loading_bar = require("common.ui.widgets.loading_bar")
--   local bar = loading_bar.new({ progress = tween, width = 0.9, height = 12, y = 0.95 })
--   ctx:push({ root = bar.root, place = bar.place, modal = false })

local M = {}

---@class common.ui.LoadingBar
---@field root trx.ui.Widget
---@field place fun(w: number, h: number): number, number

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
  return {
    root = trx.ui.widgets.Custom({
      measure = function()
        local _, h = bar:measure()
        return trx.ui.canvas.width * settings.width, h
      end,
      paint = function(x, y, w, h)
        bar:paint(x, y, w, h)
      end,
      shown = settings.shown,
    }),
    place = function(w, h)
      return (trx.ui.canvas.width - w) / 2,
        trx.ui.canvas.height * settings.y - h / 2
    end,
  }
end

return M
