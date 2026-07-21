local h = require("harness")
local test = h.test

local function set(args)
  return fake.run("set", args or "")
end

test("set wants at least an option name", function()
  assert(set("") == trx.console.Result.BAD_INVOCATION)
end)

test("an unknown option reports as unknown", function()
  assert(set("nonsense") == trx.console.Result.FAILURE)
  assert(fake.calls().last_message == "console/cmd/set/unknown_option")
end)

test("a name that could be several options reports them", function()
  assert(set("visuals") == trx.console.Result.FAILURE)
  assert(fake.calls().last_message == "console/cmd/set/ambiguous_3")
end)

test("a bare name reports the current value", function()
  assert(set("fov") == trx.console.Result.OK)
  assert(fake.calls().last_message == "console/cmd/set/option_get")
end)

test("a name and a value change the setting", function()
  assert(set("fov 90") == trx.console.Result.OK)
  assert(trx.config.get("visuals.fov") == 90, "the value did not land")
  assert(fake.calls().last_message == "console/cmd/set/option_set")
end)

test("a dash puts the default back", function()
  trx.config.set("visuals.fov", 90)
  assert(set("fov -") == trx.console.Result.OK)
  assert(trx.config.get("visuals.fov") == 65, "the default did not come back")
end)

test("a value that will not parse lists what is valid", function()
  assert(set("fov wide") == trx.console.Result.FAILURE)
  assert(trx.config.get("visuals.fov") == 65, "the option must be left alone")
  -- The invalid-value error logs first; the valid values line follows it.
  assert(fake.calls().last_message == "console/cmd/set/valid_values")
end)

test("an enum value typed with dashes still lands", function()
  assert(set("shadow-type extra-dark") == trx.console.Result.OK)
  assert(trx.config.get("visuals.shadow_type") == "extra_dark")
end)

test("a held setting reports as enforced, and -f writes through", function()
  trx.config.override("visuals.fov", 100)

  assert(set("fov 90") == trx.console.Result.FAILURE)
  assert(fake.calls().last_message == "console/cmd/set/option_enforced")

  assert(set("-f fov 90") == trx.console.Result.OK)
  assert(trx.config.get("visuals.fov") == 90, "the forced write did not land")
end)

return h.report()
