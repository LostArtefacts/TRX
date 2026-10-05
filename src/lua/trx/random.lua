local raw = trxc.random
local h = require("trx.internal.helpers")

require("trx.math")

---@class (partial) trx
---@field random trx.random

---Random numbers, drawn from one of the two sequences the engine runs on.
---
---The module's own calls draw from the control stream. This is the sequence
---the simulation runs on, so a script that draws every frame changes what the
---creatures decide next. The draw stream, `trx.random.draw`, is the one the
---original game keeps for what is only seen. Drawing from it leaves the
---simulation as it was. Both streams are the same generator and offer the same
---calls, described in `trx.random.Stream`.
---
---The savegame carries both sequences. A script's draws come back the same
---after a reload, and a script needs no seed of its own.
---
---Lua's own `math.random` is a separate generator that nothing saves. It has
---no place in anything the simulation reads. <!--noref: math.random-->
---@trx.module 33
---@class (exact) trx.random
---@trx.readonly control, draw
---@field control trx.random.Stream The sequence the simulation runs on, which the module's own functions draw from.
---@field draw trx.random.Stream The sequence kept for what is only seen. Drawing from it leaves what the creatures decide next as it was, which is what the original game keeps it for.
local M = h.module("random")

-- How wide one draw of the engine's stream is, as the generator states it.
local SPAN = raw.SPAN
-- Two of them make the fraction random() reports.
local FRACTION = SPAN * SPAN
-- Four draws is as wide a range as below() can hold without leaving what a Lua
-- integer counts.
local MAX_SPAN = SPAN * SPAN * SPAN * SPAN

-- Maps each stream handle to the sequence it draws from. A handle is an empty
-- table, so the sequence is reachable only through this map.
local sources = setmetatable({}, { __mode = "k" })

-- A whole number below `n`. The top of the range is thrown away and drawn
-- again, so that no value comes up more often than another.
local function below(self, n)
  if n > MAX_SPAN then
    error("range is too wide", 3)
  end

  local next_value = sources[self]
  local pow, draws = SPAN, 1
  while pow < n do
    pow = pow * SPAN
    draws = draws + 1
  end

  local limit = pow - (pow % n)
  while true do
    local value = 0
    for _ = 1, draws do
      value = value * SPAN + next_value()
    end
    if value < limit then
      return value % n
    end
  end
end

local function fraction(self)
  local next_value = sources[self]
  return (next_value() * SPAN + next_value()) / FRACTION
end

---One of the engine's two random sequences.
---
---Drawing from `trx.random.control` changes what the game does next,
---because the simulation runs on it. Drawing from `trx.random.draw`
---changes nothing, because only the picture uses it.
---
---Both have the same calls. The module's own functions draw from the
---control stream.
---@class (exact) trx.random.Stream
local Stream = h.class("random.Stream")

---A fraction of one, the whole number itself excepted.
---@return number # A value in [0, 1).
function Stream:random()
  return fraction(self)
end

---A whole number between two bounds, both of them included.
---
---```lua
---local pips = trx.random.draw:randint(1, 6)
---```
---@param a integer Lowest value.
---@param b integer Highest value. Below the lowest raises.
---@return integer # A value in [a, b].
function Stream:randint(a, b)
  if b < a then
    error("b must not be below a", 2)
  end
  return a + below(self, b - a + 1)
end

---A whole number below a bound, counted from zero. The bound itself never
---comes up.
---@param n integer How many values there are. Below 1 raises.
---@return integer # A value in [0, n).
function Stream:randrange(n)
  if n < 1 then
    error("n must be 1 or more", 2)
  end
  return below(self, n)
end

---One item out of a list, each as likely as the next.
---@param seq any[] What to choose from. An empty list raises.
---@return any # The item chosen.
function Stream:choice(seq)
  local count = #seq
  if count == 0 then
    error("seq must not be empty", 2)
  end
  return seq[below(self, count) + 1]
end

