local raw = trxc.ui
require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget

-- Every mesh of an object.
local ALL_MESHES = 0xFFFFFFFF

-- The tick a frame is drawn at, part way between the last two ticks.
local function now()
  return trx.signal.tick:get() - 1 + trx.ui.interpolation_rate()
end

---@class (exact) trx.ui.widgets.Model.settings: trx.ui.widgets.settings
---@field object any The object to draw, or a signal carrying it.
---@field w any The width of its box, in canvas units at the default text size, or a signal that holds it.
---@field h any The height of its box, in canvas units at the default text size, or a signal that holds it.
---@field size? any How large to draw the model within its box, from `0` to `1`, or a signal or `trx.ui.Tween` carrying it. The box keeps its room whatever the size. `1` by default.
---@field rot_x? any How far to tilt the model, or a signal or `trx.ui.Tween` carrying it.
---@field rot_y? any How far to turn the model, or a signal or `trx.ui.Tween` carrying it.
---@field rot_z? any How far to roll the model, or a signal or `trx.ui.Tween` carrying it.
---@field spin? any How far the model turns each tick, on top of `rot_y` <!--noref: rot_y-->, or a signal carrying it. It turns smoothly between ticks, and picks up where it stopped when the spin starts again.
---@field mesh_mask? any Which of its meshes to draw, one bit each, or a signal that holds the mask. Every mesh by default.
---@field light? any The light to draw it in, or a signal carrying it, as `trx.ui.primitive.model` takes it.

---An object's model, fitted into a box and lit as the game lights a model held
---up to the camera.
---
---```lua
---trx.ui.widgets.Model({
---  object = trx.catalog.objects.small_medipack_option,
---  w = 64,
---  h = 48,
---  spin = 4 * trx.math.DEG_1,
---})
---```
---@param settings trx.ui.widgets.Model.settings The model settings.
---@return trx.ui.Widget # The model.
function widgets.Model(settings)
  local self = new_widget(settings, function(w)
    local scale = raw.drawn_text_scale()
    return value_of(w.w) * scale, value_of(w.h) * scale
  end, function(w, x, y, box_w, box_h)
    local size = value_of(w.size) or 1
    if size <= 0 then
      return
    end
    local model_w = box_w * size
    local model_h = box_h * size
    -- The turn builds up from frame to frame, so a spin that stops and starts
    -- picks up where it stopped.
    local at = now()
    local spun = (w._spun or 0)
      + (value_of(w.spin) or 0) * (at - (w._spun_at or at))
    w._spun = spun % 0x10000
    w._spun_at = at
    local turn = math.floor((value_of(w.rot_y) or 0) + spun) % 0x10000
    primitive.model(
      value_of(w.object),
      x + (box_w - model_w) / 2,
      y + (box_h - model_h) / 2,
      model_w,
      model_h,
      {
        rot = {
          x = value_of(w.rot_x) or 0,
          y = turn,
          z = value_of(w.rot_z) or 0,
        },
        mesh_mask = value_of(w.mesh_mask) or ALL_MESHES,
        light = value_of(w.light),
      }
    )
  end)
  return self
end
