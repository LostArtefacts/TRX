require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget

---@class (exact) trx.ui.widgets.InputHint.settings: trx.ui.widgets.settings
---@field input any The input role whose keys to show, a list of roles shown side by side, or a signal carrying either.
---@field text? any What the keys do, or a signal carrying it.
---@field scale? any Multiplies the text size, or a signal that holds it. `1.0` by default.

---The keys bound to an input role, as the game shows them, and what they do.
---The keys follow the player's bindings and controller.
---
---```lua
---trx.ui.widgets.InputHint({
---  input = trx.input.Role.MENU_BACK,
---  text = trx.locale.get("general/actions/cancel"),
---})
---```
---@param settings trx.ui.widgets.InputHint.settings The hint settings.
---@return trx.ui.Widget # The hint.
function widgets.InputHint(settings)
  local function text(w)
    return trx.ui.input_hint_text(value_of(w.input), value_of(w.text))
  end
  local self = new_widget(settings, function(w)
    return primitive.measure_text(text(w), value_of(w.scale) or 1.0)
  end, function(w, x, y)
    primitive.text(text(w), x, y, value_of(w.scale) or 1.0, 0)
  end)
  return self
end
