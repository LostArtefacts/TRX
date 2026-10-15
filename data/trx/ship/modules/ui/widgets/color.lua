-- The widgets of the color editor: a swatch that shows a color, and a slider
-- that shows a range of colors with a marker on the one picked. Both draw a
-- black box with the colors one pixel inside it.
--
-- Usage:
--   local color = require("common.ui.widgets.color")
--   local swatch = color.swatch({ color = water, w = 13, h = 13 })
--   local hue = color.slider({ stops = stops, value = position, w = 100 })

local metrics = require("common.ui.metrics")
local value = require("common.ui.value")

local M = {}

local BLACK = trx.math.color("000000")
local WHITE = trx.math.color("FFFFFF")

local MARKER_WIDTH = 4
local MARKER_BORDER = 1

local primitive = trx.ui.primitive

-- Rounds a canvas position to the pixel it falls on.
local function snap(position)
  return primitive.to_canvas(math.floor(primitive.to_screen(position) + 0.5))
end

-- The black box, and the box one pixel inside it. Every edge lands on a whole
-- screen pixel, so that the border is as thick on one side as on the other.
local function boxes(x, y, w, h)
  local function px(position)
    return math.floor(primitive.to_screen(position) + 0.5)
  end
  local at = primitive.to_canvas
  local x0, y0 = px(x), px(y)
  local w_px, h_px = px(x + w) - x0, px(y + h) - y0
  local border = math.max(1, px(1))
  primitive.quad(at(x0), at(y0), 0, at(w_px), at(h_px), BLACK)
  return at(x0 + border),
    at(y0 + border),
    at(w_px - 2 * border),
    at(h_px - 2 * border)
end

---@class common.ui.ColorSwatch.settings: trx.ui.widgets.settings
---@field color any The color, or a signal carrying it.
---@field w number The width, in canvas units at the default text size.
---@field h number The height, in canvas units at the default text size.

-- Creates a swatch: the color in a black box, outlined as a heading is.
---@param settings common.ui.ColorSwatch.settings
---@return trx.ui.Widget
function M.swatch(settings)
  return trx.ui.widgets.Custom({
    shown = settings.shown,
    hidden = settings.hidden,
    measure = function()
      local scale = trx.ui.text_scale
      return settings.w * scale, settings.h * scale
    end,
    paint = function(x, y, w, h)
      local ix, iy, iw, ih = boxes(x, y, w, h)
      primitive.quad(ix, iy, 0, iw, ih, value.read(settings.color))
      primitive.panel(ix, iy, 0, iw, ih, trx.ui.FrameStyle.HEADING_OUTLINE)
    end,
  })
end

-- The marker on the picked color: white, in a black border, outlined as the
-- option under the cursor is.
local function marker(x, y, h)
  local half = MARKER_WIDTH * trx.ui.text_scale / 2
  local x1, x2 = snap(x - half), snap(x + half)
  local y1, y2 = snap(y - h / 2), snap(y + h / 2)
  local border =
    primitive.to_canvas(math.floor(primitive.to_screen(MARKER_BORDER) + 0.5))
  local ox, oy = x1 - border, y1 - border
  local ow, oh = x2 - x1 + 2 * border, y2 - y1 + 2 * border
  primitive.quad(ox, oy, 0, ow, oh, BLACK)
  primitive.quad(x1, y1, 0, x2 - x1, y2 - y1, WHITE)
  primitive.panel(ox, oy, 0, ow, oh, trx.ui.FrameStyle.OUTLINE)
end

---@class common.ui.ColorSlider.settings: trx.ui.widgets.settings
---@field stops any The colors the slider runs through, evenly spaced, or a signal carrying them.
---@field value any Where the marker sits, from 0 to 1, or a signal that holds it.
---@field w number The width, in canvas units at the default text size.

-- Creates a slider: the colors it runs through, and a marker on one of them.
---@param settings common.ui.ColorSlider.settings
---@return trx.ui.Widget
function M.slider(settings)
  return trx.ui.widgets.Custom({
    shown = settings.shown,
    hidden = settings.hidden,
    measure = function()
      local scale = trx.ui.text_scale
      return settings.w * scale, metrics.TEXT_HEIGHT * 0.5 * scale
    end,
    paint = function(x, y, w, h)
      local ix, iy, iw, ih = boxes(x, y, w, h)
      local stops = value.read(settings.stops)
      if #stops == 1 then
        primitive.quad(ix, iy, 0, iw, ih, stops[1])
      else
        local spans = #stops - 1
        for i = 1, spans do
          local sx = snap(ix + (i - 1) * iw / spans)
          local ex = snap(ix + i * iw / spans)
          primitive.gradient_quad(
            sx,
            iy,
            0,
            math.max(ex - sx, primitive.to_canvas(1)),
            ih,
            stops[i],
            stops[i + 1],
            stops[i],
            stops[i + 1]
          )
        end
      end
      local position = math.max(0, math.min(value.read(settings.value), 1))
      marker(ix + position * iw, iy + ih / 2, h)
    end,
  })
end

return M
