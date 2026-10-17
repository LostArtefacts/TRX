-- The loading bar that a loading screen shows at its foot, filled by a value
-- that moves, such as a tween, and shown while the player's loading bar
-- setting is on. A bar in a PS1 look takes the size and place of the PS1
-- releases, and can carry a caption above it as TR4 does.
--
-- Usage:
--   local loading_bar = require("common.ui.widgets.loading_bar")
--   loading_bar.push(ctx, {
--     progress = tween,
--     pc = { width = 0.9, height = 12, y = 0.95 },
--     ps1 = { width = 0.76, height = 18, y = 0.943 },
--   })

local M = {}

trx.locale.declare({ ["general/misc/loading"] = "Loading" })

local CAPTION_GAP = 6

---@class common.ui.LoadingBar.Place
---@field width number The width, as a share of the canvas width.
---@field height number The height, in canvas units.
---@field y number Where the middle of the bar stands, as a share of the canvas height.

---@class common.ui.LoadingBar.settings
---@field progress any How full the bar is, from 0 to 1, or a tween or a signal that holds it.
---@field visible? any Whether the bar is visible, or a signal that holds it.
---@field pc common.ui.LoadingBar.Place Where a bar in a PC look goes.
---@field ps1 common.ui.LoadingBar.Place Where a bar in a PS1 look goes.
---@field caption? boolean Whether a bar in a PS1 look carries a caption above it.
---@field on_input? fun(ctx: any, keys: any) What the bar's layer does with input.

-- Pushes a loading bar at the foot of the canvas, and its caption where it
-- has one, and returns the bar's layer.
---@param ctx any The screen context.
---@param settings common.ui.LoadingBar.settings
function M.push(ctx, settings)
  local signal = trx.signal
  local visible = settings.visible or signal.new(true)
  local is_ps1 = signal.config("ui.bar_look"):map(function()
    return trx.ui.is_ps1_bar(trx.ui.BarType.PROGRESS)
  end)
  local place = is_ps1:map(function(ps1)
    return ps1 and settings.ps1 or settings.pc
  end)

  local bar = trx.ui.widgets.Bar({
    type = trx.ui.BarType.PROGRESS,
    value = settings.progress,
    h = place:map(function(at)
      return at.height
    end),
  })
  local width = signal.combine(trx.ui.signals.layout, place, function(_, at)
    return trx.ui.canvas.width * at.width
  end)

  local function bar_top(h)
    return trx.ui.canvas.height * place:get().y - h / 2
  end

  if settings.caption then
    ctx:push({
      root = trx.ui.widgets.Label({
        text = trx.strings.upper(trx.locale.get("general/misc/loading")),
        visible = visible & is_ps1,
      }),
      place = function(w, h)
        local _, bar_h = bar:measure()
        return (trx.ui.canvas.width - w) / 2, bar_top(bar_h) - CAPTION_GAP - h
      end,
    })
  end

  return ctx:push({
    root = trx.ui.widgets.Resize({ child = bar, w = width, visible = visible }),
    place = function(w, h)
      return (trx.ui.canvas.width - w) / 2, bar_top(h)
    end,
    on_input = settings.on_input,
  })
end

return M