---Several items out of a list, drawn one after another so that the same item
---can come up more than once. Weights give some items a greater share than
---others.
---@param seq any[] What to choose from. An empty list raises.
---@param weights? number[] One share per item, none of them negative and not all zero. Defaults to an equal share each.
---@param k? integer How many to draw. Below 0 raises.
---@return any[] # The items chosen.
---@trx.default k 1
function Stream:choices(seq, weights, k)
  local count = #seq
  if count == 0 then
    error("seq must not be empty", 2)
  end

  local wanted = k or 1
  if wanted < 0 then
    error("k must not be negative", 2)
  end

  local chosen = {}
  if weights == nil then
    for i = 1, wanted do
      chosen[i] = seq[below(self, count) + 1]
    end
    return chosen
  end

  if #weights ~= count then
    error("weights must hold one share per item", 2)
  end

  local running, total = {}, 0.0
  for i = 1, count do
    if weights[i] < 0 then
      error("weights must not be negative", 2)
    end
    total = total + weights[i]
    running[i] = total
  end
  if total <= 0 then
    error("weights must not be all zero", 2)
  end

  for i = 1, wanted do
    local point = fraction(self) * total
    -- The last item takes what rounding leaves past the final share.
    chosen[i] = seq[count]
    for j = 1, count do
      if point < running[j] then
        chosen[i] = seq[j]
        break
      end
    end
  end
  return chosen
end

---A direction, anywhere around the turn.
---@return trx.math.Angle # An angle within one turn.
function Stream:angle()
  return below(self, 0x10000)
end

---Whether something with the given likelihood happens this time.
---@param p number How likely, from 0 for never to 1 for always.
---@return boolean # Whether it happens.
function Stream:chance(p)
  return fraction(self) < p
end

local function stream_of(next_value)
  local handle = h.new(Stream)
  sources[handle] = next_value
  return handle
end

local control = stream_of(raw.next_control)
local draw = stream_of(raw.next_draw)

h.properties(M, "random", {
  control = {
    get = function()
      return control
    end,
  },
  draw = {
    get = function()
      return draw
    end,
  },
})

---A fraction of one, the whole number itself excepted.
---@return number # A value in [0, 1).
function M.random()
  return Stream.random(control)
end

---A whole number between two bounds, both of them included.
---
---```lua
---local pips = trx.random.randint(1, 6)
---```
---@param a integer Lowest value.
---@param b integer Highest value. Below the lowest raises.
---@return integer # A value in [a, b].
function M.randint(a, b)
  return Stream.randint(control, a, b)
end

---A whole number below a bound, counted from zero. The bound itself never
---comes up.
---
---```lua
---local side = trx.random.randrange(6) + 1
---```
---@param n integer How many values there are. Below 1 raises.
---@return integer # A value in [0, n).
function M.randrange(n)
  return Stream.randrange(control, n)
end

---One item out of a list, each as likely as the next.
---
---```lua
---local sample = trx.random.choice({
---  trx.catalog.samples.LARA_NO,
---  trx.catalog.samples.LARA_YES,
---})
---```
---@param seq any[] What to choose from. An empty list raises.
---@return any # The item chosen.
function M.choice(seq)
  return Stream.choice(control, seq)
end

---Several items out of a list, drawn one after another so that the same item
---can come up more than once. Weights give some items a greater share than
---others.
---
---```lua
---local drops = trx.random.choices({ "medipack", "ammo" }, { 1, 3 }, 5)
---```
---@param seq any[] What to choose from. An empty list raises.
---@param weights? number[] One share per item, none of them negative and not all zero. Defaults to an equal share each.
---@param k? integer How many to draw. Below 0 raises.
---@return any[] # The items chosen.
---@trx.default k 1
function M.choices(seq, weights, k)
  return Stream.choices(control, seq, weights, k)
end

---A direction, anywhere around the turn.
---
---```lua
---trx.lara.item.rot = { x = 0, y = trx.random.angle(), z = 0 }
---```
---@return trx.math.Angle # An angle within one turn.
function M.angle()
  return Stream.angle(control)
end

---Whether something with the given likelihood happens this time.
---
---```lua
---if trx.random.chance(0.25) then
---  trx.sound.play(trx.catalog.samples.LARA_NO)
---end
---```
---@param p number How likely, from 0 for never to 1 for always.
---@return boolean # Whether it happens.
function M.chance(p)
  return Stream.chance(control, p)
end
