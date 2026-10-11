local raw = trxc.ui
local h = require("trx.internal.helpers")

require("trx.ui")
require("trx.signal")

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
  return ratio
end

---Slow to start.
---@param ratio number How far the move has gone in time.
---@return number # How far the value has gone.
function ease.quad_in(ratio)
  return ratio * ratio
end

---Slow to finish.
---@param ratio number How far the move has gone in time.
---@return number # How far the value has gone.
function ease.quad_out(ratio)
  local rest = 1 - ratio
  return 1 - rest * rest
end

---Slow at both ends, fastest in the middle.
---@param ratio number How far the move has gone in time.
---@return number # How far the value has gone.
function ease.quad_in_out(ratio)
  if ratio < 0.5 then
    return 2 * ratio * ratio
  end
  local rest = ratio - 1
  return 1 - 2 * rest * rest
end

-- The tick a frame is drawn at, part way between the last two ticks.
local function now()
  return trx.signal.tick:get() - 1 + raw.interpolation_rate()
end

---A number that moves to a target over some ticks.
---
---Reading it while the interface is painted gives the value part way between
---two ticks, so that what it moves glides at any frame rate.
---
---```lua
---local slide = trx.ui.tween(0)
---slide:to(1, 8, trx.ui.ease.quad_in_out)
---paint = function(x, y)
---  trx.ui.primitive.text("hi", x + 100 * slide:get(), y)
---end
---```
---@class (exact) trx.ui.Tween
---@field private __index trx.ui.Tween
---@field private _from number
---@field private _to number
---@field private _start number
---@field private _ticks number
---@field private _ease function
local Tween = h.class("ui.Tween")

---The value now.
---@return number # The value.
function Tween:get()
  local ticks = rawget(self, "_ticks")
  local elapsed = now() - rawget(self, "_start")
  if ticks <= 0 or elapsed >= ticks then
    return rawget(self, "_to")
  end
  local from = rawget(self, "_from")
  local ratio = rawget(self, "_ease")(math.max(elapsed, 0) / ticks)
  return from + (rawget(self, "_to") - from) * ratio
end

---Where the value is going, or where it rests.
---@return number # The target.
function Tween:target()
  return rawget(self, "_to")
end

---Whether the value is still on its way to its target, counted in whole
---ticks, as the input a script reads each tick is.
---@return boolean # `true` until the tick the move ends on.
function Tween:is_moving()
  return trx.signal.tick:get() - rawget(self, "_start")
    < rawget(self, "_ticks")
end

---Starts moving to a target from wherever the value is now.
---@param target number Where to go.
---@param ticks integer How many ticks the move takes.
---@param curve? function The curve of the move, one of `trx.ui.ease`. `trx.ui.ease.linear` by default.
---@return trx.ui.Tween # The same tween.
function Tween:to(target, ticks, curve)
  rawset(self, "_from", self:get())
  rawset(self, "_to", target)
  rawset(self, "_start", trx.signal.tick:get())
  rawset(self, "_ticks", ticks)
  rawset(self, "_ease", curve or ease.linear)
  return self
end

---Puts the value at once where it is told, and stops any move.
---@param value number The value.
---@return trx.ui.Tween # The same tween.
function Tween:set(value)
  rawset(self, "_from", value)
  rawset(self, "_to", value)
  rawset(self, "_ticks", 0)
  return self
end

---Creates a tween that rests at a value.
---@param value? number Where it rests. `0` by default.
---@return trx.ui.Tween # The tween.
function ui.tween(value)
  value = value or 0
  return setmetatable({
    _from = value,
    _to = value,
    _start = 0,
    _ticks = 0,
    _ease = ease.linear,
  }, Tween)
end
