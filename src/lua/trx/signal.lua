local raw = trxc.events
local raw_config = trxc.config
local h = require("trx.internal.helpers")

local at_level_end = require("trx.events").at_level_end

---@class (partial) trx
---@field signal trx.signal

---A value that can notify listeners when it changes.
---
---A signal lets a script react to changes without polling every frame. You can
---read the current value, listen for changes, and combine signals with `&`,
---`|` and `~`.
---
---Setting a signal to its current value does nothing: listeners do not run,
---and signals derived from it do not update. This keeps combined signals cheap
---to listen to. A combined signal fires only when its own result changes, not
---every time one of its inputs changes.
---
---A derived signal is read like any other signal, so one expression can
---provide both the current result and change notifications.
---
---Signals should carry numbers, strings or booleans, not handles. Handles are
---created fresh on each read, so two reads of the same handle do not compare
---equal and would make the signal report a change every frame. When a signal
---represents an engine-owned object, it carries the object's numeric id;
---listeners can then read the handle when they need it.
---
---Signals stay idle until something uses them: a polled signal starts reading
---only when it is created.
---
---What a script makes lasts as follows:
---
---- A signal that a level script derives, with `trx.signal.Signal:map`,
---  `trx.signal.combine`, an operator or `trx.signal.polled`, stops when the
---  level ends. One that a global script derives follows its sources while
---  something holds it or listens to it, or until `trx.signal.Signal:stop`.
---- An event handler follows the script that attached it, as `trx.events`
---  says.
---- A listener from `trx.signal.Signal:on` stays attached until
---  `trx.signal.Listener:detach`, whichever script attached it.
---- The engine's own signals, such as `trx.game.signals`, last the session.
---@trx.module 40 Signals
---@class (exact) trx.signal
---@trx.readonly tick
---@field tick trx.signal.Signal A signal that increments on every engine tick, regardless of what is on screen. Anything listening to it runs every tick.
---
---  Use this when a script needs to poll state that has no dedicated signal,
---  such as Lara's current position. A dedicated signal is cheaper when one
---  exists, because this one wakes listeners even when the state they care
---  about has not changed.
local M = h.module("signal")

-- Lua truthiness, shared by listener values and derived signal operators.
local function truth(value)
  return value ~= nil and value ~= false
end

-- Forward declaration lets the operators of the Signal type build derived
-- signals.
local derive

---A value that notifies listeners when it changes.
---@class (exact) trx.signal.Signal
---@field private __index trx.signal.Signal
---@field private _value any
---@field private _listeners table<integer, fun(value: any)>
---@field private _next_id integer
---@field private _bound trx.signal.Listener[]?
---@operator band(trx.signal.Signal): trx.signal.Signal
---@operator bor(trx.signal.Signal): trx.signal.Signal
---@operator bnot: trx.signal.Signal
---@trx.operator band Both signals are true. Fires when the result changes.
---@trx.operator bor Either signal is true. Fires when the result changes.
---@trx.operator bnot The signal is not true: `~trx.cutscenes.signals.is_playing`.
local Signal = h.class("signal.Signal", {
  operators = {
    band = function(a, b)
      return derive({ a, b }, function(x, y)
        return truth(x) and truth(y)
      end)
    end,
    bor = function(a, b)
      return derive({ a, b }, function(x, y)
        return truth(x) or truth(y)
      end)
    end,
    bnot = function(a)
      return derive({ a }, function(x)
        return not truth(x)
      end)
    end,
  },
})

---A signal listener that can be detached later.
---@class (exact) trx.signal.Listener
---@field private __index trx.signal.Listener
---@field private _signal trx.signal.Signal
---@field private _id integer
local Listener = h.class("signal.Listener")

local function new_signal(value)
  return setmetatable({
    _value = value,
    _listeners = {},
    _next_id = 1,
  }, Signal)
end

-- How many listeners each derived signal has. The count holds the signal, so
-- that a signal with a listener attached keeps following its sources even
-- where nothing else holds it.
---@type table<trx.signal.Signal, integer>
local heard = {}

-- Derived signals keep the listeners they attach to their sources, so they can
-- detach them later. Level-script signals are stopped when the level ends.
local function bind(derived, listeners)
  derived._bound = listeners
  at_level_end(function()
    derived:stop()
  end)
  return derived
end

