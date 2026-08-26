local raw = trxc.ui
local api = trx.api

require("trx.config")
require("trx.signal")
require("trx.ui")
require("trx.ui.primitive")
require("trx.math")

local primitive = trx.ui.primitive

-------------------------------------------------------------------------------
-- The widgets
--
-- A widget is created once and lives until its script releases it. Signals
-- invalidate cached measurements only when the values the widget named change.
--
-- Each region has at most one root, so a script's widgets are laid out as one
-- group after the engine's own UI for that region.
-------------------------------------------------------------------------------

local function value_of(value)
  if type(value) == "table" and value.get ~= nil then
    return value:get()
  end
  return value
end

local W = {}
W.__index = W

-- Recompute the widget's size only after something invalidates it.
function W:measure()
  if self._size == nil then
    local w, h = self:on_measure()
    self._size = { w = w or 0, h = h or 0 }
  end
  return self._size.w, self._size.h
end

function W:wake()
  self._size = nil
  if self._parent ~= nil then
    self._parent:wake()
  end
  return self
end

function W:is_shown()
  return value_of(self.shown) ~= false
end

-- Register the signals that invalidate the widget's cached size. Any signal a
-- widget reads should be listed here.
function W:wakes_on(...)
  local listeners = rawget(self, "_wakers")
  if listeners == nil then
    listeners = {}
    rawset(self, "_wakers", listeners)
  end
  for _, signal in ipairs({ ... }) do
    if type(signal) == "table" and signal.on ~= nil then
      listeners[#listeners + 1] = signal:on(function()
        self:wake()
      end)
    end
  end
  return self
end

-- Detach the widget from every signal it registered. Signals keep references
-- to their listeners, so release temporary widgets when they leave the screen.
function W:release()
  local listeners = rawget(self, "_wakers")
  if listeners == nil then
    return false
  end
  for _, listener in ipairs(listeners) do
    listener:detach()
  end
  rawset(self, "_wakers", nil)
  for _, child in ipairs(rawget(self, "children") or {}) do
    child:release()
  end
  return true
end

function W:paint(x, y, w, h)
  -- Hidden widgets keep their room but draw nothing. Widgets that are not shown
  -- keep no room at all.
  if self:is_shown() and value_of(self.hidden) ~= true then
    self:on_paint(x, y, w, h)
  end
end

local Widget = api.type("ui.Widget", {
  description = [[
A reusable UI element drawn over the game.

A widget holds its own state. Give it signals instead of fixed values, then
register those signals with `trx.ui.Widget:wakes_on`. The widget remeasures
only when a registered signal changes.

Register every signal that the widget reads. Otherwise the widget can keep a
stale cached size.]],

  methods = {
    wakes_on = {
      description = [[
Registers the signals that invalidate the widget's cached size.

When one of these signals changes, the widget and its parents are measured
again on the next layout pass.]],
      params = {
        {
          name = "...",
          type = "signal.Signal",
          description = "The signals the widget reads.",
        },
      },
      returns = {
        type = "ui.Widget",
        description = "The same widget, for method chaining.",
      },
      impl = W.wakes_on,
    },
    wake = {
      description = "Invalidates the widget's cached size manually.",
      returns = { type = "ui.Widget", description = "The same widget." },
      impl = W.wake,
    },
    measure = {
      description = "How much room the widget wants.",
      returns = {
        { type = "number", description = "The width, in canvas units." },
        { type = "number", description = "The height, in canvas units." },
      },
      impl = W.measure,
    },
    paint = {
      description = [[
Draws the widget in an assigned box.

`trx.ui.regions.place` calls this automatically. Custom layout code can call it
during `trx.events.on_ui_paint`.]],
      params = {
        { name = "x", type = "number", description = "The left edge." },
        { name = "y", type = "number", description = "The top edge." },
        {
          name = "w",
          type = "number",
          description = "The width it was given.",
        },
        {
          name = "h",
          type = "number",
          description = "The height it was given.",
        },
      },
      impl = W.paint,
    },
    is_shown = {
      description = [[
Returns whether the widget participates in layout.

A widget that is not shown keeps no room and leaves no gap. A hidden widget
keeps its room but draws nothing.]],
      returns = { type = "boolean", description = "Whether it draws." },
      impl = W.is_shown,
    },
    release = {
      description = [[
Detaches the widget and its children from registered signals.

Signals keep references to their listeners. Release temporary widgets when they
are no longer needed. Remove a placed widget from its region before releasing
it.]],
      returns = {
        type = "boolean",
        description = "Whether it was still listening to anything.",
      },
      impl = W.release,
    },
  },
})

