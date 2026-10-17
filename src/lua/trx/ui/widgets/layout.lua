local raw = trxc.ui
require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive

---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local W = base.W
local new_widget = base.new_widget
local hold = base.hold
local forget = base.forget
local value_of = base.value_of

---@class (exact) trx.ui.widgets.Resize.settings: trx.ui.widgets.settings
---@field child trx.ui.Widget The child widget.
---@field w? any The width, in canvas units, or a signal that holds it. Its own by default.
---@field h? any The height, in canvas units, or a signal that holds it. Its own by default.
---@field scaled? any Whether `w` and `h` are at the default text size, and follow the text scale as `trx.ui.widgets.Spacer` does, or a signal that holds that value. `false` by default. <!--noref: w, h-->

---Gives a child widget an explicit size.
---@param settings trx.ui.widgets.Resize.settings The resize settings.
---@return trx.ui.Widget # The resized widget.
function widgets.Resize(settings)
  local self = new_widget(settings, function(w)
    local cw, ch = w.child:measure()
    local scale = value_of(w.scaled) and raw.drawn_text_scale() or 1
    local fixed_w = value_of(w.w)
    local fixed_h = value_of(w.h)
    fixed_w = fixed_w ~= nil and fixed_w * scale or nil
    fixed_h = fixed_h ~= nil and fixed_h * scale or nil
    if fixed_w ~= nil then
      cw = fixed_w
    end
    if fixed_h ~= nil then
      ch = fixed_h
    end
    return cw, ch
  end, function(w, x, y, bw, bh)
    w.child:paint(x, y, bw, bh)
  end)
  hold(self.child, self)
  return self
end

---@class (exact) trx.ui.widgets.Blank.settings: trx.ui.widgets.settings
---@field child trx.ui.Widget The child widget.
---@field when any Whether to leave the child blank, or a signal that holds that value.

---Leaves a child widget blank while a condition holds: it keeps its room, so
---nothing around it moves, and draws nothing.
---
---```lua
---trx.ui.widgets.Blank({ when = ~can_step_left, child = left_arrow })
---```
---@param settings trx.ui.widgets.Blank.settings The settings.
---@return trx.ui.Widget # The widget.
function widgets.Blank(settings)
  local self = new_widget(settings, function(w)
    return w.child:measure()
  end, function(w, x, y, bw, bh)
    if value_of(w.when) ~= true then
      w.child:paint(x, y, bw, bh)
    end
  end)
  hold(self.child, self)
  return self
end

---@class (exact) trx.ui.widgets.Spacer.settings: trx.ui.widgets.settings
---@field w? any The width, in canvas units at the default text size, or a signal that holds it. `0` by default.
---@field h? any The height, in canvas units at the default text size, or a signal that holds it. `0` by default.

---Keeps empty room, which follows the text scale the same way the widgets
---beside it do. With no size, it takes no room, which suits a layer that shows
---nothing yet.
---
---```lua
---trx.ui.widgets.Stack({
---  children = { title, trx.ui.widgets.Spacer({ h = 5 }), body },
---})
---```
---@param settings? trx.ui.widgets.Spacer.settings The spacer settings.
---@return trx.ui.Widget # The spacer.
function widgets.Spacer(settings)
  local self = new_widget(settings or {}, function(w)
    local scale = raw.drawn_text_scale()
    return (value_of(w.w) or 0) * scale, (value_of(w.h) or 0) * scale
  end, function() end)
  return self
end

---@class (exact) trx.ui.widgets.Pad.settings: trx.ui.widgets.settings
---@field child trx.ui.Widget The child widget.
---@field x? any The margin at the left and the right, or a signal that holds it. `0` by default.
---@field y? any The margin at the top and the bottom, or a signal that holds it. `0` by default.

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
    local px = (value_of(w.x) or 0) * scale
    local py = (value_of(w.y) or 0) * scale
    return cw + px + px, ch + py + py
  end, function(w, x, y, bw, bh)
    local scale = raw.drawn_text_scale()
    local px = (value_of(w.x) or 0) * scale
    local py = (value_of(w.y) or 0) * scale
    w.child:paint(x + px, y + py, bw - px - px, bh - py - py)
  end)
  hold(self.child, self)
  return self
end

---@class (exact) trx.ui.widgets.Frame.settings: trx.ui.widgets.settings
---@field child trx.ui.Widget The child widget.
---@field style? any Which `trx.ui.FrameStyle` to draw, or a signal that holds it. `false` draws no frame, as for a row that the cursor is not on. The dialog box by default.
---@field z? any The draw order, or a signal that holds it. `160` by default, which is behind text.

