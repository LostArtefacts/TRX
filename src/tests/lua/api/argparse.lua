-- The argument parser as a command actually uses it: real fuzzy matching out of
-- trx.strings, so parsing and completion read the way the console does. parse
-- returns structured errors, which the console layer turns into localized text.

local h = require("harness")
local test, raises = h.test, h.raises

test("a positional comes back under its name", function()
  local p = trx.argparse.new({ prog = "weather" })
  p:positional("state", { choices = { "snow", "rain", "none" } })
  assert(p:parse("snow").state == "snow")
end)

test("choices restrict, and an unknown one is rejected", function()
  local p = trx.argparse.new()
  p:positional("state", { choices = { "snow", "rain", "none" } })
  local parsed, err = p:parse("hail")
  assert(parsed == nil, "hail is not a choice")
  assert(err.kind == "invalid" and err.token == "hail")
end)

test("a required positional must be given", function()
  local p = trx.argparse.new()
  p:positional("state", { choices = { "snow", "rain" } })
  local parsed, err = p:parse("")
  assert(parsed == nil)
  assert(err.kind == "missing" and err.metavar == "state")
end)

test("an optional positional may be absent", function()
  local p = trx.argparse.new()
  p:positional("state", { optional = true, choices = { "snow" } })
  assert(p:parse("").state == nil, "absent is nil")
end)

test("an integer positional is a number", function()
  local p = trx.argparse.new()
  p:positional("id", { type = "integer" })
  assert(p:parse("42").id == 42)
  assert(p:parse("-3").id == -3, "a negative whole number is fine")
  assert(p:parse("nope") == nil, "a non-integer is rejected")
  assert(p:parse("3.5") == nil, "a fractional value is not an integer")
  assert(p:parse("0x10") == nil, "hex is not an integer")
end)

test("a number reads plain decimals, not hex or scientific", function()
  local p = trx.argparse.new()
  p:positional("factor", { type = "number" })
  assert(p:parse("2.5").factor == 2.5)
  assert(p:parse("-0.5").factor == -0.5)
  assert(p:parse(".5").factor == 0.5)
  assert(p:parse("nope") == nil)
  assert(p:parse("0x10") == nil, "hex is not a plain number")
  assert(p:parse("1e5") == nil, "scientific is not a plain number")
end)

test("a choices function that raises is a clean rejection", function()
  local p = trx.argparse.new()
  p:positional("x", {
    choices = function()
      error("boom")
    end,
  })
  local parsed, err = p:parse("anything")
  assert(parsed == nil, "the raise does not propagate out of parse")
  assert(err.kind == "invalid")
end)

test("a positional reads a token one way, not several", function()
  local p = trx.argparse.new()
  raises(function()
    p:positional("level", { type = "integer", choices = { "a" } })
  end, "any_of")
end)

test("any_of tries its matchers in turn", function()
  -- /play: a number is the ordinal, a name resolves to one.
  local p = trx.argparse.new()
  p:any_of("level", {
    { type = "integer" },
    {
      choices = { { key = "Caves", value = 1 }, { key = "City", value = 2 } },
    },
  })
  assert(p:parse("3").level == 3, "a number reads as itself")
  assert(p:parse("caves").level == 1, "a name resolves to its value")
  local _, err = p:parse("nope")
  assert(err.hint.type == "integer", "the hint keeps the number")
  local seen = {}
  for _, v in ipairs(err.hint.values) do
    seen[v] = true
  end
  assert(seen.Caves and seen.City, "and lists the names")
end)

test("a greedy argument takes the whole line, spaces and all", function()
  -- /play: a number, or a level name that has spaces in it.
  local p = trx.argparse.new()
  p:any_of("level", {
    { type = "integer" },
    { choices = { { key = "Natla's Mines", value = 13 } } },
  }, { greedy = true })
  assert(p:parse("3").level == 3)
  assert(p:parse("natla's mines").level == 13, "the whole name is matched")
  local out, start, stop = p:complete("natla's mi")
  assert(out[1] == "Natla's Mines", "completion sees the whole tail too")
  assert(start == 0, "the run it replaces reaches back to the tail's start")
  assert(stop == 10, "the greedy run reaches to the end of the line")
end)

