-- The pictures the game shows: between levels, while a level loads, and for
-- the legal notice and the credits. A picture fades in, from the frame before
-- it where it may, shows for its time, and leaves: a loading picture hands
-- over to the level, any other fades out to black. A skip leaves at once, and
-- a skip while it fades out ends it.
--
-- Usage:
--   require("common.ui.screens.picture").setup()

local fade = require("common.ui.fade")
local level_start = require("common.ui.level_start")
local loading_bar = require("common.ui.widgets.loading_bar")
local timer = require("common.timer")

local M = {}

-- The loading bar of a loading picture, narrower than the level views' one.
local BAR = { width = 0.76, height = 12, y = 0.941 }

---@param ctx trx.ui.PictureContext
local function open(ctx)
  local fades = fade.screen(ctx)
  local progress = trx.ui.tween(0)
  local leaving = false
  local drawn = false
  local waiting = false
  ---@type common.timer.Timer?
  local showing = nil

  local function finish()
    if ctx.is_held then
      ctx:confirm()
    end
  end

  local function hand_over()
    waiting = false
    level_start.hand_over(ctx)
  end

  -- A loading picture hands over only once it has been drawn, so that what
  -- the level blends from is the picture.
  local function leave()
    leaving = true
    if showing ~= nil then
      showing:cancel()
    end
    progress:set(1)
    if not ctx.is_loading then
      fades:leave({ ticks = ctx.fade_out_ticks })
    elseif drawn then
      hand_over()
    else
      waiting = true
    end
  end

  fades:enter({
    layer = ctx.can_cross_fade_in and { snapshot = true } or nil,
    ticks = ctx.fade_in_ticks,
    on_done = function()
      if leaving then
        return
      end
      progress:to(1, ctx.display_ticks)
      showing =
        timer.after(ctx.display_ticks, leave, { clock = timer.Clock.UI })
    end,
  })

  local shown = trx.signal.config("ui.show_loading_bar")
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
            timer.after(0, hand_over, { clock = timer.Clock.UI })
          end
        end
      end,
    }),
    place = function()
      return 0, 0
    end,
    on_input = function(_, keys)
      if waiting or not keys:pressed(trx.input.Role.MENU_SKIP) then
        return
      end
      trx.input.hold_off_skip(trx.input.SkipContext.TO_SCREEN)
      if leaving then
        finish()
      else
        leave()
      end
    end,
    on_close = function()
      if showing ~= nil then
        showing:cancel()
      end
    end,
  })
  if ctx.is_loading then
    local bar = loading_bar.new({
      progress = progress,
      shown = shown,
      width = BAR.width,
      height = BAR.height,
      y = BAR.y,
    })
    ctx:push({ root = bar.root, place = bar.place, modal = false })
  end
  return main
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.PICTURE, open)
end

return M
