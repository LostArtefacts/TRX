require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget

---@class (exact) trx.ui.widgets.Label.settings: trx.ui.widgets.settings
---@field text any The text, a `trx.locale.Text`, or a signal carrying either.
---@field scale? any Multiplies the text size, or a signal that holds it. `1.0` by default.

---A line of text. Use a signal for text that changes. A `trx.locale.Text`
---follows the player's language without one.
---@param settings trx.ui.widgets.Label.settings The label settings.
---@return trx.ui.Widget # The label.
function widgets.Label(settings)
  local shown = ""
  local self = new_widget(settings, function(w)
    shown = tostring(value_of(w.text))
    return primitive.measure_text(shown, value_of(w.scale) or 1.0)
  end, function(w, x, y)
    primitive.text(shown, x, y, value_of(w.scale) or 1.0, 0)
  end)
  return self
end
