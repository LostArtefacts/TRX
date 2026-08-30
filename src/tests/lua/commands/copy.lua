local h = require("harness")
local test = h.test

test("copy runs the command it is given", function()
  fake.set_eval_output("Level 1")
  assert(fake.run("copy", "pos") == trx.console.Result.OK)
  assert(fake.calls().eval.cmdline == "pos")
end)

test("copy puts the output in the clipboard", function()
  fake.set_eval_output("Level 1")
  assert(fake.run("copy", "pos") == trx.console.Result.OK)
  assert(fake.calls().clipboard.text == "Level 1")
end)

test("copy shows the output as the command itself does", function()
  fake.set_eval_output("Level 1")
  assert(fake.run("copy", "pos") == trx.console.Result.OK)
  assert(fake.calls().eval.verbose == true)
end)

test("copy reports a command that printed nothing", function()
  assert(fake.run("copy", "fly") == trx.console.Result.FAILURE)
  assert(fake.calls().clipboard.count == 0)
end)

test("copy reports a command that did not run", function()
  fake.set_eval_output("Level 1")
  fake.set_eval_result(fake.CommandResult.UNAVAILABLE)
  assert(fake.run("copy", "pos") == trx.console.Result.FAILURE)
  assert(fake.calls().clipboard.count == 0)
end)

test("copy needs a command", function()
  assert(fake.run("copy", "") == trx.console.Result.FAILURE)
  assert(fake.calls().eval.count == 0)
end)

test("copy completes the command it wraps", function()
  local names = fake.complete_args("copy", "co")
  assert(names[1] == "copy", "the console's own names did not reach it")
end)

return h.report()
