local raw = trxc.ui
require("trx.internal.helpers")
local themes = require("trx.internal.themes")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget

-- The size of the game's own bars where their look states none, in canvas
-- units before the player's bar scale.
local BAR_WIDTH = 208
local BAR_HEIGHT = 18

---@class (exact) trx.ui.widgets.Bar.settings: trx.ui.widgets.settings
---@field type any The `trx.ui.BarType` theme to use, or a signal that holds it.
---@field value any The fill amount from 0 to 1, or a signal that holds it.
---@field w? any The width, in canvas units at the default bar size, or a signal that holds it. The size its look gives it by default.
---@field h? any The height, in canvas units at the default bar size, or a signal that holds it. The size its look gives it by default.
---@field smooth? any Whether the fill shades one band into the next, or a signal that holds that value. The player's smooth bars setting by default.
---@field preview? any Whether the bar takes the size of the text around it rather than the player's bar size and the size its style gives it, as a bar shown beside its setting does, or a signal that holds that value. `false` by default.

---One of the game's bars, drawn with the player's bar settings.
---
---The bar uses the same theme, border, and fill bands as the engine UI. Use a
---signal for a fill value that changes.
---@param settings trx.ui.widgets.Bar.settings The bar settings.
---@return trx.ui.Widget # The bar.
function widgets.Bar(settings)
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

  local side_borders = nil

  local function has_side_borders()
    if side_borders == nil then
      side_borders = trx.signal.config("ui.enable_bar_side_borders")
    end
    return side_borders:get() == true
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
    return themes.bar(value_of(w.type) or trx.ui.BarType.LARA_HP)
  end

  local self = new_widget(settings, function(w)
    local theme = theme_of(w)
    if value_of(w.preview) then
      local scale = raw.drawn_text_scale()
      return (value_of(w.w) or BAR_WIDTH) * scale,
        (value_of(w.h) or BAR_HEIGHT) * scale
    end
    local scale = raw.bar_scale() * (theme ~= nil and theme.basic_scale or 1)
    local look_w = theme ~= nil and theme.width or BAR_WIDTH
    local look_h = theme ~= nil and theme.height or BAR_HEIGHT
    return (value_of(w.w) or look_w) * scale, (value_of(w.h) or look_h) * scale
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
    -- The height splits into one row for each fill band, plus the frame and
    -- the padding on both sides, each as thick as a band.
    local steps = #(theme.ramp or theme.ramp_left)
    local x0, y0 = px(x), px(y)
    local w_px, h_px = px(x + bw) - x0, px(y + bh) - y0
    local edge = h_px // (steps + 2 + 2 * theme.padding)
    local pad = math.floor(edge * theme.padding)
    local at = primitive.to_canvas

    -- A side column runs from its first color at the top to its second at
    -- the bottom on the left, and the other way on the right.
    local side = theme.border_side
    if side ~= nil and has_side_borders() then
      local sy = at(y0 + edge)
      local sh = at(y0 + h_px - edge) - sy
      local lx, rx = at(x0), at(x0 + w_px - edge)
      local sw = at(x0 + edge) - lx
      primitive.gradient_quad(
        lx,
        sy,
        0,
        sw,
        sh,
        side[1],
        side[1],
        side[2],
        side[2]
      )
      primitive.gradient_quad(
        rx,
        sy,
        0,
        sw,
        sh,
        side[2],
        side[2],
        side[1],
        side[1]
      )
      x0, w_px = x0 + edge, w_px - 2 * edge
    end

    local x1, y1 = x0 + edge, y0 + edge
    local x2, y2 = x1 + pad, y1 + pad
    local ix, iy = at(x1), at(y1)
    local iw, ih = at(x0 + w_px - edge) - ix, at(y0 + h_px - edge) - iy
    local fx = at(x2)
    local fh_px = h_px - 2 * edge - 2 * pad
    local fw = (at(x0 + w_px - edge - pad) - fx) * fill

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

    local shaded = value_of(w.smooth)
    if shaded == nil then
      shaded = is_smooth()
    end
    local count = shaded and steps - 1 or steps
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
  return self
end
