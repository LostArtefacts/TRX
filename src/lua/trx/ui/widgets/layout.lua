local raw = trxc.ui
require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local W = base.W
local new_widget = base.new_widget
local value_of = base.value_of
local f32 = base.f32
local text_scale = base.text_scale
local bar_scale = base.bar_scale

---@class (exact) trx.ui.widgets.Resize.settings
---@field child trx.ui.Widget The child widget.
---@field w? number The width, in canvas units. Its own by default.
---@field h? number The height, in canvas units. Its own by default.
---@field h_bars? number The height in bar heights. This overrides the plain height.
---@field shown? any Whether the resized widget is shown, or a signal that holds that value.

---Gives a child widget an explicit size.
---
---Use h_bars when a widget must match the height of the game's bars after the
---player's bar scale is applied.
---@param settings trx.ui.widgets.Resize.settings The resize settings.
---@return trx.ui.Widget # The resized widget.
function widgets.Resize(settings)
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
end

---@class (exact) trx.ui.widgets.Pad.settings
---@field child trx.ui.Widget The child widget.
---@field x? number The margin at the left and the right. `0` by default.
---@field y? number The margin at the top and the bottom. `0` by default.
---@field shown? any Whether the padded widget is shown, or a signal that holds that value.

---Keeps a margin around a child widget.
---
---The margin is in canvas units at the default text size, and follows the text
---scale the same way the widgets inside it do.
---@param settings trx.ui.widgets.Pad.settings The padding settings.
---@return trx.ui.Widget # The padded widget.
function widgets.Pad(settings)
  local self = new_widget(settings, function(w)
    local cw, ch = w.child:measure()
    local scale = raw.drawn_text_scale()
    local px = f32((w.x or 0) * scale)
    local py = f32((w.y or 0) * scale)
    return f32(cw + f32(px + px)), f32(ch + f32(py + py))
  end, function(w, x, y, bw, bh)
    local scale = raw.drawn_text_scale()
    local px = f32((w.x or 0) * scale)
    local py = f32((w.y or 0) * scale)
    w.child:paint(
      f32(x + px),
      f32(y + py),
      f32(f32(bw - px) - px),
      f32(f32(bh - py) - py)
    )
  end)
  self.child._parent = self
  return self:wakes_on(text_scale())
end

---@class (exact) trx.ui.widgets.Frame.settings
---@field child trx.ui.Widget The child widget.
---@field style? trx.ui.FrameStyle Which frame to draw. The dialog box by default.
---@field z? integer The draw order. `160` by default, which is behind text.
---@field shown? any Whether the framed widget is shown, or a signal that holds that value.

---Draws one of the game's frames behind a child widget.
---
---The frame takes the whole box the child asks for, so pad the child where the
---text would otherwise sit against the edge.
---@param settings trx.ui.widgets.Frame.settings The frame settings.
---@return trx.ui.Widget # The framed widget.
function widgets.Frame(settings)
  local self = new_widget(settings, function(w)
    return w.child:measure()
  end, function(w, x, y, bw, bh)
    primitive.panel(
      x,
      y,
      w.z or 160,
      bw,
      bh,
      w.style or trx.ui.FrameStyle.DIALOG
    )
    w.child:paint(x, y, bw, bh)
  end)
  self.child._parent = self
  return self
end

---@class (exact) trx.ui.widgets.Fit.settings
---@field child trx.ui.Widget The child widget.
---@field shown? any Whether the fitted widget is shown, or a signal that holds that value.

---Shrinks a child widget until it is within the screen.
---
---Text keeps the size the player chose while it fits, and everything below
---this widget is drawn smaller where it does not. A dialog that has to hold a
---fixed body on a small screen wants this; a line of text that can simply wrap
---does not.
---@param settings trx.ui.widgets.Fit.settings The fit settings.
---@return trx.ui.Widget # The fitted widget.
function widgets.Fit(settings)
  -- The size at the player's own text size, which the factor is worked out
  -- from. Measuring it again under the factor would fold the factor in
  -- twice, and the widget would shrink further every frame.
  local function natural(w)
    if w._natural == nil then
      local cw, ch = w.child:measure()
      w._natural = { w = cw, h = ch }
    end
    return w._natural.w, w._natural.h
  end

  -- The factor is the room the screen leaves against what the widget wants,
  -- both at the size the player chose, so nothing else has to agree on how
  -- that size is folded in.
  local function factor_of(w)
    local nw, nh = natural(w)
    local canvas = trx.ui.canvas
    local safe = trx.ui.safe_area
    -- The margin the screen keeps at its edges, which the safe area is the
    -- canvas less twice over. What the rest of the interface has taken is
    -- not part of it: a panel that shrank because something else is on
    -- screen would change size as that comes and goes.
    local margin = (canvas.width - safe.width) / 2
    local room_w = canvas.width - 2 * margin
    local room_h = canvas.height - 2 * margin
    local factor = 1.0
    if nw > 0 and room_w > 0 then
      factor = math.min(factor, room_w / nw)
    end
    if nh > 0 and room_h > 0 then
      factor = math.min(factor, room_h / nh)
    end
    return factor
  end

  -- The child is measured again under the factor, because text rounds to
  -- whole pixels: its size at the smaller size is not its size at the
  -- larger one times the factor.
  local function under_factor(w, body)
    local factor = factor_of(w)
    if factor >= 1.0 then
      return body()
    end
    raw.push_text_scale(factor)
    w.child._size = nil
    local a, b = body()
    raw.pop_text_scale()
    w.child._size = nil
    return a, b
  end

  local self = new_widget(settings, function(w)
    return under_factor(w, function()
      return w.child:measure()
    end)
  end, function(w, x, y, bw, bh)
    under_factor(w, function()
      w.child:paint(x, y, bw, bh)
    end)
  end)
  self.child._parent = self
  -- A child that changes drops the natural size along with the cached one.
  local wake = W.wake
  self.wake = function(me)
    me._natural = nil
    return wake(me)
  end
  return self:wakes_on(text_scale())
