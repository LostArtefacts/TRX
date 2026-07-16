-- The string utilities as a script actually sees them.
--
-- The matcher is the real one out of trx/core/strings, so what these assert is
-- what the console does when a player types an object name.

local h = require("harness")
local test, raises = h.test, h.raises

local function sources()
  return {
    { key = "wolf", value = "WOLF" },
    { key = "bear", value = "BEAR" },
    { key = "large medipack", value = "MEDI_BIG" },
    { key = "small medipack", value = "MEDI_SMALL" },
  }
end

test("an exact name matches itself", function()
  local matches = trx.strings.fuzzy_match("wolf", sources())
  assert(#matches > 0, "the wolf did not match")
  assert(matches[1].value == "WOLF", "the wolf was not the best match")
  assert(matches[1].key == "wolf")
  assert(matches[1].is_full == true, "an exact name is a full match")
end)

test("the value rides along untouched", function()
  -- It is the caller's, and the matcher never looks at it. Hang an object id off
  -- a candidate and read it back.
  local matches = trx.strings.fuzzy_match("bear", {
    { key = "bear", value = { id = 8, tag = "anything" } },
  })
  assert(matches[1].value.id == 8)
  assert(matches[1].value.tag == "anything")
end)

test("a partial name matches, which is the point", function()
  -- `big medi` is what a player types; `large medipack` is what it is called.
  local matches = trx.strings.fuzzy_match("medi", sources())
  assert(#matches > 0, "medi matched nothing")
  for _, match in ipairs(matches) do
    assert(
      match.key:find("medi"),
      "matched something with no medi in it: " .. match.key
    )
  end
end)

test("a name nobody has matches nothing", function()
  assert(#trx.strings.fuzzy_match("wombat", sources()) == 0)
end)

test("weight breaks a tie", function()
  -- Two candidates spelled the same; the heavier one wins.
  local matches = trx.strings.fuzzy_match("thing", {
    { key = "thing", value = "LIGHT", weight = 1 },
    { key = "thing", value = "HEAVY", weight = 10 },
  })
  assert(matches[1].value == "HEAVY", "the heavier candidate must win")
end)

test("an empty source list matches nothing, rather than raising", function()
  assert(#trx.strings.fuzzy_match("wolf", {}) == 0)
end)

test("a key that is not a string is refused", function()
  raises(function()
    trx.strings.fuzzy_match("wolf", { { key = 8, value = "WOLF" } })
  end, "string key")
end)

test("an empty key is refused", function()
  raises(function()
    trx.strings.fuzzy_match("wolf", { { key = "", value = "WOLF" } })
  end, "empty")
end)

test("a weight that is not an integer is refused", function()
  raises(function()
    trx.strings.fuzzy_match("wolf", { { key = "wolf", weight = "heavy" } })
  end, "integer")
end)

test("a source that is not a table is refused", function()
  raises(function()
    trx.strings.fuzzy_match("wolf", { "wolf" })
  end, "list of tables")
end)

test("regex_match answers, and ignores case", function()
  assert(trx.strings.regex_match("wolf", "^wo") == true)
  assert(
    trx.strings.regex_match("WOLF", "^wo") == true,
    "the match is case-insensitive"
  )
  assert(trx.strings.regex_match("bear", "^wo") == false)
end)

test("the pattern is PCRE, not a Lua pattern", function()
  -- `%d` is what Lua's own string library wants; PCRE reads it as a literal.
  assert(trx.strings.regex_match("42", "^\\d+$") == true)
  assert(trx.strings.regex_match("42", "^%d+$") == false)
end)

test("collapse_ranges writes a run as a range", function()
  local collapse = trx.strings.collapse_ranges

  assert(collapse({}) == "", "an empty list is an empty string")
  assert(collapse({ 7 }) == "7")
  assert(collapse({ 1, 2, 3, 4 }) == "1-4")
  assert(collapse({ 0, 2, 3, 4, 9 }) == "0, 2-4, 9")
  assert(collapse({ 1, 3, 5 }) == "1, 3, 5", "gaps must not be bridged")
end)

test("collapse_ranges sorts, and takes a separator", function()
  local collapse = trx.strings.collapse_ranges

  assert(collapse({ 4, 1, 3, 2 }) == "1-4", "the caller need not sort first")
  assert(collapse({ 1, 2, 5 }, " | ") == "1-2 | 5")
end)

return h.report()
