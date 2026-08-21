local raw = trxc.random
local api = trx.api

require("trx.math")

api.module("random", {
  order = 32,
  description = [[
Random numbers, drawn from the sequence the simulation itself runs on.

The savegame carries that sequence, so what a script draws comes back the same
after a reload, and a script needs no seed of its own. The sequence moves on
for the engine as well: a script drawing every frame changes what the creatures
decide next.

Lua's own `math.random` is a separate generator that nothing saves. It has no
place in anything the simulation reads. <!--noref: math.random-->]],
})

-- How wide one draw of the engine's stream is, as the generator states it.
local SPAN = raw.SPAN
-- Two of them make the fraction random() reports.
local FRACTION = SPAN * SPAN
-- Four draws is as wide a range as below() can hold without leaving what a Lua
-- integer counts.
local MAX_SPAN = SPAN * SPAN * SPAN * SPAN

-- A whole number below `n`. The top of the range is thrown away and drawn
-- again, so that no value comes up more often than another.
local function below(n)
  if n > MAX_SPAN then
    error("range is too wide", 3)
  end

  local pow, draws = SPAN, 1
  while pow < n do
    pow = pow * SPAN
    draws = draws + 1
  end

  local limit = pow - (pow % n)
  while true do
    local value = 0
    for _ = 1, draws do
      value = value * SPAN + raw.next()
    end
    if value < limit then
      return value % n
    end
  end
end

local function fraction()
  return (raw.next() * SPAN + raw.next()) / FRACTION
end

api.define("random.random", {
  description = "A fraction of one, the whole number itself excepted.",
  returns = { type = "number", description = "A value in [0, 1)." },
  impl = fraction,
})

api.define("random.randint", {
  description = "A whole number between two bounds, both of them included.",
  params = {
    { name = "a", type = "integer", description = "Lowest value." },
    {
      name = "b",
      type = "integer",
      description = "Highest value. Below the lowest raises.",
    },
  },
  returns = { type = "integer", description = "A value in [a, b]." },
  examples = { [[local pips = trx.random.randint(1, 6)]] },
  impl = function(a, b)
    if b < a then
      error("b must not be below a", 2)
    end
    return a + below(b - a + 1)
  end,
})

api.define("random.choice", {
  description = "One item out of a list, each as likely as the next.",
  params = {
    {
      name = "seq",
      type = "any",
      list = true,
      description = "What to choose from. An empty list raises.",
    },
  },
  returns = { type = "any", description = "The item chosen." },
  examples = {
    [[local sample = trx.random.choice({
  trx.catalog.samples.LARA_NO,
  trx.catalog.samples.LARA_YES,
})]],
  },
  impl = function(seq)
    local count = #seq
    if count == 0 then
      error("seq must not be empty", 2)
    end
    return seq[below(count) + 1]
  end,
})

api.define("random.choices", {
  description = "Several items out of a list, drawn one after another so that the same item can "
    .. "come up more than once. Weights give some items a greater share than others.",
  params = {
    {
      name = "seq",
      type = "any",
      list = true,
      description = "What to choose from. An empty list raises.",
    },
    {
      name = "weights",
      type = "number",
      list = true,
      optional = true,
      description = "One share per item, none of them negative and not all zero. Defaults to an "
        .. "equal share each.",
    },
    {
      name = "k",
      type = "integer",
      optional = true,
      default = 1,
      description = "How many to draw. Below 0 raises.",
    },
  },
  returns = { type = "any", list = true, description = "The items chosen." },
  examples = {
    [[local drops = trx.random.choices({ "medipack", "ammo" }, { 1, 3 }, 5)]],
  },
  impl = function(seq, weights, k)
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
        chosen[i] = seq[below(count) + 1]
      end
      return chosen
    end

    if #weights ~= count then
      error("weights must hold one share per item", 2)
    end

    local running, total = {}, 0
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
      local point = fraction() * total
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
  end,
})

api.define("random.angle", {
  description = "A direction, anywhere around the turn.",
  params = {},
  returns = { type = "math.Angle", description = "An angle within one turn." },
  examples = {
    [[trx.lara.item.rot = { x = 0, y = trx.random.angle(), z = 0 }]],
  },
  impl = function()
    return below(0x10000)
  end,
})

api.define("random.chance", {
  description = "Whether something with the given likelihood happens this time.",
  params = {
    {
      name = "p",
      type = "number",
      description = "How likely, from 0 for never to 1 for always.",
    },
  },
  returns = { type = "boolean", description = "Whether it happens." },
  examples = {
    [[if trx.random.chance(0.25) then
  trx.sound.play(trx.catalog.samples.LARA_NO)
end]],
  },
  impl = function(p)
    return fraction() < p
  end,
})
