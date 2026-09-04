-- The JSON encoder as a script actually sees it. The API dump goes through
-- this, so what it writes is what the committed reference is generated from.

local h = require("harness")
local test, raises = h.test, h.raises

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
  assert(encode({ true, false }) == "[true,false]")
  assert(
    encode({ 1, print, 3 }) == "[1,null,3]",
    "one with no JSON form keeps the entries after it where they were"
  )
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

test("a value with no JSON of its own does not break the text", function()
  local encode = trx.json.encode

  assert(encode({ 1, print, 3 }) == "[1,null,3]", "a list keeps its length")
  assert(encode({ a = 1, b = print }) == '{"a":1}', "an object leaves it out")
  assert(encode({ print }) == "[null]")
end)

test("a number with no JSON of its own raises", function()
  local encode = trx.json.encode

  raises(function()
    encode({ 1 / 0 })
  end)
  raises(function()
    encode({ hp = 0 / 0 })
  end)
end)

test("a table that contains itself raises rather than running away", function()
  local held = {}
  held.self = held
  raises(function()
    trx.json.encode(held)
  end)
end)

test("what a script writes it reads back", function()
  local kept = trx.path.config_dir / "mymod" / "state.json"
  trx.json.write_file(kept, { seen = { "vilcabamba" }, hp = 6 })
  local held = trx.json.read_file(kept)
  assert(held.hp == 6)
  assert(held.seen[1] == "vilcabamba")
end)

test("text says where a file is as well as a path does", function()
  trx.json.write_file("/fake/config_dir/plain.json", { hp = 1 })
  assert(trx.json.read_file("/fake/config_dir/plain.json").hp == 1)
end)

test("a file that is not there reads as nothing", function()
  assert(trx.json.read_file(trx.path.config_dir / "nothing.json") == nil)
end)

test("a file outside the places a script reaches is refused", function()
  raises(function()
    trx.json.read_file("/etc/passwd")
  end)
  raises(function()
    trx.json.write_file("/etc/passwd", { hp = 1 })
  end)
  raises(function()
    trx.json.write_file(trx.path.config_dir / "a/../../out.json", {})
  end)
end)

test("a value with no JSON leaves nothing behind", function()
  local lost = trx.path.config_dir / "lost.json"
  raises(function()
    trx.json.write_file(lost, { hp = 0 / 0 })
  end)
  assert(trx.json.read_file(lost) == nil, "nothing was written")
end)

test("JSON text reads back as the value it stands for", function()
  local held = trx.json.decode('{"hp": 6, "seen": ["vilcabamba"], "ok": true}')
  assert(held.hp == 6)
  assert(held.seen[1] == "vilcabamba")
  assert(held.ok == true)
  assert(trx.json.decode("[1, 2, 3]")[2] == 2)
  assert(trx.json.decode('"wolf"') == "wolf")
end)

test("the game's own spelling is taken", function()
  local held = trx.json.decode('{ hp: 6, /* a comment */ seen: ["a",], }')
  assert(held.hp == 6)
  assert(held.seen[1] == "a")
end)

test("text that is not JSON raises, saying where", function()
  raises(function()
    trx.json.decode("{")
  end)
  raises(function()
    trx.json.decode("nonsense")
  end)
end)

test("what encode writes, decode reads", function()
  local held = { hp = 6, seen = { "a", "b" }, ok = false }
  local back = trx.json.decode(trx.json.encode(held))
  assert(back.hp == 6 and back.ok == false and back.seen[2] == "b")
end)

test("a key that is a number comes out as text", function()
  local encode = trx.json.encode

  assert(encode({ [2] = "a" }) == '{"2":"a"}')
  assert(encode({ [true] = "a" }) == "{}", "a key of another kind is left out")
end)

test("text nested past what the reader takes raises", function()
  local deep = string.rep("[", 1000) .. string.rep("]", 1000)
  raises(function()
    trx.json.decode(deep)
  end)
  assert(#trx.json.decode(string.rep("[", 50) .. string.rep("]", 50)) == 1)
end)

test("a table read from a file says where it was written", function()
  local kept = trx.path.config_dir / "where.json"
  trx.json.write_file(kept, { hp = 1 })
  local held = trx.json.read_file(kept)
  local where = trx.json.where(held)
  assert(where ~= nil and where:find("where.json", 1, true), tostring(where))
  assert(trx.json.where({}) == nil, "one a script built carries none")
  assert(trx.json.where(7) == nil)
end)

return h.report()
