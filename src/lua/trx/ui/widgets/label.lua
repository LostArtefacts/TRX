local api = trx.api
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
local value_of = base.value_of
local new_widget = base.new_widget
local text_scale = base.text_scale

api.define("ui.widgets.Label", {
  description = "A line of text. Use a signal for text that changes.",
  params = {
    {
      name = "settings",
      type = "table",
      description = "The label settings.",
      fields = {
        {
          name = "text",
          type = "any",
          description = "The text, or a signal carrying it.",
        },
        {
          name = "scale",
          type = "number",
          optional = true,
          description = "Multiplies the text size. `1.0` by default.",
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the label is shown, or a signal that holds that value.",
        },
      },
    },
  },
  returns = { type = "ui.Widget", description = "The label." },
  impl = function(settings)
    local self = new_widget(settings, function(w)
      return primitive.measure_text(tostring(value_of(w.text)), w.scale or 1.0)
    end, function(w, x, y)
      primitive.text(tostring(value_of(w.text)), x, y, w.scale or 1.0, 0)
    end)
    return self:wakes_on(self.text, text_scale())
  end,
})
