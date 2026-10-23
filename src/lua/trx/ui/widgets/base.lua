local h = require("trx.internal.helpers")

local track = require("trx.signal").track
require("trx.ui")
require("trx.ui.primitive")
require("trx.math")
require("trx.ui.tween")

local ui = trx.ui

-------------------------------------------------------------------------------
-- The widgets
--
-- A widget is created once and lives until its script releases it. Signals
-- invalidate cached measurements only when the values the widget read change.
--
-- A widget has one holder at a time: another widget, a region or a layer.
-------------------------------------------------------------------------------

local Signal = h.class_of("signal.Signal")
local Tween = h.class_of("ui.Tween")

-- The draw order of a frame, behind text.
local FRAME_Z = 160

-- The text colour that marks the option the player is on where no frame
-- does: the colour the original TR4 pulses its picked entry in.
local PICKED_COLOR = "1"

-- Removes a placed widget from its region. trx.ui.regions sets it, because
-- the regions are built on top of the widgets.
---@type fun(widget: trx.ui.Widget)?
local unplace = nil

local function is_signal(value)
  return getmetatable(value) == Signal
end

local function is_widget(value)
  return type(value) == "table" and value.wake ~= nil
end

-- Reads a setting. A signal hands over the value it holds, and a tween where
-- it is now.
local function value_of(value)
  local class = getmetatable(value)
  if class == Signal or class == Tween then
    return value:get()
  end
  return value
end

-- The listeners a widget holds, which go when it is released: one waker for
-- each signal it follows, keyed by the signal, and the listeners of its own
-- code, keyed by themselves.
local function held_by(widget)
  local held = widget._held
  if held == nil then
    held = {}
    widget._held = held
  end
  return held
end

-- Wakes the widget when the signal changes. A signal followed already adds
-- nothing, and a released widget follows nothing.
local function follow(widget, signal)
  if widget._released then
    return
  end
  local held = held_by(widget)
  if held[signal] == nil then
    held[signal] = signal:on(function()
      widget:wake()
    end)
  end
end

-- Runs fn when the signal changes, for as long as the widget lives.
local function listen(widget, signal, fn)
  local listener = signal:on(fn)
  held_by(widget)[listener] = listener
end

---A reusable UI element drawn over the game.
---
---A widget holds its own state. Every setting that holds a value, rather than
---a widget or a function, also takes a signal that holds the value. The widget
---remeasures only when a signal that it read while measuring changes, and
---when the room the interface lays out in changes. A `trx.ui.Tween` moves
---between ticks, so a widget reads one only while it paints, and reading one
---while it measures raises an error.
---
---A widget is in one place at a time: inside another widget, in a region or as
---the root of a layer. Using it in a second place is an error. Releasing a widget ends it and every widget under it, and
---using a released widget is an error.
---@class (exact) trx.ui.Widget
---@field private visible? any
---@field private child? trx.ui.Widget
---@field private children? trx.ui.Widget[]
---@field private _size? { w: number, h: number }
---@field private _holder? any
---@field private _released? boolean
---@field private _factor? number
---@field private _spun? number
---@field private _spun_at? number
---@field private _boxes? table
---@field private _held? table<any, trx.signal.Listener>
---@field private on_measure fun(self: trx.ui.Widget): number?, number?
---@field private on_paint fun(self: trx.ui.Widget, x: number, y: number, w: number, h: number)
local Widget = h.class("ui.Widget")

---How much room the widget wants.
---@return number # The width, in canvas units.
---@return number # The height, in canvas units.
function Widget:measure()
  if self._size == nil then
    -- The text and bar scales are part of the room, so every size follows it.
    follow(self, ui.signals.layout)
    local w, h = track(function(read)
      if getmetatable(read) == Tween then
        error("a widget cannot measure by a tween; read it while painting", 0)
      end
      follow(self, read)
    end, self.on_measure, self)
    self._size = { w = w or 0, h = h or 0 }
  end
  return self._size.w, self._size.h
end

---Invalidates the widget's cached size manually.
---@return trx.ui.Widget # The same widget.
function Widget:wake()
  self._size = nil
  self._boxes = nil
  local holder = self._holder
  if type(holder) == "table" and holder.wake ~= nil then
    holder:wake()
  end
  return self
end

---Returns whether the widget participates in layout.
---
---A widget that is not visible keeps no room and leaves no gap.
---@return boolean # Whether it draws.
function Widget:is_visible()
  return value_of(self.visible) ~= false
