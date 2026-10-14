require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget

---@class (exact) trx.ui.widgets.Image.settings: trx.ui.widgets.settings
---@field path any The image file, named from the images directory, or a signal carrying it.
---@field w any The width, in canvas units, or a signal that holds it.
---@field h any The height, in canvas units, or a signal that holds it.
---@field opacity? any How solid the image is, from 0 to 1, or a signal that holds that value. `1` by default.

---A picture from an image file, at a size the script gives.
---
---The widget keeps its room even where the game ships no such image, so a
---screen built around it does not move when the image is missing.
---@param settings trx.ui.widgets.Image.settings The image settings.
---@return trx.ui.Widget # The image.
function widgets.Image(settings)
  local self = new_widget(settings, function(w)
    return value_of(w.w), value_of(w.h)
  end, function(w, x, y)
    local iw, ih = w:measure()
    primitive.image(value_of(w.path), x, y, iw, ih, value_of(w.opacity) or 1.0)
  end)
  return self
end
