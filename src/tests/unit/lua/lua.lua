-- The eval API as a script actually sees it.

local h = require("harness")
local test = h.test

test("an expression runs in the shared state", function()
  fake.witness = nil
  assert(trx.lua.eval_expr("fake.witness = 7") == nil)
  assert(fake.witness == 7, "the chunk did not reach the caller's state")
end)

test("code that does not parse reports a syntax failure", function()
  local err = trx.lua.eval_expr("this is not lua")
  assert(err ~= nil)
  assert(err.kind == "syntax", err.kind)
  assert(err.message ~= "", "the error text was dropped")
end)

test("code that raises reports a runtime failure", function()
  local err = trx.lua.eval_expr("error('boom')")
  assert(err ~= nil)
  assert(err.kind == "runtime", err.kind)
  assert(err.message:find("boom"), err.message)
end)

test("a file runs in the shared state", function()
  fake.eval_file_witness = nil
  assert(trx.lua.eval_file(fake.script_dir .. "eval_file_target.lua") == nil)
  assert(fake.eval_file_witness == "ran", "the file did not run")
end)

test("a file that cannot be read reports a runtime failure", function()
  local err = trx.lua.eval_file(fake.script_dir .. "no_such_file.lua")
  assert(err ~= nil)
  assert(err.kind == "runtime", err.kind)
  assert(err.message ~= "", "the error text was dropped")
end)

return h.report()