---Draws one of the game's frames behind a child widget.
---
---The frame takes the whole box the child asks for, so pad the child where the
---text would otherwise sit against the edge. While `trx.ui.frames_shown` is
---off, no frame is drawn, and the child of the box around the option the
---player is on draws its text in the picked colour instead.
---@param settings trx.ui.widgets.Frame.settings The frame settings.
---@return trx.ui.Widget # The framed widget.
function widgets.Frame(settings)
  local self = new_widget(settings, function(w)
    return w.child:measure()
  end, function(w, x, y, bw, bh)
    local style = value_of(w.style)
    if not trx.ui.frames_shown then
      if style == trx.ui.FrameStyle.SELECTED then
        local previous = raw.set_text_color(base.PICKED_COLOR)
        w.child:paint(x, y, bw, bh)
        raw.set_text_color(previous)
      else
        w.child:paint(x, y, bw, bh)
      end
      return
    end
    if style ~= false then
      primitive.panel(
        x,
        y,
        value_of(w.z) or base.FRAME_Z,
        bw,
        bh,
        style or trx.ui.FrameStyle.DIALOG
      )
    end
    w.child:paint(x, y, bw, bh)
  end)
  hold(self.child, self)
  return self
end

---@class (exact) trx.ui.widgets.Fit.settings: trx.ui.widgets.settings
---@field child trx.ui.Widget The child widget.

---Shrinks a child widget until it is within the screen.
---
---Text keeps the size the player chose while it fits, and everything below
---this widget is drawn smaller where it does not. A dialog that has to hold a
---fixed body on a small screen wants this; a line of text that can simply wrap
---does not.
---@param settings trx.ui.widgets.Fit.settings The fit settings.
---@return trx.ui.Widget # The fitted widget.
function widgets.Fit(settings)
  -- The factor is the room the screen leaves against what the widget wants,
  -- both at the size the player chose, so nothing else has to agree on how
  -- that size is folded in.
  local function factor_of(nw, nh)
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

  -- The tree is measured at the size the player chose, and where it does not
  -- fit, measured again under the factor, because text rounds to whole
  -- pixels: its size at the smaller size is not its size at the larger one
  -- times the factor. The sizes under the factor are kept for painting.
  local self = new_widget(settings, function(w)
    forget(w.child)
    local nw, nh = w.child:measure()
    w._factor = factor_of(nw, nh)
    if w._factor >= 1.0 then
      return nw, nh
    end
    raw.push_text_scale(w._factor)
    forget(w.child)
    local cw, ch = w.child:measure()
    raw.pop_text_scale()
    return cw, ch
  end, function(w, x, y, bw, bh)
    if w._factor == nil or w._factor >= 1.0 then
      w.child:paint(x, y, bw, bh)
      return
    end
    raw.push_text_scale(w._factor)
    w.child:paint(x, y, bw, bh)
    raw.pop_text_scale()
  end)
  hold(self.child, self)
  return self
end

-- What a stack reads for each axis: where a child box keeps its position and
-- its size, the setting that aligns along the axis, and the values of that
-- setting.
local AXES = {
  x = {
    i = 1,
    pos = 2,
    size = 4,
    setting = "align",
    center = trx.ui.HAlign.CENTER,
    far = trx.ui.HAlign.RIGHT,
    span = trx.ui.HAlign.SPAN,
    distribute = trx.ui.HAlign.DISTRIBUTE,
  },
  y = {
    i = 2,
    pos = 3,
    size = 5,
    setting = "v_align",
    center = trx.ui.VAlign.CENTER,
    far = trx.ui.VAlign.BOTTOM,
    span = trx.ui.VAlign.SPAN,
    distribute = trx.ui.VAlign.DISTRIBUTE,
  },
}

---@class (exact) trx.ui.widgets.Stack.settings: trx.ui.widgets.settings
---@field children trx.ui.Widget[] The widgets, in the order they are laid out.
---@field orientation? any The `trx.ui.Orientation` to lay out in, or a signal that holds it. Vertical by default.
---@field spacing? any The gap between one and the next, in canvas units at the default text size, or a signal that holds it. `0` by default.
---@field align? any The `trx.ui.HAlign`, or a signal that holds it, that says where the children sit across the width. In a vertical stack, it places each narrower child. In a horizontal stack, it places the whole row, and `trx.ui.HAlign.DISTRIBUTE` spreads the children out. Left by default.
---@field v_align? any The `trx.ui.VAlign`, or a signal that holds it, that says where the children sit down the height. In a horizontal stack, it places each shorter child. In a vertical stack, it places the whole column, and `trx.ui.VAlign.DISTRIBUTE` spreads the children out. Top by default.

