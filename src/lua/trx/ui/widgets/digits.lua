local raw = trxc.ui
require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget
local text_scale = base.text_scale

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
  local rise = 0.0
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

---@class (exact) trx.ui.widgets.Digits.settings
---@field object trx.catalog.objects The sprite object to draw the characters from.
---@field text any The text, or a signal carrying it.
---@field color any What color to draw the characters in, or a signal carrying one.
---@field color_bottom? any The color the characters fade to down their height. The main color by default, which draws them flat.
---@field mark_color? any What color to draw the `T` in. The main color by default.
---@field mark_color_bottom? any The color the `T` fades to. Its own color by default.
---@field shown? any Whether the digits are shown, or a signal that holds that value.

---A line of text drawn from an object's sprites, one sprite per character.
---
---The object supplies the ten digits, then a colon, a full stop, a `T` and an
---`s`, in that order, which is how the assault course digits are laid out. A
---space and a dash move the pen without drawing. <!--noref: s-->
---
---The widget measures nothing where the level did not load the object, so a
---script can keep it on screen for a level that has no digits.
---
---```lua
---trx.ui.widgets.Digits({
---  object = trx.catalog.objects.assault_digits,
---  text = timer:map(format_time),
---  color = trx.math.color("ffffff"),
---})
---```
---@param settings trx.ui.widgets.Digits.settings The digit settings.
---@return trx.ui.Widget # The digits.
function widgets.Digits(settings)
  local self = new_widget(settings, function(w)
    if not digits_loaded(w) then
      return 0, 0
    end
    local scale = raw.text_scale()
    local glyphs = digit_glyphs(tostring(value_of(w.text)))
    local width, bottom = 0, 0.0
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
end
