local raw = trxc.ui
require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

require("trx.game")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget
local text_scale = base.text_scale

-- The height in canvas units at the default text size, and the frame around
-- the fill.
local HEIGHT = 4.0
local BORDER = 1.0

local BACKGROUND = trx.math.color(0x06, 0x06, 0x06)

-- The fill colour of each game, by game number.
local FILLS = {
  trx.math.color(0xA1, 0x83, 0x3C),
  trx.math.color(0x5A, 0xB5, 0x5A),
  trx.math.color(0x1C, 0x6A, 0xC4),
  trx.math.color(0xA1, 0x83, 0x3C),
}

---@class (exact) trx.ui.widgets.SleekBar.settings
---@field progress any How full the bar is, from 0 to 1, or a signal that holds it.
---@field shown? any Whether the bar is shown, or a signal that holds that value.

---A thin bar that shows progress, as the game draws it under a button that the
---player holds.
---
---The bar is a dark frame with a fill in the game's own colour. It takes the
---width of the box that it is given, and its height follows the text size. Use
---a signal for progress that changes.
---
---```lua
---local held = trx.signal.new(0)
---local bar = trx.ui.widgets.SleekBar({ progress = held })
---```
---@param settings trx.ui.widgets.SleekBar.settings The bar settings.
---@return trx.ui.Widget # The bar.
function widgets.SleekBar(settings)
  local self = new_widget(settings, function()
    return 0, HEIGHT * raw.drawn_text_scale()
  end, function(w, x, y, bw, bh)
    local progress = math.max(0.0, math.min(1.0, value_of(w.progress) or 0))

    -- Places every edge on a whole screen pixel first. Canvas units do not
    -- map to whole pixels, so rounding each edge on its own can give the
    -- frame a different thickness on opposite sides.
    local px = function(v)
      return math.floor(primitive.to_screen(v) + 0.5)
    end
    local at = primitive.to_canvas
    local x0, y0 = px(x), px(y)
    local x1, y1 = px(x + bw), px(y + bh)
    local border = math.max(1, px(BORDER))
    local fill_px = math.floor((x1 - x0 - 2 * border) * progress)

    primitive.quad(
      at(x0),
      at(y0),
      0,
      at(x1) - at(x0),
      at(y1) - at(y0),
      BACKGROUND
    )
    if fill_px > 0 then
      local fx, fy = at(x0 + border), at(y0 + border)
      primitive.quad(
        fx,
        fy,
        0,
        at(x0 + border + fill_px) - fx,
        at(y1 - border) - fy,
        FILLS[trx.game.tr_version] or FILLS[1]
      )
    end
  end)
  return self:wakes_on(self.progress, text_scale())
end