---Lays widgets out one after another.
---
---Widgets that are not shown take no room and leave no gap.
---@param settings trx.ui.widgets.Stack.settings The stack settings.
---@return trx.ui.Widget # The stack.
function widgets.Stack(settings)
  local function is_horizontal(w)
    return value_of(w.orientation) == trx.ui.Orientation.HORIZONTAL
  end

  local function measure(w)
    -- The boxes follow the alignment too, which only laying out reads.
    value_of(w.align)
    value_of(w.v_align)
    local along = is_horizontal(w) and AXES.x or AXES.y
    local across = along == AXES.x and AXES.y or AXES.x
    local spacing = (value_of(w.spacing) or 0) * raw.drawn_text_scale()
    local along_total, across_total = 0.0, 0.0
    local first = true
    for _, child in ipairs(w.children) do
      if child:is_visible() then
        local size = { child:measure() }
        if not first then
          along_total = along_total + spacing
        end
        first = false
        along_total = along_total + size[along.i]
        across_total = math.max(size[across.i], across_total)
      end
    end
    if along == AXES.x then
      return along_total, across_total
    end
    return across_total, along_total
  end

  -- Works out the box of each child shown, for the box that the stack is
  -- given.
  local function lay_out(w, x, y, bw, bh)
    local boxes = { x = x, y = y, w = bw, h = bh }
    local along = is_horizontal(w) and AXES.x or AXES.y
    local across = along == AXES.x and AXES.y or AXES.x
    local along_align = value_of(w[along.setting])
    local across_align = value_of(w[across.setting])
    local origin = { x, y }
    local room = { bw, bh }
    local plain_spacing = value_of(w.spacing) or 0
    local spacing = plain_spacing * raw.drawn_text_scale()

    local shown = {}
    local total = 0.0
    for _, child in ipairs(w.children) do
      if child:is_visible() then
        local size = { child:measure() }
        shown[#shown + 1] = { child, size }
        total = total + size[along.i]
      end
    end

    -- Spare room along the axis goes into the gaps, or in front of the
    -- children, depending on what the stack asked for.
    local at = origin[along.i]
    if along_align == along.distribute then
      -- The engine shares out what the children and their plain gaps leave,
      -- counted apart rather than as they were measured.
      local gaps = math.max(0, #shown - 1)
      local leftover = room[along.i]
        - (total + plain_spacing * gaps * raw.drawn_text_scale())
      if gaps > 0 and leftover > 0 then
        spacing = spacing + leftover / gaps
      end
    elseif along_align == along.center or along_align == along.far then
      local spare = room[along.i] - select(along.i, w:measure())
      at = at + (along_align == along.center and spare * 0.5 or spare)
    end

    for _, entry in ipairs(shown) do
      local child, size = entry[1], entry[2]
      local box = { child, 0, 0, size[1], size[2] }
      local across_at = origin[across.i]
      local across_room = room[across.i]
      if across_align == across.span then
        box[across.size] = math.max(size[across.i], across_room)
      elseif across_align == across.center then
        across_at = across_at + (across_room - size[across.i]) * 0.5
      elseif across_align == across.far then
        across_at = across_at + across_room - size[across.i]
      end
      box[along.pos] = at
      box[across.pos] = across_at
      boxes[#boxes + 1] = box
      at = at + size[along.i] + spacing
    end
    return boxes
  end

  -- The boxes are kept until the stack wakes or is given another box, so a
  -- stack that nothing changes lays nothing out as it paints.
  local self = new_widget(settings, measure, function(w, x, y, bw, bh)
    local boxes = w._boxes
    if
      boxes == nil
      or boxes.x ~= x
      or boxes.y ~= y
      or boxes.w ~= bw
      or boxes.h ~= bh
    then
      boxes = lay_out(w, x, y, bw, bh)
      w._boxes = boxes
    end
    for _, box in ipairs(boxes) do
      box[1]:paint(box[2], box[3], box[4], box[5])
    end
  end)

  for _, child in ipairs(self.children) do
    hold(child, self)
  end
  return self
end
