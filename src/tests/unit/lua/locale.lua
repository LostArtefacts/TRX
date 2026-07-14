-- The locale API as a script actually sees it.

local h = require("harness")
local test = h.test

test("a key reads back the text behind it", function()
  assert(trx.locale.get("test/plain") == "Plain text")
end)

test("a key nobody has reads back as itself", function()
  -- So a typo shows up on the screen, where someone will see it, rather than as
  -- a nil three lines further down.
  assert(trx.locale.get("test/missing") == "test/missing")
end)

test("format fills the placeholders in", function()
  assert(trx.locale.format("test/formatted", 3) == "Text with 3 in it")
end)

test("format of a key nobody has gives back the key", function()
  -- The key has no placeholders in it, so formatting it is a no-op rather than
  -- an error.
  assert(trx.locale.format("test/missing") == "test/missing")
end)

test("format of text a translator put a bare percent sign in", function()
  -- The player gets the text rather than an error out of string.format.
  assert(trx.locale.format("test/percent") == "100% of the text")
end)

test("format keeps every argument under strict", function()
  -- The checked wrapper is generated per parameter list, and format's is a "..."
  -- one: an arity pinned to the named parameters would drop the rest.
  trx.api.strict(true)
  local ok, result = pcall(trx.locale.format, "test/two", 1, 5)
  trx.api.strict(false)
  assert(ok, result)
  assert(result == "1 of 5", result)
end)

return h.report()
