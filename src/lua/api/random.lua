local raw = trxc.random
local api = trx.api

require("trx.math")

api.module("random", {
  order = 32,
  description = [[
Random numbers, drawn from one of the two sequences the engine runs on.

The module's own calls draw from the control stream. This is the sequence the
simulation runs on, so a script that draws every frame changes what the
creatures decide next. The draw stream, `trx.random.draw`, is the one the
original game keeps for what is only seen. Drawing from it leaves the
simulation as it was. Both streams are the same generator and offer the same
calls, described in `trx.random.Stream`.

The savegame carries both sequences. A script's draws come back the same
after a reload, and a script needs no seed of its own.

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

-- Maps each stream handle to the sequence it draws from. A handle is an empty
-- table, so the sequence is reachable only through this map.
local sources = setmetatable({}, { __mode = "k" })
local Stream

local function stream_of(next_value)
  local handle = setmetatable({}, Stream)
  sources[handle] = next_value
  return handle
end

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

local function random_impl(self)
  return fraction(self)
end

local function randint_impl(self, a, b)
  if b < a then
    error("b must not be below a", 2)
  end
  return a + below(self, b - a + 1)
end

local function choice_impl(self, seq)
  local count = #seq
  if count == 0 then
    error("seq must not be empty", 2)
  end
  return seq[below(self, count) + 1]
end

local function choices_impl(self, seq, weights, k)
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

local function angle_impl(self)
  return below(self, 0x10000)
end

local function chance_impl(self, p)
  return fraction(self) < p
end

local RANDOM = {
  description = "A fraction of one, the whole number itself excepted.",
  returns = { type = "number", description = "A value in [0, 1)." },
}

local RANDINT = {
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
}

local CHOICE = {
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
}

local CHOICES = {
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
}

local ANGLE = {
  description = "A direction, anywhere around the turn.",
  params = {},
  returns = { type = "math.Angle", description = "An angle within one turn." },
}

local CHANCE = {
  description = "Whether something with the given likelihood happens this time.",
  params = {
    {
      name = "p",
      type = "number",
      description = "How likely, from 0 for never to 1 for always.",
    },
  },
  returns = { type = "boolean", description = "Whether it happens." },
}

-- Builds one API entry per call, so that a stream's method and the module's
-- own function describe the same parameters and returns.
local function spec(base, extra)
  local entry = {}
  for key, value in pairs(base) do
    entry[key] = value
  end
  for key, value in pairs(extra) do
    entry[key] = value
  end
  return entry
end

Stream = api.type("random.Stream", {
  description = [[
    One of the engine's two random sequences.

    Drawing from `trx.random.control` changes what the game does next,
    because the simulation runs on it. Drawing from `trx.random.draw`
    changes nothing, because only the picture uses it.

    Both have the same calls. The module's own functions draw from the
    control stream.
  ]],

  methods = {
    random = spec(RANDOM, { impl = random_impl }),

    randint = spec(RANDINT, {
      examples = { [[local pips = trx.random.draw:randint(1, 6)]] },
      impl = randint_impl,
    }),

    choice = spec(CHOICE, { impl = choice_impl }),
    choices = spec(CHOICES, { impl = choices_impl }),
    angle = spec(ANGLE, { impl = angle_impl }),
    chance = spec(CHANCE, { impl = chance_impl }),
  },
})

local control = stream_of(raw.next_control)
local draw = stream_of(raw.next_draw)

api.property("random.control", {
  type = "random.Stream",
  description = "The sequence the simulation runs on, which the module's own functions draw from.",
  get = function()
    return control
  end,
})

api.property("random.draw", {
  type = "random.Stream",
  description = "The sequence kept for what is only seen. Drawing from it leaves what the "
    .. "creatures decide next as it was, which is what the original game keeps it for.",
  examples = {
    [[trx.fx.blood({
  pos = { x = 6799 - trx.random.draw:randint(0, 255), y = -512, z = 76209 },
  strength = 7,
})]],
  },
  get = function()
    return draw
  end,
})

api.define(
  "random.random",
  spec(RANDOM, {
    impl = function()
      return random_impl(control)
    end,
  })
)

api.define(
  "random.randint",
  spec(RANDINT, {
    examples = { [[local pips = trx.random.randint(1, 6)]] },
    impl = function(a, b)
      return randint_impl(control, a, b)
    end,
  })
)

api.define(
  "random.choice",
  spec(CHOICE, {
    examples = {
      [[local sample = trx.random.choice({
  trx.catalog.samples.LARA_NO,
  trx.catalog.samples.LARA_YES,
})]],
    },
    impl = function(seq)
      return choice_impl(control, seq)
    end,
  })
)

api.define(
  "random.choices",
  spec(CHOICES, {
    examples = {
      [[local drops = trx.random.choices({ "medipack", "ammo" }, { 1, 3 }, 5)]],
    },
    impl = function(seq, weights, k)
      return choices_impl(control, seq, weights, k)
    end,
  })
)

api.define(
  "random.angle",
  spec(ANGLE, {
    examples = {
      [[trx.lara.item.rot = { x = 0, y = trx.random.angle(), z = 0 }]],
    },
    impl = function()
      return angle_impl(control)
    end,
  })
)

api.define(
  "random.chance",
  spec(CHANCE, {
    examples = {
      [[if trx.random.chance(0.25) then
  trx.sound.play(trx.catalog.samples.LARA_NO)
end]],
    },
    impl = function(p)
      return chance_impl(control, p)
    end,
  })
)