local function new_widget(settings, on_measure, on_paint)
  local self = setmetatable(settings or {}, Widget)
  self.on_measure = on_measure
  self.on_paint = on_paint
  self:wakes_on(self.shown)
  return self
end

-- Lazily expose player scale settings as signals, so widgets that depend on
-- them are remeasured when the settings change.
local function scale_signal(key)
  local held = nil
  return function()
    if held == nil then
      held = trx.signal.config(key)
    end
    return held
  end
end

local text_scale = scale_signal("ui.text_scale")
local bar_scale = scale_signal("ui.bar_scale")

api.namespace("ui.widgets", {
  description = [[
The widgets a script builds its screen from.

A widget is created once and kept. Give it signals instead of fixed values, then
register those signals with `trx.ui.Widget:wakes_on`.

Put a widget on screen with `trx.ui.regions.place`.]],
})

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

api.define("ui.widgets.Bar", {
  description = [[
One of the game's bars, drawn with the player's bar settings.

The bar uses the same theme, border, and fill bands as the engine UI. Use a
signal for a fill value that changes.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The bar settings.",
      fields = {
        {
          name = "type",
          type = "ui.BarType",
          description = "The bar theme to use.",
        },
        {
          name = "value",
          type = "any",
          description = "The fill amount from 0 to 1, or a signal that holds it.",
        },
        {
          name = "w",
          type = "number",
          optional = true,
          description = "The width, in canvas units. The game's own by default.",
        },
        {
          name = "h",
          type = "number",
          optional = true,
          description = "The height, in canvas units. The game's own by default.",
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the bar is shown, or a signal that holds that value.",
        },
      },
    },
  },
  returns = { type = "ui.Widget", description = "The bar." },
  impl = function(settings)
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
    -- ramp entries, while flat bars use one ramp entry per band. Each band
    -- starts exactly where the previous one ends.
    local function bands(y, h, count)
      local out = {}
      for i = 0, count - 1 do
        local top = y + h * i / count
        local bottom = y + h * (i + 1) / count
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
      local fx, fy = at(x2), at(y2)
      local fh = at(y0 + h_px - 2 * edge) - fy
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
      for _, band in ipairs(bands(fy, fh, count)) do
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
  end,
})

api.define("ui.widgets.Resize", {
  description = [[
Gives a child widget an explicit size.

Use h_bars when a widget must match the height of the game's bars after the
player's bar scale is applied.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The resize settings.",
      fields = {
        {
          name = "child",
          type = "ui.Widget",
          description = "The child widget.",
        },
        {
          name = "w",
          type = "number",
          optional = true,
          description = "The width, in canvas units. Its own by default.",
        },
        {
          name = "h",
          type = "number",
          optional = true,
          description = "The height, in canvas units. Its own by default.",
        },
        {
          name = "h_bars",
          type = "number",
          optional = true,
          description = "The height in bar heights. This overrides the plain height.",
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the resized widget is shown, or a signal that holds that value.",
        },
      },
    },
  },
  returns = { type = "ui.Widget", description = "The resized widget." },
  impl = function(settings)
    local BAR_HEIGHT = 18

    local self = new_widget(settings, function(w)
      local cw, ch = w.child:measure()
      if w.w ~= nil then
        cw = w.w
      end
      if w.h_bars ~= nil then
        ch = w.h_bars * BAR_HEIGHT * raw.bar_scale()
      elseif w.h ~= nil then
        ch = w.h
      end
      return cw, ch
    end, function(w, x, y, bw, bh)
      w.child:paint(x, y, bw, bh)
    end)
    self.child._parent = self
    return self:wakes_on(bar_scale())
  end,
})

