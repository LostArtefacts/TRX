local raw = trxc.ui
local h = require("trx.internal.helpers")
require("trx.ui.tween")
local base = require("trx.ui.widgets.base")

---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget

---@class (exact) trx.ui.widgets.Carousel.settings: trx.ui.widgets.settings
---@field items any[] What the row holds, in order.
---@field selected trx.signal.Signal The place in the row, counted from `1`, of the item in the middle. Setting it slides the row there.
---@field build function Makes the widget for an item. It receives the item and its place in the row, and returns a widget.
---@field w any The width of the row, in canvas units at the default text size, or a signal that holds it.
---@field h any The height of the row, in canvas units at the default text size, or a signal that holds it.
---@field spacing any How far apart the middles of two neighbours stand, in canvas units at the default text size, or a signal that holds it.
---@field side_size? any How large the items beside the middle are drawn, as a share of their own size, or a signal that holds it. `0.6` by default.
---@field ticks? any How many ticks a slide takes, or a signal that holds it. `8` by default.
---@field curve? any The curve of a slide, one of `trx.ui.ease`, or a signal that holds it. `trx.ui.ease.quad_in_out` by default.

-- How far beyond the neighbours of the middle an item slides before it is
-- gone.
local REACH = 1.5
local SLIDE_TICKS = 8
local SIDE_SIZE = 0.6

---A row of items turned to the one in the middle.
---@class (exact) trx.ui.Carousel: trx.ui.Widget
---@field private _position trx.ui.Tween
local Carousel = h.class("ui.Carousel", { extends = "ui.Widget" })

---Returns whether the row is still sliding to the selected item.
---@return boolean # Whether it slides.
function Carousel:is_sliding()
  return self._position:is_moving()
end

-- How far an item stands from the middle, in places, going the short way round
-- so that the row wraps.
local function distance(index, position, count)
  local d = (index - position) % count
  if d > count / 2 then
    d = d - count
  end
  return d
end

-- How large an item is drawn at a distance from the middle.
local function share(d, side)
  d = math.abs(d)
  if d <= 1 then
    return 1 + (side - 1) * d
  elseif d < REACH then
    return side * (REACH - d) / (REACH - 1)
  end
  return 0
end

---A row of items turned to the one in the middle, as the inventory ring
---offers what to combine an entry with. The item in the middle is drawn at its
---own size, those beside it smaller, and the rest out of sight. Setting the
---selected signal slides the row there, the short way round.
---
---```lua
---local selected = trx.signal.new(1)
---trx.ui.widgets.Carousel({
---  items = objects,
---  selected = selected,
---  build = function(object)
---    return trx.ui.widgets.Model({ object = object, w = 100, h = 120 })
---  end,
---  w = 260,
---  h = 150,
---  spacing = 90,
---})
---```
---@param settings trx.ui.widgets.Carousel.settings The carousel settings.
---@return trx.ui.Carousel # The carousel.
function widgets.Carousel(settings)
  local count = #settings.items
  local children = {}
  for index, item in ipairs(settings.items) do
    children[index] = settings.build(item, index)
  end
  local start = value_of(settings.selected) or 1
  local position = trx.ui.tween(start)
  local placed = start

  local self = new_widget(settings, function(w)
    local scale = raw.drawn_text_scale()
    return value_of(w.w) * scale, value_of(w.h) * scale
  end, function(w, x, y, box_w, box_h)
    if count == 0 then
      return
    end
    local scale = raw.drawn_text_scale()
    local at = position:get()
    local side_size = value_of(w.side_size) or SIDE_SIZE
    local spacing = value_of(w.spacing)
    for index, child in ipairs(children) do
      local d = count > 1 and distance(index, at, count) or 0
      local size = share(d, side_size)
      if size > 0 then
        local child_w, child_h = child:measure()
        child_w = child_w * size
        child_h = child_h * size
        local middle_x = x + box_w / 2 + d * spacing * scale
        child:paint(
          middle_x - child_w / 2,
          y + (box_h - child_h) / 2,
          child_w,
          child_h
        )
      end
    end
  end, Carousel)
  self.children = children
  for _, child in ipairs(children) do
    base.hold(child, self)
  end

  -- The selection slides the row the short way round from where it is.
  base.listen(self, self.selected, function(index)
    if count < 2 or index == nil then
      return
    end
    local step = (index - placed) % count
    if step > count / 2 then
      step = step - count
    end
    placed = index
    position:to(
      position:target() + step,
      value_of(self.ticks) or SLIDE_TICKS,
      { curve = value_of(self.curve) or trx.ui.ease.quad_in_out }
    )
  end)

  self._position = position
  return self
end
