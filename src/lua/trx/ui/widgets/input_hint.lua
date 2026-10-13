require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget
local text_scale = base.text_scale

---@class (exact) trx.ui.widgets.InputHint.settings
---@field input any The input role whose keys to show, a list of roles shown side by side, or a signal carrying either.
---@field text? any What the keys do, or a signal carrying it.
---@field scale? number Multiplies the text size. `1.0` by default.
---@field shown? any Whether the hint is shown, or a signal that holds that value.

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
    return primitive.measure_text(text(w), w.scale or 1.0)
  end, function(w, x, y)
    primitive.text(text(w), x, y, w.scale or 1.0, 0)
  end)
  return self:wakes_on(text_scale())
end
