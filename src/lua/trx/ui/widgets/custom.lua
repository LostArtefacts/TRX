require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local new_widget = base.new_widget

---@class (exact) trx.ui.widgets.Custom.settings: trx.ui.widgets.settings
---@field measure function Returns the width and the height the widget wants, in canvas units.
---@field paint function Draws the widget with `trx.ui.primitive`. It receives the left edge, the top edge, the width and the height of the box the widget was given.

---A widget that measures and draws itself through functions the script gives.
---
---Use it for drawing that the other widgets do not cover. The widget
---remeasures when a signal that `measure` read changes. <!--noref: measure-->
---
---```lua
---local mark = trx.ui.widgets.Custom({
---  measure = function()
---    return 8, 8
---  end,
---  paint = function(x, y, w, h)
---    trx.ui.primitive.quad(x, y, 0, w, h, trx.math.color("#ffffff"))
---  end,
---})
---```
---@param settings trx.ui.widgets.Custom.settings The widget settings.
---@return trx.ui.Widget # The widget.
function widgets.Custom(settings)
  local measure = settings.measure
  local paint = settings.paint
  local rest = {}
  for key, value in pairs(settings) do
    if key ~= "measure" and key ~= "paint" then
      rest[key] = value
    end
  end
  return new_widget(rest, function()
    return measure()
  end, function(_, x, y, w, h)
    paint(x, y, w, h)
  end)
end
