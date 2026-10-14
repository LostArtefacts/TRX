-- Slides the name of the level down from the top of the screen as the level
-- starts, keeps it there for three seconds, and slides it back out.

local IN_TICKS = 10
local HOLD_TICKS = 3 * trx.game.LOGIC_FPS
local MARGIN = 8

local slide = trx.ui.tween(0)

local banner = trx.ui.layers.push({
  root = trx.ui.widgets.Frame({
    child = trx.ui.widgets.Pad({
      x = 8,
      y = 4,
      child = trx.ui.widgets.Label({ text = "The Lost Valley" }),
    }),
  }),
  place = function(w, h)
    local area = trx.ui.safe_area
    local y = area.y - h + (h + MARGIN) * slide:get()
    return area.x + (area.width - w) / 2, y
  end,
})

slide:to(1, IN_TICKS, { curve = trx.ui.ease.quad_out })

local watch
watch = trx.events.on_tick(function()
  if slide:is_moving() then
    return
  end
  if slide:target() == 1 then
    slide:to(0, IN_TICKS, { curve = trx.ui.ease.quad_in, delay = HOLD_TICKS })
  else
    banner:close()
    watch:detach()
  end
end)
