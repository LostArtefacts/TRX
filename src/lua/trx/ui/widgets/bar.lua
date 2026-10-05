local raw = trxc.ui
require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget
local bar_scale = base.bar_scale

---@class (exact) trx.ui.widgets.Bar.settings
---@field type trx.ui.BarType The bar theme to use.
---@field value any The fill amount from 0 to 1, or a signal that holds it.
---@field w? number The width, in canvas units. The game's own by default.
---@field h? number The height, in canvas units. The game's own by default.
---@field shown? any Whether the bar is shown, or a signal that holds that value.

---One of the game's bars, drawn with the player's bar settings.
---
---The bar uses the same theme, border, and fill bands as the engine UI. Use a
---signal for a fill value that changes.
---@param settings trx.ui.widgets.Bar.settings The bar settings.
---@return trx.ui.Widget # The bar.
function widgets.Bar(settings)
  local STEPS = 5
  local BLACK = trx.math.color("#000000")

  -- Smooth bars shade one fill band into the next. The setting differs by
  -- game.
  local smooth = nil

  local function is_smooth()
    if smooth == nil then
      smooth = trx.signal.config("ui.enable_smooth_bars")
    end
    return smooth:get() == true
  end

  -- Color channels are integer values; match the engine by truncating mixes.
  local function mix(c1, c2, ratio)
    return {
      r = math.floor(c1.r + (c2.r - c1.r) * ratio),
      g = math.floor(c1.g + (c2.g - c1.g) * ratio),
      b = math.floor(c1.b + (c2.b - c1.b) * ratio),
      a = math.floor(c1.a + (c2.a - c1.a) * ratio),
    }
  end

  -- Split the fill into vertical bands. Smooth bars blend between adjacent
  -- ramp entries, while flat bars use one ramp entry per band. The split
  -- is made in whole screen pixels so that each band starts exactly where
  -- the previous one ends, with no gap after rounding.
  local function bands(y_px, h_px, count)
    local at = primitive.to_canvas
    local out = {}
    for i = 0, count - 1 do
      local top = at(y_px + h_px * i // count)
      local bottom = at(y_px + h_px * (i + 1) // count)
      out[#out + 1] = { i + 1, top, bottom - top }
    end
    return out
  end

  local function theme_of(w)
    return raw.bar_theme(w.type or trx.ui.BarType.LARA_HP)
  end

  local self = new_widget(settings, function(w)
    local theme = theme_of(w)
    local scale = raw.bar_scale() * (theme ~= nil and theme.basic_scale or 1)
    return (w.w or 208) * scale, (w.h or 18) * scale
  end, function(w, x, y, bw, bh)
    local theme = theme_of(w)
    if theme == nil then
      return
    end

    local fill = math.max(0.0, math.min(1.0, value_of(w.value) or 0))
    fill = math.floor(fill * 100) / 100

    -- Work in screen pixels first so every bar edge lands on a whole pixel.
    -- Rounding canvas coordinates independently can make opposite borders
    -- come out at different thicknesses.
    local px = function(v)
      return math.floor(primitive.to_screen(v) + 0.5)
    end
    local x0, y0 = px(x), px(y)
    local w_px, h_px = px(x + bw) - x0, px(y + bh) - y0
    local edge = h_px // (STEPS + 4)

    local at = primitive.to_canvas
    local x1, y1 = x0 + edge, y0 + edge
    local x2, y2 = x1 + edge, y1 + edge
    local ix, iy = at(x1), at(y1)
    local iw, ih = at(x0 + w_px - edge) - ix, at(y0 + h_px - edge) - iy
    local fx = at(x2)
    local fh_px = h_px - 4 * edge
    local fw = (at(x0 + w_px - 2 * edge) - fx) * fill

    local ox, oy = at(x0), at(y0)
    local ow, oh = at(x0 + w_px) - ox, at(y0 + h_px) - oy

    if theme.kind == "ps1" then
      primitive.gradient_quad(
        ox,
        oy,
        0,
        ow,
        oh,
        theme.border_tl,
        theme.border_tr,
        theme.border_bl,
        theme.border_br
      )
    else
      primitive.quad(ox, oy, 0, ow, oh, theme.border_light)
      -- Match the engine: the dark side of the border extends to the far
      -- edge instead of stopping before the last border strip.
      primitive.quad(
        ix,
        iy,
        0,
        at(x0 + w_px) - ix,
        at(y0 + h_px) - iy,
        theme.border_dark
      )
    end

    primitive.quad(ix, iy, 0, iw, ih, BLACK)
    if fill <= 0 then
      return
    end

    local shaded = is_smooth()
    local count = shaded and STEPS - 1 or STEPS
    for _, band in ipairs(bands(y2, fh_px, count)) do
      local step, by, step_h = band[1], band[2], band[3]
      if theme.kind == "ps1" then
        local tl = theme.ramp_left[step]
        local tr = theme.ramp_right[step]
        if shaded then
          local bl = theme.ramp_left[step + 1]
          local br = theme.ramp_right[step + 1]
          primitive.gradient_quad(
            fx,
            by,
            0,
            fw,
            step_h,
            tl,
            mix(tl, tr, fill),
            bl,
            mix(bl, br, fill)
          )
        else
          local trm = mix(tl, tr, fill)
          primitive.gradient_quad(fx, by, 0, fw, step_h, tl, trm, tl, trm)
        end
      elseif shaded then
        local c1 = theme.ramp[step]
        local c2 = theme.ramp[step + 1]
        primitive.gradient_quad(fx, by, 0, fw, step_h, c1, c1, c2, c2)
      else
        primitive.quad(fx, by, 0, fw, step_h, theme.ramp[step])
      end
    end
  end)
  return self:wakes_on(bar_scale())
end
