local h = require("harness")
local test = h.test

local function heal(args)
  return fake.run("heal", args or "")
end

test("heal takes no arguments", function()
  assert(heal("me") == trx.console.Result.BAD_INVOCATION)
end)

test("heal needs a level", function()
  fake.set_current_level(nil)
  assert(heal() == trx.console.Result.UNAVAILABLE)
end)

test("heal does not run in a cutscene", function()
  fake.set_in_cutscene(true)
  assert(heal() == trx.console.Result.UNAVAILABLE)
end)

test("heal fills Lara up, cures her and puts her out", function()
  local lara = trx.lara.item
  lara.hit_points = 1

  assert(heal() == trx.console.Result.OK)
  assert(lara.hit_points == lara.max_hit_points, "Lara was left hurt")

  local calls = fake.calls()
  assert(calls.cure_poison == 1, "the poison was left behind")
  assert(calls.extinguish == 1, "Lara was left burning")
  assert(calls.last_level == trx.log.LogLevel.INFO)
  assert(calls.last_message == "console/cmd/heal/success")
end)

test("an unhurt Lara is still cured and put out", function()
  local lara = trx.lara.item
  assert(lara.hit_points == lara.max_hit_points, "she starts unhurt")

  assert(heal() == trx.console.Result.OK)

  local calls = fake.calls()
  assert(calls.cure_poison == 1, "the poison was left behind")
  assert(calls.extinguish == 1, "Lara was left burning")
  assert(calls.last_level == trx.log.LogLevel.WARNING)
  assert(calls.last_message == "console/cmd/heal/already_full_hp")
end)

return h.report()
