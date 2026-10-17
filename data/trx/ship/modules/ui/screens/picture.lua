-- The pictures the game shows: between levels, while a level loads, and for
-- the legal notice and the credits. A picture fades in, from the frame before
-- it where it may, shows for its time, and leaves: a loading picture hands
-- over to the level, any other fades out to black. A skip leaves at once, and
-- a skip while it fades out ends it.

local fade = require("common.ui.fade")
local level_start = require("common.ui.level_start")
local loading_bar = require("common.ui.widgets.loading_bar")
local timer = require("common.timer")

local M = {}

-- The loading bar of a loading picture, narrower than the level views' one.
local BAR = {
  pc = { width = 0.76, height = 12, y = 0.941 },
  ps1 = { width = 0.76, height = 18, y = 0.943 },
}

-- How long the screen stays black after the picture has faded out.
local HOLD_TICKS = 3

---@param ctx trx.ui.PictureContext
local function open(ctx)
  local drawn = false
  local waiting = false
  ---@type common.timer.Timer?
  local pending = nil

  local function hand_over()
    waiting = false
    level_start.hand_over(ctx)
  end

  -- A loading picture hands over only once it has been drawn, so that what
  -- the level blends from is the picture.
  local show = fade.screen(ctx):show({
    layer = ctx.can_cross_fade_in and { snapshot = true } or nil,
    skip = trx.input.SkipContext.TO_SCREEN,
    leave = function(fades)
      if not ctx.is_loading then
        fades:leave({ ticks = ctx.fade_out_ticks, hold = HOLD_TICKS })
      elseif drawn then
        hand_over()
      else
        waiting = true
      end
    end,
  })

  ctx:on_end(function()
    if pending ~= nil then
      pending:cancel()
    end
  end)

  local main = ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return trx.ui.canvas.width, trx.ui.canvas.height
      end,
      paint = function()
        trx.ui.primitive.background(ctx.background)
        if not drawn then
          drawn = true
          if waiting then
            pending = timer.after(0, hand_over, { clock = timer.Clock.UI })
          end
        end
      end,
    }),
    place = { canvas = true },
    on_input = function(_, keys)
      if not waiting then
        show:control(keys)
      end
    end,
  })
  if ctx.is_loading then
    loading_bar.push(ctx, {
      progress = show.progress,
      visible = trx.signal.config("ui.show_loading_bar"),
      pc = BAR.pc,
      ps1 = BAR.ps1,
    })
  end
  return main
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.PICTURE, open)
end

return M
