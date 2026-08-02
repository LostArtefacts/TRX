-- api.type() reaches into the C struct binder: left callable, a script could
-- re-expose the members the declarations withheld.

local h = require("harness")
local test, raises = h.test, h.raises

-- The cases share one Lua state and may be shuffled, so the seal has to be
-- something any of them can ask for. It happens once, as it does at boot.
local function seal()
  if trx.api.seal ~= nil then
    trx.api.seal({ partial = true })
  end
end

test("an undeclared member is unreachable before the seal", function()
  assert(trx.items[0].box_num == nil, "box_num was never declared")
end)

-- The declaring half of the registry goes the way trxc goes: a script cannot
-- reach api.type to re-expose a member the declarations withheld, because there
-- is nothing left to reach.
-- Named rather than listed: a list of what goes has to be kept in step with
-- every declarator added, and one left off it is one nobody notices survived.
test("sealing takes the declaring half off trx.api", function()
  seal()

  local kept = { strict = true, is_strict = true }
  for name in pairs(trx.api) do
    assert(kept[name], "trx.api." .. name .. " must not survive the seal")
  end

  assert(trx.items[0].box_num == nil, "box_num must still be unreachable")
end)

test("what is left of trx.api is strict mode", function()
  seal()

  assert(trx.api.is_strict() == false)
  trx.api.strict(true)
  assert(trx.api.is_strict() == true)

  raises(function()
    trx.items.spawn("wolf", { x = 0, y = 0, z = 0 })
  end)

  trx.api.strict(false)
  assert(trx.api.is_strict() == false)
end)

return h.report()
