require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget

---@class (exact) trx.ui.widgets.Image.settings: trx.ui.widgets.settings
---@field path any The image file, named from the images directory, or a signal carrying it.
---@field w? any The width, in canvas units, or a signal that holds it. Without it, the width follows the height in the image's own shape.
---@field h? any The height, in canvas units, or a signal that holds it. Without it, the height follows the width in the image's own shape.
---@field opacity? any How solid the image is, from 0 to 1, or a signal that holds that value. `1` by default.

---A picture from an image file, at a size the script gives.
---
---Give the width, the height, or both. Where one of them is left out, it
---follows the other in the image's own shape. The widget keeps its room even
---where the game ships no such image, so a screen built around it does not
---move when the image is missing; a size left out is then 0.
---
---```lua
---local logo = trx.ui.widgets.Image({ path = "logo.webp", w = 512 })
---```
---@param settings trx.ui.widgets.Image.settings The image settings.
---@return trx.ui.Widget # The image.
function widgets.Image(settings)
  if settings.w == nil and settings.h == nil then
    error("an image needs a width or a height", 2)
  end
  local self = new_widget(settings, function(w)
    local width, height = value_of(w.w), value_of(w.h)
    if width ~= nil and height ~= nil then
      return width, height
    end
    local iw, ih = primitive.image_size(value_of(w.path))
    if iw == nil or iw <= 0 or ih <= 0 then
      return width or 0, height or 0
    elseif width == nil then
      return height * iw / ih, height
    end
    return width, width * ih / iw
  end, function(w, x, y)
    local iw, ih = w:measure()
    primitive.image(value_of(w.path), x, y, iw, ih, value_of(w.opacity) or 1.0)
  end)
  return self
end