end

-- Detaches a widget and every widget under it from what they listen to, and
-- marks each one released.
local function end_tree(widget)
  widget._released = true
  local held = widget._held
  if held ~= nil then
    for _, listener in pairs(held) do
      listener:detach()
    end
    widget._held = nil
  end
  local child = widget.child
  if child ~= nil then
    child._holder = nil
    end_tree(child)
  end
  for _, other in ipairs(widget.children or {}) do
    other._holder = nil
    end_tree(other)
  end
end

---Ends the widget and every widget under it: removes the widget from its
---region if it is placed, and detaches each one from its signals. Does nothing
---if the widget is already released.
---
---Signals keep references to their listeners. Release temporary widgets when
---they are no longer needed. A widget that another widget or a layer holds is
---released with its holder, and releasing it directly is an error. A widget that a level script placed is released when the level ends.
function Widget:release()
  if self._released then
    return
  end
  if unplace ~= nil then
    unplace(self)
  end
  if is_widget(self._holder) then
    error("the widget is part of another widget; release that one", 2)
  elseif self._holder ~= nil then
    error("the widget is in use; release what holds it", 2)
  end
  end_tree(self)
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
  -- Widgets that are not visible keep no room at all.
  if self:is_visible() then
    self:on_paint(x, y, w, h)
  end
end

-- Drops the cached size of a widget and of every widget under it, so the
-- next measure works the whole tree out again, such as under another text
-- scale.
local function forget(widget)
  widget._size = nil
  widget._boxes = nil
  local child = widget.child
  if child ~= nil then
    forget(child)
  end
  for _, other in ipairs(widget.children or {}) do
    forget(other)
  end
end

-- Copies the settings into a new widget of the class, which is Widget by
-- default. A setting that has the name of a member of the class, or that
-- starts with an underscore, is an error, because it would hide that member.
local function new_widget(settings, on_measure, on_paint, class)
  class = class or Widget
  local self = setmetatable({}, class)
  for key, value in pairs(settings or {}) do
    if
      type(key) ~= "string"
      or key:sub(1, 1) == "_"
      or key == "on_measure"
      or key == "on_paint"
      or class[key] ~= nil
    then
      error(("'%s' is not a widget setting"):format(tostring(key)), 3)
    end
    if key == "shown" or key == "hidden" then
      error(
        ("'%s' is not a widget setting; use visible, or wrap the widget in trx.ui.widgets.Blank"):format(
          key
        ),
        3
      )
    end
    self[key] = value
  end
  self.on_measure = on_measure
  self.on_paint = on_paint
  return self
end

-- Gives a widget its holder: the widget it is part of, which wakes with it,
-- or the layer it is the root of. A widget
-- has one holder at a time, so one that is held already is an error at the
-- given level, the caller of the widget constructor by default.
local function hold(widget, holder, level)
  level = level or 3
  if widget._released then
    error("the widget was released", level)
  end
  local held_by = widget._holder
  if held_by ~= nil and held_by ~= holder then
    if is_widget(held_by) and is_widget(holder) then
      error("the widget is already part of another widget", level)
    end
    error("the widget is already in use", level)
  end
  widget._holder = holder
end

-- Frees a widget from its holder, for another one.
local function unhold(widget, holder)
  if widget._holder == holder then
    widget._holder = nil
  end
end

---The settings that every widget takes, with its own.
---@class (exact) trx.ui.widgets.settings
---@field visible? any Whether the widget is visible, or a signal that holds that value. A widget that is not visible keeps no room and leaves no gap. `trx.ui.widgets.Blank` keeps the room instead.

---@class (partial) trx.ui
---@field widgets trx.ui.widgets

---The widgets a script builds its screen from.
---
---A widget is created once and kept. Give it signals instead of fixed values,
---and it remeasures when they change.
---
---Put a widget on screen with `trx.ui.regions.place`.
---@class (partial,exact) trx.ui.widgets
ui.widgets = h.namespace("ui.widgets")

local function set_unplace(fn)
  unplace = fn
end

return {
  W = Widget,
  FRAME_Z = FRAME_Z,
  PICKED_COLOR = PICKED_COLOR,
  is_signal = is_signal,
  value_of = value_of,
  new_widget = new_widget,
  hold = hold,
  unhold = unhold,
  set_unplace = set_unplace,
  forget = forget,
  listen = listen,
}
