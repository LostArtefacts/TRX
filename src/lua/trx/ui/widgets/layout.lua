local raw = trxc.ui
local api = trx.api
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive
local W = base.W
local new_widget = base.new_widget
local text_scale = base.text_scale
local bar_scale = base.bar_scale

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

api.define("ui.widgets.Pad", {
  description = [[
Keeps a margin around a child widget.

The margin is in canvas units at the default text size, and follows the text
scale the same way the widgets inside it do.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The padding settings.",
      fields = {
        {
          name = "child",
          type = "ui.Widget",
          description = "The child widget.",
        },
        {
          name = "x",
          type = "number",
          optional = true,
          description = "The margin at the left and the right. `0` by default.",
        },
        {
          name = "y",
          type = "number",
          optional = true,
          description = "The margin at the top and the bottom. `0` by default.",
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the padded widget is shown, or a signal that holds that value.",
        },
      },
    },
  },
  returns = { type = "ui.Widget", description = "The padded widget." },
  impl = function(settings)
    local self = new_widget(settings, function(w)
      local cw, ch = w.child:measure()
      local scale = raw.drawn_text_scale()
      return cw + 2 * (w.x or 0) * scale, ch + 2 * (w.y or 0) * scale
    end, function(w, x, y, bw, bh)
      local scale = raw.drawn_text_scale()
      local px = (w.x or 0) * scale
      local py = (w.y or 0) * scale
      w.child:paint(x + px, y + py, bw - 2 * px, bh - 2 * py)
    end)
    self.child._parent = self
    return self:wakes_on(text_scale())
  end,
})

api.define("ui.widgets.Frame", {
  description = [[
Draws one of the game's frames behind a child widget.

The frame takes the whole box the child asks for, so pad the child where the
text would otherwise sit against the edge.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The frame settings.",
      fields = {
        {
          name = "child",
          type = "ui.Widget",
          description = "The child widget.",
        },
        {
          name = "style",
          type = "ui.FrameStyle",
          optional = true,
          description = "Which frame to draw. The dialog box by default.",
        },
        {
          name = "z",
          type = "integer",
          optional = true,
          description = "The draw order. `160` by default, which is behind text.",
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the framed widget is shown, or a signal that holds that value.",
        },
      },
    },
  },
  returns = { type = "ui.Widget", description = "The framed widget." },
  impl = function(settings)
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
  end,
})

api.define("ui.widgets.Fit", {
  description = [[
Shrinks a child widget until it is within the screen.

Text keeps the size the player chose while it fits, and everything below this
widget is drawn smaller where it does not. A dialog that has to hold a fixed
body on a small screen wants this; a line of text that can simply wrap does
not.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The fit settings.",
      fields = {
        {
          name = "child",
          type = "ui.Widget",
          description = "The child widget.",
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the fitted widget is shown, or a signal that holds that value.",
        },
      },
    },
  },
  returns = { type = "ui.Widget", description = "The fitted widget." },
  impl = function(settings)
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
      along = along
        + math.max(0, shown - 1) * (w.spacing or 0) * raw.drawn_text_scale()
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
      local spacing = (w.spacing or 0) * raw.drawn_text_scale()
      local shown = 0
      for _, child in ipairs(w.children) do
        if child:is_shown() then
          shown = shown + 1
        end
      end

      -- Spare room along the axis goes into the gaps, or in front of the
      -- children, depending on what the stack asked for.
      local spare = box_along - along
      local at = 0
      if
        along_align
        == (
          horizontal and trx.ui.HAlign.DISTRIBUTE or trx.ui.VAlign.DISTRIBUTE
        )
      then
        spacing = spacing
          + (shown > 1 and math.max(0, spare) / (shown - 1) or 0)
      elseif
        along_align
        == (horizontal and trx.ui.HAlign.CENTER or trx.ui.VAlign.CENTER)
      then
        at = spare / 2
      elseif
        along_align
        == (horizontal and trx.ui.HAlign.RIGHT or trx.ui.VAlign.BOTTOM)
      then
        at = spare
      end

      for _, child in ipairs(w.children) do
        if child:is_shown() then
          local cw, ch = child:measure()
          local size_along = horizontal and cw or ch
          local size_across = horizontal and ch or cw
          local offset = 0
          if span then
            size_across = box_across
          elseif center then
            offset = (box_across - size_across) / 2
          elseif far then
            offset = box_across - size_across
          end
          if horizontal then
            child:paint(x + at, y + offset, size_along, size_across)
          else
            child:paint(x + offset, y + at, size_across, size_along)
          end
          at = at + size_along + spacing
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