api.define("ui.widgets.Row", {
  description = [[
A widget with a left and right arrow beside a child widget.

Unlit arrows stay hidden but keep their room, so the child widget does not move
when arrows appear or disappear.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The row settings.",
      fields = {
        {
          name = "child",
          type = "ui.Widget",
          description = "The child widget placed between the arrows.",
        },
        {
          name = "left",
          type = "any",
          description = "Whether the left arrow is lit, or a signal that holds that value.",
        },
        {
          name = "right",
          type = "any",
          description = "Whether the right arrow is lit, or a signal that holds that value.",
        },
        {
          name = "spacing",
          type = "number",
          optional = true,
          description = "The gap between each arrow and the child widget. 15 by default.",
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the row is shown, or a signal that holds that value.",
        },
      },
    },
  },
  returns = { type = "ui.Widget", description = "The row." },
  impl = function(settings)
    -- The arrow's hidden state is the inverse of the caller's lit state.
    local function not_of(value)
      if type(value) == "table" and value.get ~= nil then
        return ~value
      end
      return value ~= true
    end

    local function arrow(glyph, lit)
      local self = trx.ui.widgets.Label({ text = glyph })
      self.hidden = not_of(lit)
      return self
    end

    local row = trx.ui.widgets.Stack({
      orientation = trx.ui.Orientation.HORIZONTAL,
      v_align = trx.ui.VAlign.CENTER,
      spacing = settings.spacing or 15,
      shown = settings.shown,
      children = {
        arrow("\\{button left}", settings.left),
        settings.child,
        arrow("\\{button right}", settings.right),
      },
    })
    return row
  end,
})

api.define("ui.widgets.Stack", {
  description = [[
Lays widgets out one after another.

Widgets that are not shown take no room and leave no gap.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The stack settings.",
      fields = {
        {
          name = "children",
          type = "table",
          list = true,
          description = "The widgets, in the order they are laid out.",
        },
        {
          name = "orientation",
          type = "ui.Orientation",
          optional = true,
          description = "The layout direction. Vertical by default.",
        },
        {
          name = "spacing",
          type = "number",
          optional = true,
          description = "The gap between one and the next. `0` by default.",
        },
        {
          name = "align",
          type = "ui.HAlign",
          optional = true,
          description = "Where a narrower child sits in a vertical stack.",
        },
        {
          name = "v_align",
          type = "ui.VAlign",
          optional = true,
          description = "Where a shorter child sits in a horizontal stack.",
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the stack is shown, or a signal that holds that value.",
        },
      },
    },
  },
  returns = { type = "ui.Widget", description = "The stack." },
  impl = function(settings)
    local function is_horizontal(w)
      return w.orientation == trx.ui.Orientation.HORIZONTAL
    end

    local self = new_widget(settings, function(w)
      local along, across, shown = 0, 0, 0
      for _, child in ipairs(w.children) do
        if child:is_shown() then
          local cw, ch = child:measure()
          if is_horizontal(w) then
            along, across = along + cw, math.max(across, ch)
          else
            along, across = along + ch, math.max(across, cw)
          end
          shown = shown + 1
        end
      end
      along = along + math.max(0, shown - 1) * (w.spacing or 0)
      if is_horizontal(w) then
        return along, across
      end
      return across, along
    end, function(w, x, y, bw, bh)
      local at = is_horizontal(w) and x or y
      for _, child in ipairs(w.children) do
        if child:is_shown() then
          local cw, ch = child:measure()
          if is_horizontal(w) then
            local offset = 0
            if w.v_align == trx.ui.VAlign.CENTER then
              offset = (bh - ch) / 2
            elseif w.v_align == trx.ui.VAlign.BOTTOM then
              offset = bh - ch
            end
            child:paint(at, y + offset, cw, ch)
            at = at + cw + (w.spacing or 0)
          else
            local offset = 0
            if w.align == trx.ui.HAlign.CENTER then
              offset = (bw - cw) / 2
            elseif w.align == trx.ui.HAlign.RIGHT then
              offset = bw - cw
            end
            child:paint(x + offset, at, cw, ch)
            at = at + ch + (w.spacing or 0)
          end
        end
      end
    end)

    -- Child widgets wake their parent stack when their size changes.
    for _, child in ipairs(self.children) do
      child._parent = self
    end
    return self
  end,
})

-- The sprite each character of a Digits widget draws, and how far the pen moves
-- over it. A colon and a full stop are narrower than a digit and are drawn
-- slightly back, which is how the engine has always spaced them.
local DIGIT_ADVANCE = 20

local DIGIT_GLYPHS = {
  [":"] = { sprite = 10, offset = -6, advance = 14 },
  ["."] = { sprite = 11, offset = -6, advance = 14 },
  ["T"] = { sprite = 12, offset = -6, advance = 16, drop = 1, mark = true },
  ["s"] = { sprite = 13, offset = -6, advance = 14, nudge = -4 },
  ["-"] = {},
  [" "] = { advance = 8 },
}

for digit = 0, 9 do
  DIGIT_GLYPHS[tostring(digit)] = { sprite = digit }
end

