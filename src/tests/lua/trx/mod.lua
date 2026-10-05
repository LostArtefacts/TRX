local h = require("harness")
local test = h.test

test("the mod types are the reflected enum", function()
  assert(trx.mod.Type.BASE_GAME == 0)
  assert(trx.mod.Type.EXPANSION_PACK == 1)
  assert(trx.mod.Type.MISC == 2)
  assert(trx.mod.Type.DIRECT_LEVEL == 3)
  assert(trx.mod.Type.CUSTOM == 4)
end)

test("list gives the mods in order, counted from one", function()
  local mods = trx.mod.list
  assert(#mods == 2)

  assert(mods[1].name == "base")
  assert(mods[1].title == "Base Game")
  assert(mods[1].type == trx.mod.Type.BASE_GAME)
  assert(mods[1].engine_version == 4)
  assert(mods[1].base_mod == nil)
  assert(mods[1].is_available)
  assert(mods[1].is_valid)

  assert(mods[2].name == "extra")
  assert(mods[2].type == trx.mod.Type.CUSTOM)
  assert(mods[2].base_mod == "base")
  assert(not mods[2].is_valid)
end)

test("current is the loaded mod", function()
  assert(trx.mod.current.name == "base")
end)

test("switch takes a valid mod, by name or by handle", function()
  assert(trx.mod.switch("base") == true)
  assert(trx.mod.switch(trx.mod.list[1]) == true)
end)

test("switch refuses an invalid or unknown mod", function()
  assert(trx.mod.switch("extra") == false)
  assert(trx.mod.switch("nope") == false)
end)

return h.report()