test("a greedy run reaches the line end from a caret mid-tail", function()
  local p = trx.argparse.new()
  p:positional("level", { suggest = { "Angkor Wat" }, greedy = true })
  -- "angkor wa" with the caret after "angko": the run still spans the tail.
  local out, start, stop = p:complete("angkor wa", 5)
  assert(out[1] == "Angkor Wat")
  assert(start == 0, "the run begins at the tail's start")
  assert(stop == 9, "the run reaches the end of the line, past the caret")
end)

test("a match matcher reads a token its own way", function()
  local p = trx.argparse.new()
  p:positional("slot", {
    match = function(token)
      local n = token:match("^q(%d+)$")
      if n ~= nil then
        return { quick = true, index = tonumber(n) }, true
      end
      if token:match("^%d+$") then
        return { quick = false, index = tonumber(token) }, true
      end
      return nil, false
    end,
  })
  assert(p:parse("3").slot.index == 3)
  assert(p:parse("q2").slot.quick == true)
  assert(p:parse("q2").slot.index == 2)
  assert(p:parse("nonsense") == nil)
end)

test("suggest completes but does not restrict or hint", function()
  local p = trx.argparse.new()
  p:positional("option", { suggest = { "fov", "gamma" } })
  -- Any token is taken; suggest never rejects.
  assert(p:parse("anything").option == "anything")
  -- It offers completions...
  assert(p:complete("f")[1] == "fov")
  -- ...but stays out of the error: a missing option carries no "expected" hint.
  local _, err = p:parse("")
  assert(err.kind == "missing" and err.hint == nil)
end)

test("a flag floats anywhere", function()
  local p = trx.argparse.new()
  p:flag("force", { short = "-f", long = "--force" })
  p:positional("slot", { type = "integer", optional = true })
  assert(p:parse("-f 1").force == true)
  assert(p:parse("-f 1").slot == 1)
  assert(p:parse("1 --force").force == true, "it may trail")
  assert(p:parse("3").force == false, "absent means false")
  assert(p:parse("3").slot == 3)
end)

test("rest is the remainder, verbatim; optional is nil when absent", function()
  local p = trx.argparse.new()
  p:positional("option", { choices = { "name" } })
  p:rest("value", { optional = true })
  local parsed = p:parse("name a   b  c")
  assert(parsed.option == "name")
  assert(parsed.value == "a   b  c", "the spacing is kept")
  assert(p:parse("name").value == nil, "absent optional rest is nil")
end)

test("a required rest is missing when absent", function()
  local p = trx.argparse.new()
  p:rest("code")
  local parsed, err = p:parse("")
  assert(parsed == nil and err.kind == "missing" and err.metavar == "code")
end)

test("a leading -h reaches help, a later one is literal", function()
  local p = trx.argparse.new()
  p:rest("code")
  assert(p:parse("-h").help == true)
  assert(p:parse("x = -h").code == "x = -h", "a later -h is part of the rest")
end)

test("help wins over a token that would otherwise fail", function()
  local p = trx.argparse.new()
  -- No positionals, so a stray token is 'unexpected'; -h answers regardless.
  assert(p:parse("-h stray").help == true)
end)

test("help is a reserved flag name", function()
  local p = trx.argparse.new()
  raises(function()
    p:flag("help", { long = "--help" })
  end, "reserved")
end)

test(
  "a positional may be named help without hijacking the help path",
  function()
    local p = trx.argparse.new()
    p:positional("help", { type = "integer", optional = true })
    assert(p:parse("5").help == 5, "its value reaches run")
    assert(p:parse("-h").help == true, "the flag still shows help")
  end
)

test("an unexpected argument is refused with its token", function()
  local p = trx.argparse.new()
  local parsed, err = p:parse("stray")
  assert(parsed == nil and err.kind == "unexpected" and err.token == "stray")
end)

