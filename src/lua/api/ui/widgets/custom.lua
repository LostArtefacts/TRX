local api = trx.api
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
local new_widget = base.new_widget

api.define("ui.widgets.Custom", {
  description = [[
A widget that measures and draws itself through functions the script gives.

Use it for drawing that the other widgets do not cover. Register the signals
that the functions read with `trx.ui.Widget:wakes_on`.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The widget settings.",
      fields = {
        {
          name = "measure",
          type = "function",
          description = "Returns the width and the height the widget wants, in canvas units.",
        },
        {
          name = "paint",
          type = "function",
          description = [[
Draws the widget with `trx.ui.primitive`. It receives the left edge, the top
edge, the width and the height of the box the widget was given.]],
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the widget is shown, or a signal that holds that value.",
        },
      },
    },
  },
  returns = { type = "ui.Widget", description = "The widget." },
  examples = {
    [[local mark = trx.ui.widgets.Custom({
  measure = function()
    return 8, 8
  end,
  paint = function(x, y, w, h)
    trx.ui.primitive.quad(x, y, 0, w, h, trx.math.color("#ffffff"))
  end,
})]],
  },
  impl = function(settings)
    -- Removes measure and paint from the settings: the settings table becomes
    -- the widget, and these keys would hide its own methods.
    local measure = settings.measure
    local paint = settings.paint
    settings.measure = nil
    settings.paint = nil
    return new_widget(settings, function()
      return measure()
    end, function(_, x, y, w, h)
      paint(x, y, w, h)
    end)
  end,
})
