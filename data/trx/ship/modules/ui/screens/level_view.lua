-- The views of the level that TR4 shows while a level starts, each from one of
-- the level's cameras: it fades in from black, shows for its time with the
-- loading bar filling, and fades out to black. A skip fades it out at once, and
-- a skip while it fades out ends it.

local fade = require("common.ui.fade")
local loading_bar = require("common.ui.widgets.loading_bar")

local M = {}

-- How long the screen stays black after the view has faded out.
local HOLD_TICKS = 3

-- The loading bar spans most of the canvas width and stands at its foot.
local BAR = { width = 0.94, height = 14, y = 0.936 }

---@param ctx trx.ui.TimedContext
local function open(ctx)
  trx.input.hold_off(trx.input.Role.MENU_SKIP)

  local show = fade.screen(ctx):show({
    skip = trx.input.SkipContext.TO_GAME,
    leave = function(fades)
      fades:leave({ ticks = ctx.fade_out_ticks, hold = HOLD_TICKS })
    end,
  })
  local bar = loading_bar.new({
    progress = show.progress,
    visible = trx.signal.config("ui.show_loading_bar"),
    width = BAR.width,
    height = BAR.height,
    y = BAR.y,
  })
  return ctx:push({
    root = bar.widget,
    place = bar.place,
    on_input = function(_, keys)
      show:control(keys)
    end,
  })
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.LEVEL_VIEW, open)
end

return M
