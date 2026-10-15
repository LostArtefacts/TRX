-- The shipped value helpers, which shipped widgets read their settings with.

local h = require("harness")
local test = h.test

local value = require("common.ui.value")

test("a value reads as itself and a signal as what it holds", function()
  assert(value.read(3) == 3)
  assert(value.read(trx.signal.new(4)) == 4)
end)

test("a plain value becomes a signal that holds it", function()
  local signal = value.signal(5)
  assert(signal:get() == 5)
  local same = trx.signal.new(6)
  assert(value.signal(same) == same)
end)

test("a pick follows the signal its index names", function()
  local index = trx.signal.new(1)
  local a, b, c = trx.signal.new("a"), trx.signal.new("b"), trx.signal.new("c")
  local picked = value.pick(index, { a, b, c })
  assert(picked:get() == "a")
  index:set(3)
  assert(picked:get() == "c")
  c:set("z")
  assert(picked:get() == "z")
end)

return h.report()