-- Returns a function that runs update on a derived signal while something
-- holds the signal. The sources hold the signal through this function only,
-- and weakly, so a signal that nothing reads any more is collected, and the
-- listeners go the first time one of them runs after that.
local function while_held(derived, listeners, update)
  local box = setmetatable({ derived }, { __mode = "v" })
  return function(...)
    local held = box[1]
    if held == nil then
      for _, listener in ipairs(listeners) do
        listener:detach()
      end
      return
    end
    update(held, ...)
  end
end

-- Creates a derived signal by reading one or more source signals.
derive = function(sources, fn)
  local count = #sources

  local function value_of()
    local args = {}
    for i = 1, count do
      args[i] = sources[i]:get()
    end
    return fn(table.unpack(args, 1, count))
  end

  local derived = new_signal(value_of())
  local listeners = {}
  local recompute = while_held(derived, listeners, function(held)
    held:set(value_of())
  end)
  for i, source in ipairs(sources) do
    listeners[i] = source:on(recompute)
  end
  return bind(derived, listeners)
end

-- Called with each signal or tween read while a tracked function runs, or nil.
---@type fun(read: trx.signal.Signal|trx.ui.Tween)?
local tracker = nil

-- Hands a value that is read to the function that track runs it under, if any.
local function report(value)
  if tracker ~= nil then
    tracker(value)
  end
end

-- Runs fn and reports each signal or tween it reads to callback, so that a
-- widget follows every signal its measuring reads, however it reads it.
local function track(callback, fn, ...)
  local outer = tracker
  tracker = callback
  local results = table.pack(pcall(fn, ...))
  tracker = outer
  if not results[1] then
    error(results[2], 0)
  end
  return table.unpack(results, 2, results.n)
end

---The value the signal holds now.
---@return any # What it holds.
function Signal:get()
  report(self)
  return self._value
end

---Sets the signal's value. Setting the current value again does nothing, so
---repeated writes are cheap.
---@param value any The new value.
---@return boolean # Whether the value changed and listeners ran.
function Signal:set(value)
  if self._value == value then
    return false
  end
  self._value = value
  -- Copy listeners before running them, so detaching during a callback
  -- does not change the current dispatch pass.
  local listeners = self._listeners
  local round = {}
  for id, fn in pairs(listeners) do
    round[id] = fn
  end
  for id, fn in pairs(round) do
    if listeners[id] ~= nil then
      fn(value)
    end
  end
  return true
end

---Calls the handler with the new value whenever the signal changes. Attaching
---a listener does not call it immediately; read the signal directly when you
---need its current value.
---
---The listener stays attached until `trx.signal.Listener:detach`, even when a
---level script attaches it, so that it can follow an engine signal past the
---level. A level script that needs the value for its own level only can
---derive a signal instead, which stops with the level, or detach the listener
---in `trx.events.on_level_unload`.
---@param fn fun(value: any) The function to call when the signal changes.
---@trx.arg fn.value The new value.
---@return trx.signal.Listener # The listener handle used to detach later.
function Signal:on(fn)
  local listeners = self._listeners
  local id = self._next_id
  self._next_id = id + 1
  listeners[id] = fn
  if self._bound ~= nil then
    heard[self] = (heard[self] or 0) + 1
  end
  return setmetatable({ _signal = self, _id = id }, Listener)
end

---Creates a signal by applying a function to this signal's value.
---
---Use this for derived values that are not simple boolean combinations, such
---as a bar fill amount or resolved key text.
---@param fn fun(value: any): any The function that computes the derived value.
---@trx.arg fn.value This signal's value.
---@return trx.signal.Signal # The derived signal.
function Signal:map(fn)
  return derive({ self }, fn)
end

---Stops a derived signal from following its sources. It keeps its last value
---and will not update again. Signals made by level scripts stop when the level
---ends; global scripts can call this to stop one earlier.
---@return boolean # Whether the signal was still following any sources.
function Signal:stop()
  local bound = self._bound
  if bound == nil then
    return false
  end
  for _, listener in ipairs(bound) do
    listener:detach()
  end
  self._bound = nil
  return true
end

---Whether the signal holds this value.
---@param value any What to compare against.
---@return trx.signal.Signal # The derived signal.
function Signal:eq(value)
  return derive({ self }, function(x)
    return x == value
  end)
end

---Detaches the listener so it no longer receives changes.
---@return boolean # Whether it was still listening.
function Listener:detach()
  local signal = self._signal
  local listeners = signal._listeners
  local id = self._id
  local was_listening = listeners[id] ~= nil
  listeners[id] = nil
  local count = heard[signal]
  if was_listening and count ~= nil then
    heard[signal] = count > 1 and count - 1 or nil
  end
  return was_listening
end

