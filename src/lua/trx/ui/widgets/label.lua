require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget
local text_scale = base.text_scale

---@class (exact) trx.ui.widgets.Label.settings
---@field text any The text, or a signal carrying it.
---@field scale? number Multiplies the text size. `1.0` by default.
---@field shown? any Whether the label is shown, or a signal that holds that value.

---A line of text. Use a signal for text that changes.
---@param settings trx.ui.widgets.Label.settings The label settings.
---@return trx.ui.Widget # The label.
function widgets.Label(settings)
  local self = new_widget(settings, function(w)
    return primitive.measure_text(tostring(value_of(w.text)), w.scale or 1.0)
  end, function(w, x, y)
    primitive.text(tostring(value_of(w.text)), x, y, w.scale or 1.0, 0)
  end)
  return self:wakes_on(self.text, text_scale())
end
