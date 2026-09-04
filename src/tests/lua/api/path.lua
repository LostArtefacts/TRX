-- A place on disk as a script sees it, with nothing installed under it.

local h = require("harness")
local test, raises = h.test, h.raises

test("a root is a path of its own", function()
  assert(tostring(trx.path.config_dir) == "/fake/config_dir")
  assert(tostring(trx.path.saves_dir) == "/fake/saves_dir")
end)

test("joining appends a child segment", function()
  local kept = trx.path.config_dir / "mymod" / "state.json"
  assert(tostring(kept) == "/fake/config_dir/mymod/state.json", tostring(kept))
end)

test("an absolute path replaces the left side when joined", function()
  local absolute = trx.path.config_dir / "/etc/hosts"
  assert(tostring(absolute) == "/etc/hosts")
end)

test("the parts of a path are read off it", function()
  local kept = trx.path.config_dir / "mymod" / "state.json"
  assert(kept.name == "state.json", kept.name)
  assert(kept.stem == "state", kept.stem)
  assert(kept.suffix == ".json", kept.suffix)
  assert(tostring(kept.parent) == "/fake/config_dir/mymod")
  assert((trx.path.config_dir / "README").suffix == "", "a file may have none")
end)

test("two paths with the same text are equal", function()
  assert(trx.path.config_dir / "a" == trx.path.config_dir / "a")
  assert(trx.path.config_dir / "a" ~= trx.path.config_dir / "b")
end)

test("a path joins text as itself", function()
  local kept = trx.path.config_dir / "state.json"
  assert("at " .. kept == "at /fake/config_dir/state.json")
  assert(kept .. " it is" == "/fake/config_dir/state.json it is")
end)

test("text becomes a path, with its tokens expanded", function()
  local kept = trx.path.new("%config_dir%/mymod/state.json")
  assert(kept == trx.path.config_dir / "mymod" / "state.json", tostring(kept))
  assert(tostring(trx.path.new("/etc/hosts")) == "/etc/hosts")
end)

test("a path says nothing about what is there until asked", function()
  assert(not (trx.path.config_dir / "nothing"):exists())
  assert(trx.path.resolve("common_config", "weapons.json5"):exists())
end)

test("resolving searches the way the engine searches", function()
  local found = trx.path.resolve("common_config", "weapons.json5")
  assert(
    tostring(found) == "/fake/games_dir/mod/weapons.json5",
    tostring(found)
  )
  assert(trx.path.resolve("common_config", "nothing.json5") == nil)
end)

test("a kind the engine does not have is refused", function()
  raises(function()
    trx.path.resolve("nowhere", "weapons.json5")
  end)
  raises(function()
    trx.path.new(trx.path.config_dir)
  end)
end)

test("every file kind listed is one that may be resolved", function()
  local kinds = trx.path.kinds()
  assert(#kinds > 0)
  for _, kind in ipairs(kinds) do
    trx.path.resolve(kind, "nothing.json5")
  end
end)

test("a member the type does not declare is not there", function()
  local kept = trx.path.config_dir / "state.json"
  assert(kept.drive == nil)
  raises(function()
    kept.name = "other.json"
  end)
end)

test("a script reads and writes what it keeps", function()
  local kept = trx.path.config_dir / "mymod" / "state.json"
  assert(kept:read_text() == nil, "nothing is there to begin with")
  kept:write_text("hello")
  assert(kept:read_text() == "hello")
  assert(kept:exists())
  kept:write_text("again")
  assert(kept:read_text() == "again", "a file that is there is written over")
end)

test("a script reaches the engine's places and nothing else", function()
  assert((trx.path.config_dir / "mymod" / "state.json"):is_reachable())
  assert(not trx.path.new("/etc/passwd"):is_reachable())
  assert(
    not trx.path
      .new(tostring(trx.path.config_dir) .. "_elsewhere/x")
      :is_reachable(),
    "a place whose name only starts the same is a place of its own"
  )
  assert(
    not (trx.path.config_dir / "a/../../escape"):is_reachable(),
    "a path that walks upwards is refused rather than worked out"
  )
  raises(function()
    trx.path.new("/etc/passwd"):read_text()
  end)
  raises(function()
    trx.path.new("/etc/passwd"):write_text("no")
  end)
  raises(function()
    trx.path.new("/etc/passwd"):exists()
  end)
end)

test("a name that holds dots is a name like any other", function()
  local kept = trx.path.config_dir / "state..bak"
  assert(kept:is_reachable(), "two dots inside a name walk nowhere")
  kept:write_text("hello")
  assert(kept:read_text() == "hello")
  assert(not (trx.path.config_dir / ".." / "escape"):is_reachable())
  assert(not trx.path.new("/fake/../etc/passwd"):is_reachable())
end)

return h.report()
