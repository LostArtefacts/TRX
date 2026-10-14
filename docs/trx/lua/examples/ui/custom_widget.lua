-- Draws a battery of five cells in the top left corner. One cell drains every
-- five seconds, and the battery stays empty once the last cell has drained.

local CELLS = 5
local CELL_W = 6
local CELL_H = 10
local GAP = 2
local TICKS_PER_CELL = 5 * trx.game.LOGIC_FPS

local FULL = trx.math.color("#40e040")
local EMPTY = trx.math.color("#404040")

local start = trx.signal.tick:get()
local charge = trx.signal.tick:map(function(tick)
  return math.max(0, CELLS - (tick - start) // TICKS_PER_CELL)
end)

local battery = trx.ui.widgets.Custom({
  measure = function()
    return CELLS * (CELL_W + GAP) - GAP, CELL_H
  end,
  -- The size does not depend on the charge, so only the paint reads it.
  paint = function(x, y)
    for i = 1, CELLS do
      local color = i <= charge:get() and FULL or EMPTY
      trx.ui.primitive.quad(
        x + (i - 1) * (CELL_W + GAP),
        y,
        0,
        CELL_W,
        CELL_H,
        color
      )
    end
  end,
})

trx.ui.regions.place(trx.ui.Region.TOP_LEFT, battery)
