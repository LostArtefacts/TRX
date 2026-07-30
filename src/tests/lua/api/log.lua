-- The logging API as a script actually sees it.

local h = require("harness")
local test, raises = h.test, h.raises

test("each function logs at its own level", function()
  local cases = {
    { fn = trx.log.info, level = trx.log.LogLevel.INFO },
    { fn = trx.log.warn, level = trx.log.LogLevel.WARNING },
    { fn = trx.log.warning, level = trx.log.LogLevel.WARNING },
    { fn = trx.log.error, level = trx.log.LogLevel.ERROR },
    { fn = trx.log.debug, level = trx.log.LogLevel.DEBUG },
  }
  for _, case in ipairs(cases) do
    case.fn("msg")
    assert(fake.calls().log.level == case.level, "wrong level")
  end
  assert(fake.calls().log.count == #cases, "not every level reached the log")
end)

test("generic logs at the level it is handed", function()
  trx.log.generic(trx.log.LogLevel.ERROR, "boom")
  local calls = fake.calls()
  assert(calls.log.level == trx.log.LogLevel.ERROR)
  assert(calls.log.message == "boom")
end)

test("the level values come from C", function()
  -- DEBUG is 0 and ERROR is 3 in the C enum, least to most important.
  assert(trx.log.LogLevel.DEBUG == 0)
  assert(trx.log.LogLevel.INFO == 1)
  assert(trx.log.LogLevel.WARNING == 2)
  assert(trx.log.LogLevel.ERROR == 3)
end)

test("the log blames the line that called it, not the wrapper", function()
  local expected = debug.getinfo(1, "l").currentline + 1
  trx.log.info("who called me")
  assert(
    fake.calls().log.line == expected,
    ("blamed line %d, called from %d"):format(fake.calls().log.line, expected)
  )
end)

test("generic blames its caller too", function()
  local expected = debug.getinfo(1, "l").currentline + 1
  trx.log.generic(trx.log.LogLevel.INFO, "and me")
  assert(fake.calls().log.line == expected, "generic blamed the wrong line")
end)

return h.report()