local function digit_glyphs(text)
  local glyphs = {}
  for i = 1, #text do
    local char = text:sub(i, i)
    local glyph = DIGIT_GLYPHS[char]
    if glyph == nil then
      error(("trx.ui.widgets.Digits cannot draw %q"):format(char), 2)
    end
    glyphs[#glyphs + 1] = glyph
  end
  return glyphs
end

-- The characters sit around the point each is drawn at, and reach different
-- distances above it. Report the highest, so that a line of them can be drawn
-- from the top of the box it was given.
local function digit_rise(object, glyphs)
  local rise = 0
  for _, glyph in ipairs(glyphs) do
    if glyph.sprite ~= nil then
      local _, y0 = primitive.sprite_bounds(object, glyph.sprite)
      rise = math.min(rise, y0)
    end
  end
  return rise
end

-- The digits are drawn from a sprite object the level may never have loaded,
-- which is every level outside the gym.
local function digits_loaded(widget)
  return primitive.sprite_count(widget.object) > 0
end

api.define("ui.widgets.Digits", {
  description = [[
A line of text drawn from an object's sprites, one sprite per character.

The object supplies the ten digits, then a colon, a full stop, a `T` and an
`s`, in that order, which is how the assault course digits are laid out. A
space and a dash move the pen without drawing. <!--noref: s-->

The widget measures nothing where the level did not load the object, so a
script can keep it on screen for a level that has no digits.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The digit settings.",
      fields = {
        {
          name = "object",
          type = "catalog.objects",
          description = "The sprite object to draw the characters from.",
        },
        {
          name = "text",
          type = "any",
          description = "The text, or a signal carrying it.",
        },
        {
          name = "color",
          type = "any",
          description = "What color to draw the characters in, or a "
            .. "signal carrying one.",
        },
        {
          name = "color_bottom",
          type = "any",
          optional = true,
          description = "The color the characters fade to down their "
            .. "height. The main color by default, which draws them flat.",
        },
        {
          name = "mark_color",
          type = "any",
          optional = true,
          description = "What color to draw the `T` in. The main color "
            .. "by default.",
        },
        {
          name = "mark_color_bottom",
          type = "any",
          optional = true,
          description = "The color the `T` fades to. Its own color by default.",
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the digits are shown, or a signal that holds "
            .. "that value.",
        },
      },
    },
  },
  returns = { type = "ui.Widget", description = "The digits." },
  examples = {
    [[trx.ui.widgets.Digits({
  object = trx.catalog.objects.assault_digits,
  text = timer:map(format_time),
  color = trx.math.color("ffffff"),
})]],
  },
  impl = function(settings)
    local self = new_widget(settings, function(w)
      if not digits_loaded(w) then
        return 0, 0
      end
      local scale = raw.text_scale()
      local glyphs = digit_glyphs(tostring(value_of(w.text)))
      local width, bottom = 0, 0
      for _, glyph in ipairs(glyphs) do
        width = width + (glyph.offset or 0) + (glyph.advance or DIGIT_ADVANCE)
        if glyph.sprite ~= nil then
          local _, _, _, y1 = primitive.sprite_bounds(w.object, glyph.sprite)
          bottom = math.max(bottom, y1 + (glyph.drop or 0))
        end
      end
      return width * scale, (bottom - digit_rise(w.object, glyphs)) * scale
    end, function(w, x, y)
      if not digits_loaded(w) then
        return
      end
      local scale = raw.text_scale()
      local glyphs = digit_glyphs(tostring(value_of(w.text)))
      local top = y - digit_rise(w.object, glyphs) * scale
      local pen = x
      for _, glyph in ipairs(glyphs) do
        pen = pen + (glyph.offset or 0) * scale
        if glyph.sprite ~= nil then
          local top_color = value_of(w.color)
          local bottom_color = value_of(w.color_bottom) or top_color
          if glyph.mark and w.mark_color ~= nil then
            top_color = value_of(w.mark_color)
            bottom_color = value_of(w.mark_color_bottom) or top_color
          end
          primitive.gradient_sprite(
            w.object,
            glyph.sprite,
            pen + (glyph.nudge or 0) * scale,
            top + (glyph.drop or 0) * scale,
            0,
            scale,
            top_color,
            top_color,
            bottom_color,
            bottom_color
          )
        end
        pen = pen + (glyph.advance or DIGIT_ADVANCE) * scale
      end
    end)
    return self:wakes_on(self.text, text_scale())
  end,
})