end

---@class (exact) trx.ui.widgets.Row.settings
---@field child trx.ui.Widget The child widget placed between the arrows.
---@field left any Whether the left arrow is lit, or a signal that holds that value.
---@field right any Whether the right arrow is lit, or a signal that holds that value.
---@field spacing? number The gap between each arrow and the child widget. 15 by default.
---@field shown? any Whether the row is shown, or a signal that holds that value.

---A widget with a left and right arrow beside a child widget.
---
---Unlit arrows stay hidden but keep their room, so the child widget does not
---move when arrows appear or disappear.
---@param settings trx.ui.widgets.Row.settings The row settings.
---@return trx.ui.Widget # The row.
function widgets.Row(settings)
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
end

---@class (exact) trx.ui.widgets.Stack.settings
---@field children table[] The widgets, in the order they are laid out.
---@field orientation? trx.ui.Orientation The layout direction. Vertical by default.
---@field spacing? number The gap between one and the next. `0` by default.
---@field align? trx.ui.HAlign Where a narrower child sits in a vertical stack.
---@field v_align? trx.ui.VAlign Where a shorter child sits in a horizontal stack.
---@field shown? any Whether the stack is shown, or a signal that holds that value.

---Lays widgets out one after another.
---
---Widgets that are not shown take no room and leave no gap.
---@param settings trx.ui.widgets.Stack.settings The stack settings.
---@return trx.ui.Widget # The stack.
function widgets.Stack(settings)
  local function is_horizontal(w)
    return w.orientation == trx.ui.Orientation.HORIZONTAL
  end

  local self = new_widget(settings, function(w)
    local along, across = 0.0, 0.0
    local spacing = f32((w.spacing or 0) * raw.drawn_text_scale())
    local first = true
    for _, child in ipairs(w.children) do
      if child:is_shown() then
        local cw, ch = child:measure()
        if not first then
          along = f32(along + spacing)
        end
        first = false
        if is_horizontal(w) then
          along, across = f32(along + cw), math.max(across, ch)
        else
          along, across = f32(along + ch), math.max(across, cw)
        end
      end
    end
    if is_horizontal(w) then
      return along, across
    end
    return across, along
  end, function(w, x, y, bw, bh)
    local horizontal = is_horizontal(w)
    local along_align = horizontal and w.align or w.v_align
    local across_align = horizontal and w.v_align or w.align
    local span = across_align
      == (horizontal and trx.ui.VAlign.SPAN or trx.ui.HAlign.SPAN)
    local center = across_align
      == (horizontal and trx.ui.VAlign.CENTER or trx.ui.HAlign.CENTER)
    local far = across_align
      == (horizontal and trx.ui.VAlign.BOTTOM or trx.ui.HAlign.RIGHT)

    local along, across = w:measure()
    if not horizontal then
      along, across = across, along
    end
    local box_along = horizontal and bw or bh
    local box_across = horizontal and bh or bw
    local spacing = f32((w.spacing or 0) * raw.drawn_text_scale())
    local shown = 0
    for _, child in ipairs(w.children) do
      if child:is_shown() then
        shown = shown + 1
      end
    end

    -- Spare room along the axis goes into the gaps, or in front of the
    -- children, depending on what the stack asked for.
    local spare = f32(box_along - along)
    local at = horizontal and x or y
    if
      along_align
      == (horizontal and trx.ui.HAlign.DISTRIBUTE or trx.ui.VAlign.DISTRIBUTE)
    then
      -- The engine shares out what the children and their plain gaps leave,
      -- counted apart rather than as they were measured.
      local total = 0.0
      for _, child in ipairs(w.children) do
        if child:is_shown() then
          local cw, ch = child:measure()
          total = f32(total + (horizontal and cw or ch))
        end
      end
      local gaps = math.max(0, shown - 1)
      local leftover = f32(
        box_along
          - f32(
            total + f32(f32((w.spacing or 0) * gaps) * raw.drawn_text_scale())
          )
      )
      if gaps > 0 and leftover > 0 then
        spacing = f32(spacing + f32(leftover / gaps))
      end
    elseif
      along_align
      == (horizontal and trx.ui.HAlign.CENTER or trx.ui.VAlign.CENTER)
    then
      at = f32(at + f32(spare * 0.5))
    elseif
      along_align
      == (horizontal and trx.ui.HAlign.RIGHT or trx.ui.VAlign.BOTTOM)
    then
      at = f32(at + spare)
    end

    local origin = horizontal and y or x
    for _, child in ipairs(w.children) do
      if child:is_shown() then
        local cw, ch = child:measure()
        local size_along = horizontal and cw or ch
        local size_across = horizontal and ch or cw
        local across_at = origin
        if span then
          size_across = math.max(size_across, box_across)
        elseif center then
          across_at = f32(origin + f32(f32(box_across - size_across) * 0.5))
        elseif far then
          across_at = f32(f32(origin + box_across) - size_across)
        end
        if horizontal then
          child:paint(at, across_at, size_along, size_across)
        else
          child:paint(across_at, at, size_across, size_along)
        end
        at = f32(f32(at + size_along) + spacing)
      end
    end
  end)

  -- Child widgets wake their parent stack when their size changes.
  for _, child in ipairs(self.children) do
    child._parent = self
  end
  return self
end
