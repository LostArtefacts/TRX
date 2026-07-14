-- The surface as a level script meets it: sealed, and with the globals hardened.
-- No other suite gets that far.

local h = harness
local test, raises = h.test, h.raises

test("the raw C bridge is gone", function()
  assert(trxc == nil, "sealing takes trxc off the globals")
  assert(trx.items.spawn ~= nil, "the public surface is still there")
end)

test("the loader and the globals it feeds are gone", function()
  for _, name in ipairs({ "load", "loadfile", "dofile", "require", "package" }) do
    assert(_G[name] == nil, name .. " must not survive hardening")
  end
  assert(string.dump == nil, "string.dump feeds the loader")
end)

-- Strict mode compiles its wrappers with load(), which api.lua captures at module
-- scope because hardening nils the global. Every other strict test runs
-- unhardened and would not notice if that capture went.
test("strict mode still works once the globals are hardened", function()
  trx.api.strict(true)

  raises(function()
    trx.items.spawn("wolf", { x = 0, y = 0, z = 0 })
  end)
  assert(trx.items.get(1) ~= nil, "a good call still goes through")

  trx.api.strict(false)
  assert(trx.items.get(1) ~= nil)
end)

test("the surface cannot be reopened", function()
  raises(function()
    trx.api.define("items.evil", { impl = function() end })
  end, "sealed")
end)

return h.report()
