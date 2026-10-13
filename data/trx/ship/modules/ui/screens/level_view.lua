-- The views of the level that TR4 shows while a level starts, each from one of
-- the level's cameras: it fades in from black, shows for its time with the
-- loading bar filling, and fades out to black. A skip fades it out at once, and
-- a skip while it fades out ends it.
--
-- Usage:
--   require("common.ui.screens.level_view").setup()

local fade = require("common.ui.fade")
local loading_bar = require("common.ui.widgets.loading_bar")
local timer = require("common.timer")

local M = {}

-- How long the screen stays black after the view has faded out.
local HOLD_TICKS = 3

-- The loading bar spans most of the canvas width and stands at its foot.
local BAR = { width = 0.94, height = 14, y = 0.953 }

---@param ctx trx.ui.TimedContext
local function open(ctx)
  trx.input.hold_off(trx.input.Role.MENU_SKIP)

  local fades = fade.screen(ctx)
  local progress = trx.ui.tween(0)
  local leaving = false
  ---@type common.timer.Timer?
  local showing = nil

  local function finish()
    if ctx.is_held then
      ctx:confirm()
    end
  end

  local function fade_out()
    leaving = true
    if showing ~= nil then
      showing:cancel()
    end
    progress:set(1)
    fades:leave({ ticks = ctx.fade_out_ticks, hold = HOLD_TICKS })
  end

  fades:enter({
    ticks = ctx.fade_in_ticks,
    on_done = function()
      if leaving then
        return
      end
      progress:to(1, ctx.display_ticks)
      showing =
        timer.after(ctx.display_ticks, fade_out, { clock = timer.Clock.UI })
    end,
  })

  local shown = trx.signal.config("ui.show_loading_bar")
  local bar = loading_bar.new({
    progress = progress,
    shown = shown,
    width = BAR.width,
    height = BAR.height,
    y = BAR.y,
  })
  return ctx:push({
    root = bar.root,
    place = bar.place,
    on_input = function(_, keys)
      if not keys:pressed(trx.input.Role.MENU_SKIP) then
        return
      end
      trx.input.hold_off_skip(trx.input.SkipContext.TO_GAME)
      if leaving then
        finish()
      else
        fade_out()
      end
    end,
    on_close = function()
      if showing ~= nil then
        showing:cancel()
      end
    end,
  })
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.LEVEL_VIEW, open)
end

return M
