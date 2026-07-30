local h = require("harness")
local test = h.test

local function rule(args)
  return fake.run("rule", args or "")
end

test("no arguments list every rule", function()
  assert(rule("") == trx.console.Result.OK)
  assert(
    fake.calls().log.count == #trx.rules.list(),
    "every rule should have been reported"
  )
end)

test("an unknown rule reports as unknown", function()
  assert(rule("nonsense") == trx.console.Result.FAILURE)
  assert(fake.calls().log.message == "Unknown rule: nonsense")
end)

test("a name that could be several rules reports them", function()
  assert(rule("exposure.drain") == trx.console.Result.FAILURE)
  assert(
    fake.calls().log.message
      == "Ambiguous input: exposure.drain-land and exposure.drain-water"
  )
end)

test("a bare name reports the current value", function()
  assert(rule("exposure.damage") == trx.console.Result.OK)
  assert(fake.calls().log.message == "exposure.damage is currently set to 10")
end)

test("a name and a value change the rule", function()
  assert(rule("exposure.damage 25") == trx.console.Result.OK)
  assert(trx.rules.exposure.damage == 25, "the value did not land")
  assert(fake.calls().log.message == "exposure.damage changed to 25")
end)

test("a dash puts the default back", function()
  trx.rules.exposure.damage = 25
  assert(rule("exposure.damage -") == trx.console.Result.OK)
  assert(trx.rules.exposure.damage == 10, "the default did not come back")
end)

test("a value that will not parse leaves the rule alone", function()
  assert(rule("exposure.damage plenty") == trx.console.Result.FAILURE)
  assert(trx.rules.exposure.damage == 10, "the rule must be left alone")
  assert(fake.calls().log.message == "Invalid invocation: plenty")
end)

test("a rule reads back the way the console shows it", function()
  trx.rules.exposure.drain_water = 4
  assert(trx.rules.format_value("exposure.drain_water") == "4")
end)

return h.report()