test("complete offers a choice's keys, filtered", function()
  local p = trx.argparse.new()
  p:positional("state", { choices = { "snow", "rain", "none" } })
  assert(#p:complete("") == 3, "an empty token offers them all")
  assert(p:complete("sn")[1] == "snow", "a prefix narrows to the match")
end)

test("complete reports where the run it replaces begins", function()
  local p = trx.argparse.new()
  p:positional("first", { choices = { "a", "b" } })
  p:positional("state", { choices = { "snow", "rain" } })

  local _, empty_start, empty_end = p:complete("a ")
  assert(
    empty_start == 2 and empty_end == 2,
    "past a space, the run is empty and sits at the caret"
  )

  local out, at_word, word_end = p:complete("a sn")
  assert(out[1] == "snow")
  assert(at_word == 2, "the run begins where the active token starts")
  assert(word_end == 4, "and ends where it ends")
end)

test("complete finds the token the caret sits in, not the last", function()
  local p = trx.argparse.new()
  p:positional("first", { choices = { "snow", "sun" } })
  p:positional("second", { choices = { "rain" } })
  -- "sn rain" with the caret after "sn": complete the first token, not "rain".
  local out, start, stop = p:complete("sn rain", 2)
  assert(out[1] == "snow", "the caret's token is completed")
  assert(start == 0 and stop == 2, "and its run is the token, not the tail")
end)

test("complete follows a runtime choice list", function()
  local levels = { "Caves", "City" }
  local p = trx.argparse.new()
  p:positional("level", {
    choices = function()
      local out = {}
      for _, name in ipairs(levels) do
        out[#out + 1] = name
      end
      return out
    end,
  })
  assert(p:complete("cit")[1] == "City")
  levels[#levels + 1] = "Cistern"
  assert(#p:complete("ci") == 2, "the list is read afresh each time")
end)

test("a boolean offers on and off without being told to", function()
  local p = trx.argparse.new()
  p:positional("state", { type = "boolean", optional = true })
  local seen = {}
  for _, key in ipairs(p:complete("")) do
    seen[key] = true
  end
  assert(seen.on and seen.off)
end)

test("a flag completes from either spelling alone", function()
  local p = trx.argparse.new()
  p:flag("force", { long = "--force" })
  p:flag("quiet", { short = "-q" })
  assert(p:complete("--f")[1] == "--force", "a long-only flag is offered")
  assert(p:complete("-q")[1] == "-q", "a short-only flag is offered")
end)

test("complete offers each value once", function()
  local p = trx.argparse.new()
  p:positional("state", {
    choices = { "snow", "rain" },
    suggest = { "snow", "sleet" },
  })
  local seen, dupes = {}, 0
  for _, key in ipairs(p:complete("")) do
    if seen[key] then
      dupes = dupes + 1
    end
    seen[key] = true
  end
  assert(dupes == 0, "a value in both choices and suggest is not repeated")
end)

test("usage joins alternatives and lines them out", function()
  local saved = trx.locale
  trx.locale = {
    get = function(key)
      return key
    end,
    format = function(key, ...)
      local a = { ... }
      return key .. (#a > 0 and (" " .. table.concat(a, " ")) or "")
    end,
  }
  local p = trx.argparse.new({ prog = "music" })
  p:any_of("what", {
    { choices = { "status" }, metavar = "status", help = "h_status" },
    { type = "integer", metavar = "id", help = "h_id" },
  }, { optional = true })
  local u = p:usage()
  trx.locale = saved
  assert(u:find("[status|id]", 1, true), "synopsis joins the metavars: " .. u)
  assert(u:find("status: h_status", 1, true), u)
  assert(u:find("id (", 1, true) and u:find("h_id", 1, true), u)
end)

test("format_error turns a structured error into text", function()
  -- The console layer localizes; a plain stub stands in for trx.locale here.
  local saved = trx.locale
  trx.locale = {
    get = function(key)
      return key
    end,
    format = function(key, ...)
      return key .. ":" .. table.concat({ ... }, ",")
    end,
  }
  local p = trx.argparse.new()
  p:positional("state", { choices = { "snow", "rain" } })
  local _, err = p:parse("hail")
  local msg = p:format_error(err)
  trx.locale = saved
  assert(msg:find("state") and msg:find("hail") and msg:find("snow"), msg)
end)

-- A command whose first argument is optional - a verb before a value, a count
-- before a name - only works if a token that does not fit it can move on.
test("a token passes over an optional positional it does not fit", function()
  local p = trx.argparse.new()
  p:positional("action", { choices = { "give", "take" }, optional = true })
  p:positional("num", { type = "integer", optional = true })

  local parsed = p:parse("3")
  assert(parsed ~= nil, "3 is not a verb, but it is a number")
  assert(parsed.action == nil, "the verb was left out")
  assert(parsed.num == 3)

  parsed = p:parse("take 3")
  assert(
    parsed.action == "take" and parsed.num == 3,
    "both still fill in turn"
  )
end)

test("a required positional does not pass a token on", function()
  local p = trx.argparse.new()
  p:positional("action", { choices = { "give", "take" } })
  p:positional("num", { type = "integer", optional = true })

  local parsed, err = p:parse("3")
  assert(parsed == nil, "the verb has to be there")
  assert(err.kind == "invalid" and err.metavar == "action")
end)

-- The error names the argument the player was filling, not the last one that
-- happened to refuse the token.
test("a token nothing takes is refused by the first that could", function()
  local p = trx.argparse.new()
  p:positional("action", { choices = { "give", "take" }, optional = true })
  p:positional("num", { type = "integer", optional = true })

  local parsed, err = p:parse("hail")
  assert(parsed == nil)
  assert(
    err.kind == "invalid" and err.metavar == "action" and err.token == "hail"
  )
end)

test("passing over one argument does not shift the ones behind it", function()
  local p = trx.argparse.new()
  p:positional("count", { type = "integer", optional = true })
  p:positional("name", { choices = { "uzi", "shotgun" } })

  local parsed = p:parse("uzi")
  assert(parsed.count == nil and parsed.name == "uzi")

  parsed = p:parse("2 uzi")
  assert(parsed.count == 2 and parsed.name == "uzi")
end)

test(
  "completion offers what the slot takes, passed-over arguments included",
  function()
    local p = trx.argparse.new()
    p:positional("action", { choices = { "give", "take" }, optional = true })
    p:positional("num", { choices = { "1", "3" }, optional = true })

    local out = p:complete("")
    assert(#out == 4, "both arguments are reachable from the first slot")

    -- Once the verb is in, only what follows it is offered.
    out = p:complete("give ")
    assert(#out == 2 and out[1] == "1" and out[2] == "3")
  end
)

test("a choices function sees the arguments a token passed over", function()
  local p = trx.argparse.new()
  p:positional("action", { choices = { "give", "take" }, optional = true })
  p:positional("num", {
    choices = function(parsed)
      return parsed.action == "take" and { "1" } or { "2" }
    end,
    optional = true,
  })

  assert(p:parse("take 1").num == "1")
  assert(p:parse("2").num == "2", "no verb means the other set")
  assert(p:parse("take 2") == nil, "2 is not one of take's")
end)

-- /give is a count and then a name: the name is the tail, and the count in
-- front of it is optional, so an empty line has to offer what the tail takes.
test(
  "completion reaches a greedy argument a token would pass over to",
  function()
    local p = trx.argparse.new()
    p:positional("count", { type = "integer", optional = true })
    p:rest("what", { suggest = { "uzi", "shotgun" } })

    local out, rstart, rend = p:complete("")
    assert(
      table.concat(out, ",") == "shotgun,uzi",
      "the tail is what an empty line takes"
    )
    assert(rstart == 0 and rend == 0)

    -- With the count in, the tail owns the rest of the line as it did before.
    out = p:complete("5 sho")
    assert(#out == 1 and out[1] == "shotgun")
  end
)

-- The run a suggestion replaces begins where the tail does, which is at the
-- token that landed on it and not at the one the caret sits in: a passed-over
-- argument leaves the two at different places.
test("a greedy run reaches back over the words already typed", function()
  local p = trx.argparse.new()
  p:positional("count", { type = "integer", optional = true })
  p:rest("what", { suggest = { "big medipack" } })

  local out, rstart, rend = p:complete("5 big med")
  assert(#out == 1 and rstart == 2 and rend == 9)

  out, rstart, rend = p:complete("big med")
  assert(#out == 1 and out[1] == "big medipack")
  assert(rstart == 0 and rend == 7, "the whole tail, not the last word alone")
end)

test("completion sorts the keys the typed text opens to the front", function()
  local p = trx.argparse.new()
  p:positional("option", {
    suggest = { "zzz.ui.zzz", "ui.cc", "aaa.ui.zzz", "ui.aa", "ui.bb" },
  })
  assert(
    table.concat(p:complete("ui."), ",")
      == "ui.aa,ui.bb,ui.cc,aaa.ui.zzz,zzz.ui.zzz"
  )
end)

test("completion sorts a list nothing is typed into", function()
  local p = trx.argparse.new()
  p:positional("weather", { choices = { "snow", "rain", "fog" } })
  assert(table.concat(p:complete(""), ",") == "fog,rain,snow")
end)

test("completion sorts without regard to case", function()
  local p = trx.argparse.new()
  p:positional("level", { suggest = { "bacon", "Angkor", "apple" } })
  assert(table.concat(p:complete(""), ",") == "Angkor,apple,bacon")
end)

return h.report()