-- Creates a polled signal that belongs to no level. An engine module keeps one
-- such signal for the whole session and hands it to every script that asks,
-- so it must not stop with the level of the script that asked first.
local function lasting_polled(read)
  local polled = new_signal(read())
  trx.signal.tick:on(function()
    polled:set(read())
  end)
  return polled
end

-- Subscribed as the module loads, so the handler belongs to no level. One a
-- level script attached would go when that level ended, and every polled signal
-- in the session would stop reading with it.
local tick = new_signal(raw.tick())
trx.events.on_tick(function()
  tick:set(raw.tick())
end)

h.properties(M, "signal", {
  tick = {
    get = function()
      return tick
    end,
  },
})

---Creates a signal by reading a value once per tick and notifying listeners
---only when that value changes.
---
---Use this for state that has no dedicated engine signal. The read function
---runs every tick, but listeners run only on changes, so several listeners on
---one polled signal share one read.
---@param read function The function to read each tick. Tables compare by identity, so returning a fresh table every tick reports a change every tick.
---@return trx.signal.Signal # The polled signal.
function M.polled(read)
  local polled = new_signal(read())
  local listeners = {}
  listeners[1] =
    trx.signal.tick:on(while_held(polled, listeners, function(held)
      held:set(read())
    end))
  return bind(polled, listeners)
end

---Creates a signal that reads a value once per tick while another signal is
---true, and holds a fixed value while it is not.
---
---Use this for state that has no dedicated engine signal and matters only
---while something shows it, such as a readout that the player can turn off. A
---reader that is off costs nothing per tick.
---
---```lua
---local shown = trx.signal.config("ui.enable_fps_counter")
---local fps = trx.signal.polled_while(shown, trx.game.get_fps, 0)
---```
---@param shown trx.signal.Signal Whether to read.
---@param read function The function to read each tick while the first signal is true.
---@param idle any The value to hold while the first signal is false.
---@return trx.signal.Signal # The signal.
function M.polled_while(shown, read, idle)
  local signal = new_signal(idle)
  local listeners = {}
  ---@type trx.signal.Listener?
  local ticking = nil
  local function follow(held, on)
    if truth(on) then
      held:set(read())
      if ticking == nil then
        ticking = trx.signal.tick:on(while_held(held, listeners, function(h)
          h:set(read())
        end))
      end
    else
      held:set(idle)
      if ticking ~= nil then
        ticking:detach()
        ticking = nil
      end
    end
  end
  follow(signal, shown:get())
  listeners[1] = shown:on(while_held(signal, listeners, follow))
  listeners[2] = {
    detach = function()
      if ticking ~= nil then
        ticking:detach()
        ticking = nil
      end
    end,
  }
  return bind(signal, listeners)
end

-- Reuse one signal per setting, so multiple scripts share one engine watcher.
local settings = {} --[[@as table<string, trx.signal.Signal>]]

---Returns a signal for a config setting.
---
---The signal holds the setting's current value and updates whenever the player
---or a script changes it. Asking for the same setting twice returns the same
---signal.
---@param key string Dotted setting path, as accepted by `trx.config.get`.
---@return trx.signal.Signal # The setting's signal.
function M.config(key)
  local existing = settings[key]
  if existing ~= nil then
    return existing
  end
  -- The signal is kept for as long as the game runs, so the watcher behind it
  -- has to be too: one scoped to the level that first asked would leave every
  -- later reader holding a signal that never moves again. The call back as it
  -- attaches is what gives the signal the setting's current value.
  local created = new_signal(nil)
  settings[key] = created
  raw_config.on_change_lasting(key, function(value)
    created:set(value)
  end)
  return created
end

---Creates a signal by applying a function to several source signals.
---
---Pass the signals first and the function last. `trx.signal.Signal:map` is the
---one-signal version. For boolean combinations, `&`, `|` and `~` are shorter.
---
---```lua
---local fill = trx.signal.combine(
---  trx.lara.signals.hp,
---  trx.lara.signals.max_hp,
---  function(hp, max_hp)
---    return hp / max_hp
---  end
---)
---```
---@param ... trx.signal.Signal|function The signals to read, in the order the function takes them, then the function that computes the derived value.
---@return trx.signal.Signal # The derived signal.
function M.combine(...)
  local args = { ... }
  local fn = table.remove(args)
  return derive(args, fn)
end

---Creates a script-owned signal with an initial value.
---@param value any? The initial value.
---@return trx.signal.Signal # The new signal.
---@type fun(value: any?): trx.signal.Signal
M.new = new_signal

return { lasting_polled = lasting_polled, report = report, track = track }
