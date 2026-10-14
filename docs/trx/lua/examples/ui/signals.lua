-- Shows how long the level has run in the bottom right corner. The time hides
-- while photo mode is open.

local start = trx.signal.tick:get()
local seconds = trx.signal.tick:map(function(tick)
  return (tick - start) // trx.game.LOGIC_FPS
end)

local time = trx.ui.widgets.Label({
  text = seconds:map(function(s)
    return ("%d:%02d"):format(s // 60, s % 60)
  end),
  shown = ~trx.game.signals.is_photo_mode,
})

trx.ui.regions.place(trx.ui.Region.BOTTOM_RIGHT, time)
