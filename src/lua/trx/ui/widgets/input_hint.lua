require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget

---@class (exact) trx.ui.widgets.InputHint.settings: trx.ui.widgets.settings
---@field input any The input role whose keys to show, a list of roles shown side by side, or a signal carrying either.
---@field text? any What the keys do, as a string or a `trx.locale.Text`, or a signal carrying either.
---@field scale? any Multiplies the text size, or a signal that holds it. `1.0` by default.

---The keys bound to an input role, as the game shows them, and what they do.
---The keys follow the player's bindings and controller.
---
---```lua
---trx.ui.widgets.InputHint({
---  input = trx.input.Role.MENU_BACK,
---  text = trx.locale.text("general/actions/cancel"),
---})
---```
---@param settings trx.ui.widgets.InputHint.settings The hint settings.
---@return trx.ui.Widget # The hint.
function widgets.InputHint(settings)
  local shown = ""
  local self = new_widget(settings, function(w)
    shown =
      tostring(trx.ui.input_hint_text(value_of(w.input), value_of(w.text)))
    return primitive.measure_text(shown, value_of(w.scale) or 1.0)
  end, function(w, x, y)
    primitive.text(shown, x, y, value_of(w.scale) or 1.0, 0)
  end)
  return self
end
