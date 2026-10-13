require("trx.internal.helpers")
require("trx.ui.tween")
local base = require("trx.ui.widgets.base")

---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget
local text_scale = base.text_scale

---@class (exact) trx.ui.widgets.Carousel.settings
---@field items any[] What the row holds, in order.
---@field selected trx.signal.Signal The place in the row, counted from `1`, of the item in the middle. Setting it slides the row there.
---@field build function Makes the widget for an item. It receives the item and its place in the row, and returns a widget.
---@field w number The width of the row, in canvas units at the player's text size.
---@field h number The height of the row, in canvas units at the player's text size.
---@field spacing number How far apart the middles of two neighbours stand, in canvas units at the player's text size.
---@field side_size? number How large the items beside the middle are drawn, as a share of their own size. `0.6` by default.
---@field ticks? integer How many ticks a slide takes. `8` by default.
---@field curve? function The curve of a slide, one of `trx.ui.ease`. `trx.ui.ease.quad_in_out` by default.
---@field shown? any Whether the row is shown, or a signal that holds that value.

-- How far beyond the neighbours of the middle an item slides before it is
-- gone.
local REACH = 1.5
local SLIDE_TICKS = 8

---A row of items turned to the one in the middle.
---@class (exact) trx.ui.widgets.Carousel : trx.ui.Widget
---@field is_sliding fun(): boolean Whether the row is still sliding to the selected item.

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
---@return trx.ui.widgets.Carousel # The carousel.
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
    local scale = value_of(text_scale())
    return w.w * scale, w.h * scale
  end, function(w, x, y, box_w, box_h)
    if count == 0 then
      return
    end
    local scale = value_of(text_scale())
    local at = position:get()
    for index, child in ipairs(children) do
      local d = count > 1 and distance(index, at, count) or 0
      local size = share(d, w.side_size or 0.6)
      if size > 0 then
        local child_w, child_h = child:measure()
        child_w = child_w * size
        child_h = child_h * size
        local middle_x = x + box_w / 2 + d * w.spacing * scale
        child:paint(
          middle_x - child_w / 2,
          y + (box_h - child_h) / 2,
          child_w,
          child_h
        )
      end
    end
  end)
  self.children = children
  for _, child in ipairs(children) do
    base.set_parent(child, self)
  end

  -- The selection slides the row the short way round from where it is.
  local listener = self.selected:on(function(index)
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
      self.ticks or SLIDE_TICKS,
      { curve = self.curve or trx.ui.ease.quad_in_out }
    )
  end)

  rawset(self, "is_sliding", function()
    return position:is_moving()
  end)

  self:wakes_on(text_scale()):release_with(listener)
  ---@cast self trx.ui.widgets.Carousel
  return self
end
