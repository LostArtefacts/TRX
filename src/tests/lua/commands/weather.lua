-- /weather, dispatched through the console. Both arguments are optional and the
-- weather name comes first, so a lone number has to reach the severity: that is
-- the parser passing the token over.

local h = require("harness")
local test = h.test
local R = trx.console.Result

local function weather(args)
  return fake.run("weather", args or "")
end

test("weather needs a level", function()
  fake.set_current_level(nil)
  assert(weather("rain") == R.UNAVAILABLE)
end)

test("no argument reports what is falling", function()
  trx.weather.set(trx.weather.Type.RAIN)

  assert(weather() == R.OK)
  assert(trx.weather.current == trx.weather.Type.RAIN)
  assert(trx.weather.severity == 1)
end)

test("a name on its own sets the weather", function()
  assert(weather("snow") == R.OK)
  assert(trx.weather.current == trx.weather.Type.SNOW)
  assert(trx.weather.severity == 1)
end)

test("a name and a number set both", function()
  assert(weather("rain 2.5") == R.OK)
  assert(trx.weather.current == trx.weather.Type.RAIN)
  assert(trx.weather.severity == 2.5)
end)

test("a number on its own leaves the weather alone", function()
  trx.weather.set(trx.weather.Type.SNOW)

  assert(weather("3") == R.OK)
  assert(trx.weather.current == trx.weather.Type.SNOW)
  assert(trx.weather.severity == 3)
end)

test("a severity past the range is clamped to it", function()
  assert(weather("snow 100") == R.OK)
  assert(trx.weather.severity == 4)
end)

test("an unknown weather is refused", function()
  assert(weather("fog") == R.FAILURE)
end)

return h.report()
