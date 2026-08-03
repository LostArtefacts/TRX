-- The JSON encoder as a script actually sees it. The API dump goes through
-- this, so what it writes is what the committed reference is generated from.

local h = require("harness")
local test = h.test

test("a scalar writes as itself", function()
  local encode = trx.json.encode

  assert(encode(true) == "true")
  assert(encode(false) == "false")
  assert(encode(7) == "7")
  assert(encode("wolf") == '"wolf"')
end)

test("a table with entries writes as a list", function()
  local encode = trx.json.encode

  assert(encode({ 1, 2, 3 }) == "[1,2,3]")
  assert(encode({ "a", "b" }) == '["a","b"]')
  assert(encode({}) == "[]", "nothing at all reads as none of it")
end)

test("a table with keys writes as an object, in sorted order", function()
  -- pairs() walks a table in any order, and the dump is committed and diffed.
  local written = trx.json.encode({ name = "wolf", ids = { 7, 8 }, hp = 6 })
  assert(
    written == '{"hp":6,"ids":[7,8],"name":"wolf"}',
    "keys must come out sorted: " .. written
  )
end)

test(
  "a string carries its quotes and its control characters through",
  function()
    local encode = trx.json.encode

    assert(encode('say "hi"') == '"say \\"hi\\""')
    assert(encode("back\\slash") == '"back\\\\slash"')
    assert(encode("two\nlines") == '"two\\nlines"')
    assert(encode("a\tb") == '"a\\tb"')
    assert(
      encode("bell\a") == '"bell\\u0007"',
      "a control character with no escape of its own still has to be escaped"
    )
  end
)

test("nesting comes out nested", function()
  local written = trx.json.encode({
    rooms = { { num = 0 }, { num = 1 } },
  })
  assert(written == '{"rooms":[{"num":0},{"num":1}]}', written)
end)

return h.report()
