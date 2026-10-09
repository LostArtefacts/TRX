local h = require("trx.internal.helpers")

require("trx.config")
require("trx.signal")
require("trx.ui")
require("trx.ui.primitive")
require("trx.math")

local ui = trx.ui

-------------------------------------------------------------------------------
-- The widgets
--
-- A widget is created once and lives until its script releases it. Signals
-- invalidate cached measurements only when the values the widget named change.
--
-- Each region has at most one root, so a script's widgets are laid out as one
-- group after the engine's own UI for that region.
-------------------------------------------------------------------------------

-- The engine lays its own widgets out in single precision. A box that lands on
-- half a pixel rounds one way or the other depending on the precision it was
-- worked out in, so the layout here rounds every step the same way to land
-- where the engine would.
local function f32(value)
  return (string.unpack("<f", string.pack("<f", value)))
end

local function value_of(value)
  if type(value) == "table" and value.get ~= nil then
    return value:get()
  end
  return value
end

---A reusable UI element drawn over the game.
---
---A widget holds its own state. Give it signals instead of fixed values, then
---register those signals with `trx.ui.Widget:wakes_on`. The widget remeasures
---only when a registered signal changes.
---
---Register every signal that the widget reads. Otherwise the widget can keep a
---stale cached size.
---@class (exact) trx.ui.Widget
---@field hidden? any Whether the widget keeps its room but draws nothing, or a signal that holds that value.
---@field private shown? any
---@field private children? trx.ui.Widget[]
---@field private _size? { w: number, h: number }
---@field private _parent? trx.ui.Widget
---@field private _layer? trx.ui.Layer
---@field private _region_listener? trx.signal.Listener
---@field private _slot? integer
---@field private on_measure fun(self: trx.ui.Widget): number?, number?
---@field private on_paint fun(self: trx.ui.Widget, x: number, y: number, w: number, h: number)
local Widget = h.class("ui.Widget")

-- Recompute the widget's size only after something invalidates it.
---How much room the widget wants.
---@return number # The width, in canvas units.
---@return number # The height, in canvas units.
function Widget:measure()
  if self._size == nil then
    local w, h = self:on_measure()
    self._size = { w = w or 0, h = h or 0 }
  end
  return self._size.w, self._size.h
end

---Invalidates the widget's cached size manually.
---@return trx.ui.Widget # The same widget.
function Widget:wake()
  self._size = nil
  if self._parent ~= nil then
    self._parent:wake()
  end
  return self
end

---Returns whether the widget participates in layout.
---
---A widget that is not shown keeps no room and leaves no gap. A hidden widget
---keeps its room but draws nothing.
---@return boolean # Whether it draws.
function Widget:is_shown()
  return value_of(self.shown) ~= false
end

-- Register the signals that invalidate the widget's cached size. Any signal a
-- widget reads should be listed here.
---Registers the signals that invalidate the widget's cached size.
---
---When one of these signals changes, the widget and its parents are measured
---again on the next layout pass.
---@param ... trx.signal.Signal The signals the widget reads.
---@return trx.ui.Widget # The same widget, for method chaining.
function Widget:wakes_on(...)
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
---Detaches the widget and its children from registered signals.
---
---Signals keep references to their listeners. Release temporary widgets when
---they are no longer needed. Remove a placed widget from its region before
---releasing it.
---@return boolean # Whether it was still listening to anything.
function Widget:release()
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

---Draws the widget in an assigned box.
---
---`trx.ui.regions.place` calls this automatically. Custom layout code can call
---it during `trx.ui.on_paint`.
---@param x number The left edge.
---@param y number The top edge.
---@param w number The width it was given.
---@param h number The height it was given.
function Widget:paint(x, y, w, h)
  -- Hidden widgets keep their room but draw nothing. Widgets that are not shown
  -- keep no room at all.
  if self:is_shown() and value_of(self.hidden) ~= true then
    self:on_paint(x, y, w, h)
  end
end

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

---@class (partial) trx.ui
---@field widgets trx.ui.widgets

---The widgets a script builds its screen from.
---
---A widget is created once and kept. Give it signals instead of fixed values,
---then register those signals with `trx.ui.Widget:wakes_on`.
---
---Put a widget on screen with `trx.ui.regions.place`.
---@class (partial,exact) trx.ui.widgets
ui.widgets = h.namespace("ui.widgets")

return {
  W = Widget,
  f32 = f32,
  value_of = value_of,
  new_widget = new_widget,
  text_scale = text_scale,
  bar_scale = bar_scale,
}
