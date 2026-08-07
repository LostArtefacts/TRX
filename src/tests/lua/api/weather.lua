local h = require("harness")
local test = h.test

test("the weather types are the reflected enum", function()
  assert(trx.weather.Type.NONE == 0)
  assert(trx.weather.Type.RAIN == 1)
  assert(trx.weather.Type.SNOW == 2)
end)

test("set changes the weather, and current reads it back", function()
  trx.weather.set(trx.weather.Type.SNOW)
  assert(trx.weather.current == trx.weather.Type.SNOW)

  trx.weather.set(trx.weather.Type.NONE)
  assert(trx.weather.current == trx.weather.Type.NONE)
end)

test("severity reads back what it was set to", function()
  assert(trx.weather.severity == 1)

  trx.weather.severity = 2.5
  assert(trx.weather.severity == 2.5)
end)

test("a severity outside the range is clamped to it", function()
  trx.weather.severity = -1
  assert(trx.weather.severity == 0)

  trx.weather.severity = 100
  assert(trx.weather.severity == 4)
end)

test("an out-of-range weather is refused", function()
  local ok = pcall(trx.weather.set, 99)
  assert(not ok, "an unknown weather type must raise")
end)

return h.report()
