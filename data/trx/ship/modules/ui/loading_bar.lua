-- The loading bar that a loading screen shows at its foot, filled by a value
-- that moves, such as a tween, and shown while the player's loading bar
-- setting is on.

local M = {}

-- Returns the bar and the place callback that puts it at the foot of the
-- canvas. `geometry` gives its width as a share of the canvas width, its
-- height in canvas units, and where its middle stands as a share of the canvas
-- height.
function M.new(progress, shown, geometry)
  local bar = trx.ui.widgets.Bar({
    type = trx.ui.BarType.PROGRESS,
    value = progress,
    h = geometry.height,
    smooth = true,
  })
  local widget = trx.ui.widgets.Custom({
    measure = function()
      local _, h = bar:measure()
      return trx.ui.canvas.width * geometry.width, h
    end,
    paint = function(x, y, w, h)
      bar:paint(x, y, w, h)
    end,
    shown = shown,
  })
  local function place(w, h)
    return (trx.ui.canvas.width - w) / 2,
      trx.ui.canvas.height * geometry.y - h / 2
  end
  return widget, place
end

return M
