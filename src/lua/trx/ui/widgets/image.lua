local api = trx.api
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
local value_of = base.value_of
local new_widget = base.new_widget

api.define("ui.widgets.Image", {
  description = [[
A picture from an image file, at a size the script gives.

The widget keeps its room even where the game ships no such image, so a screen
built around it does not move when the image is missing.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The image settings.",
      fields = {
        {
          name = "path",
          type = "any",
          description = "The image file, named from the images directory, or a signal carrying it.",
        },
        {
          name = "w",
          type = "number",
          description = "The width, in canvas units.",
        },
        {
          name = "h",
          type = "number",
          description = "The height, in canvas units.",
        },
        {
          name = "opacity",
          type = "any",
          optional = true,
          description = "How solid the image is, from 0 to 1, or a signal that holds that value. `1` by default.",
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the image is shown, or a signal that holds that value.",
        },
      },
    },
  },
  returns = { type = "ui.Widget", description = "The image." },
  impl = function(settings)
    local self = new_widget(settings, function(w)
      return w.w, w.h
    end, function(w, x, y)
      primitive.image(
        value_of(w.path),
        x,
        y,
        w.w,
        w.h,
        value_of(w.opacity) or 1.0
      )
    end)
    return self:wakes_on(self.path, self.opacity)
  end,
})
