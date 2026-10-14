local raw = trxc.tween
local h = require("trx.internal.helpers")
local report = require("trx.signal").report

require("trx.ui")

local ui = trx.ui

---@class (partial) trx.ui
---@field ease trx.ui.ease
---@field tween fun(value?: number): trx.ui.Tween

---Curves that shape how a `trx.ui.Tween` moves. Each takes how far the move
---has gone in time, from `0` to `1`, and returns how far the value has gone.
---@class (partial,exact) trx.ui.ease
ui.ease = h.namespace("ui.ease")

---@class (partial) trx.ui.ease
local ease = trx.ui.ease

---The same pace throughout.
---@param ratio number How far the move has gone in time.
---@return number # How far the value has gone.
function ease.linear(ratio)
  return raw.ease("linear", ratio)
end

---Slow to start.
---@param ratio number How far the move has gone in time.
---@return number # How far the value has gone.
function ease.quad_in(ratio)
  return raw.ease("quad_in", ratio)
end

---Slow to finish.
---@param ratio number How far the move has gone in time.
---@return number # How far the value has gone.
function ease.quad_out(ratio)
  return raw.ease("quad_out", ratio)
end

---Slow at both ends, fastest in the middle.
---@param ratio number How far the move has gone in time.
---@return number # How far the value has gone.
function ease.quad_in_out(ratio)
  return raw.ease("quad_in_out", ratio)
end

-- The engine runs a tween on its own copy of the curve, which it knows by name.
local CURVES = {
  [ease.linear] = "linear",
  [ease.quad_in] = "quad_in",
  [ease.quad_out] = "quad_out",
  [ease.quad_in_out] = "quad_in_out",
}

---A number that moves to a target over some ticks.
---
---Reading it while the interface is painted gives the value part way between
---two ticks, so that what it moves glides at any frame rate.
---
---```lua
---local slide = trx.ui.tween(0)
---slide:to(1, 8, { curve = trx.ui.ease.quad_in_out })
---paint = function(x, y)
---  trx.ui.primitive.text("hi", x + 100 * slide:get(), y)
---end
---```
---@class (exact) trx.ui.Tween
---@field private __index trx.ui.Tween
---@field private _raw userdata
local Tween = h.class("ui.Tween")

---The value now.
---@return number # The value.
function Tween:get()
  report(self)
  return raw.get(self._raw)
end

---Where the value is going, or where it rests.
---@return number # The target.
function Tween:target()
  return raw.target(self._raw)
end

---Whether the value is still on its way to its target, counted in whole
---ticks, as the input a script reads each tick is.
---@return boolean # `true` until the tick the move ends on.
function Tween:is_moving()
  return raw.is_moving(self._raw)
end

---@class (exact) trx.ui.Tween.to.options
---@field curve? function The curve of the move, one of `trx.ui.ease`. `trx.ui.ease.linear` by default. Any other function raises an error.
---@field delay? integer How many ticks to wait before the move starts. The value holds still while it waits, and `trx.ui.Tween:is_moving` reports true. `0` by default.

---Starts moving to a target from wherever the value is now.
---@param target number Where to go.
---@param ticks integer How many ticks the move takes.
---@param options? trx.ui.Tween.to.options How the move goes.
---@return trx.ui.Tween # The same tween.
function Tween:to(target, ticks, options)
  options = options or {}
  local name = CURVES[options.curve or ease.linear]
  if name == nil then
    error("the curve is not one of trx.ui.ease", 2)
  end
  local delay = options.delay or 0
  raw.to(self._raw, target, ticks, name, delay)
  return self
end

---Puts the value at once where it is told, and stops any move.
---@param value number The value.
---@return trx.ui.Tween # The same tween.
function Tween:set(value)
  raw.set(self._raw, value)
  return self
end

---Creates a tween that rests at a value.
---@param value? number Where it rests. `0` by default.
---@return trx.ui.Tween # The tween.
function ui.tween(value)
  return setmetatable({ _raw = raw.new(value or 0) }, Tween)
end

-- The name the engine knows a curve by, for the modules that hand a curve to
-- the engine.
return {
  curve_name = function(curve)
    return CURVES[curve or ease.linear]
  end,
}
