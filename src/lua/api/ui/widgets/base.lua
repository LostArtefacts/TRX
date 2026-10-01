local api = trx.api

require("trx.config")
require("trx.signal")
require("trx.ui")
require("trx.ui.primitive")
require("trx.math")

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

return {
  W = W,
  value_of = value_of,
  new_widget = new_widget,
  text_scale = text_scale,
  bar_scale = bar_scale,
}
