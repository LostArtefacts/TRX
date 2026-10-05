-- The numbers a script draws, the first case pinning them to the engine's own
-- sequence.

local h = require("harness")
local test = h.test

-- randint over the width of one draw takes exactly one and passes it through,
-- which is what lets these be the generator's own numbers. The seed is set in
-- random.c, so this case has to run before anything else draws.
test("a script draws what the engine draws", function()
  assert(trx.random.randint(0, 0x7FFF) == 31118)
  assert(trx.random.randint(0, 0x7FFF) == 27161)
  assert(trx.random.randint(0, 0x7FFF) == 18162)
end)

test("random reports a fraction of one", function()
  for _ = 1, 200 do
    local value = trx.random.random()
    assert(value >= 0.0 and value < 1.0)
  end
end)

test("randint holds to its bounds, both included", function()
  local low, high = false, false
  for _ = 1, 500 do
    local value = trx.random.randint(1, 6)
    assert(math.type(value) == "integer")
    assert(value >= 1 and value <= 6)
    low = low or value == 1
    high = high or value == 6
  end
  assert(low and high, "both bounds come up")
end)

test("randint over a single value has nothing to choose", function()
  assert(trx.random.randint(4, 4) == 4)
  assert(trx.random.randint(-3, -3) == -3)
end)

test("randint spans negative bounds", function()
  for _ = 1, 200 do
    local value = trx.random.randint(-5, 5)
    assert(value >= -5 and value <= 5)
  end
end)

test("randint raises where the bounds are the wrong way round", function()
  h.raises(function()
    trx.random.randint(6, 1)
  end)
end)

test("choice takes an item out of the list", function()
  local seq = { "a", "b", "c" }
  local seen = {}
  for _ = 1, 300 do
    local item = trx.random.choice(seq)
    assert(item == "a" or item == "b" or item == "c")
    seen[item] = true
  end
  assert(seen.a and seen.b and seen.c, "every item comes up")
end)

test("choice raises on an empty list", function()
  h.raises(function()
    trx.random.choice({})
  end)
end)

test("choices draws as many as asked for", function()
  assert(#trx.random.choices({ "a", "b" }, nil, 5) == 5)
  assert(#trx.random.choices({ "a", "b" }) == 1, "one by default")
  assert(#trx.random.choices({ "a", "b" }, nil, 0) == 0)
end)

test("choices leaves out what carries no weight", function()
  local drawn = trx.random.choices({ "never", "always" }, { 0, 1 }, 100)
  assert(#drawn == 100)
  for _, item in ipairs(drawn) do
    assert(item == "always")
  end
end)

test("choices follows the shares it is given", function()
  local counts = { a = 0, b = 0 }
  for _, item in ipairs(trx.random.choices({ "a", "b" }, { 1, 9 }, 2000)) do
    counts[item] = counts[item] + 1
  end
  assert(counts.a > 0 and counts.b > counts.a, "the greater share wins out")
end)

test("choices raises on weights it cannot use", function()
  h.raises(function()
    trx.random.choices({ "a", "b" }, { 1 })
  end)
  h.raises(function()
    trx.random.choices({ "a", "b" }, { 1, -1 })
  end)
  h.raises(function()
    trx.random.choices({ "a", "b" }, { 0, 0 })
  end)
  h.raises(function()
    trx.random.choices({ "a", "b" }, nil, -1)
  end)
end)

test("angle stays within one turn", function()
  local seen_high = false
  for _ = 1, 500 do
    local angle = trx.random.angle()
    assert(math.type(angle) == "integer")
    assert(angle >= 0 and angle < 0x10000)
    seen_high = seen_high or angle > 0x8000
  end
  assert(seen_high, "the far half of the turn comes up")
end)

test("chance answers to the certainties", function()
  for _ = 1, 100 do
    assert(not trx.random.chance(0))
    assert(trx.random.chance(1))
  end
end)

test("chance falls near the likelihood it is given", function()
  local hits = 0
  for _ = 1, 2000 do
    if trx.random.chance(0.25) then
      hits = hits + 1
    end
  end
  assert(hits > 350 and hits < 650, "about a quarter of 2000, got " .. hits)
end)

test("strict mode holds the arguments to their types", function()
  trx.api.strict(true)
  h.raises(function()
    trx.random.randint(1.5, 6)
  end)
  h.raises(function()
    trx.random.chance("half")
  end)
  trx.api.strict(false)
  assert(trx.random.randint(2, 2) == 2)
end)

return h.report()
