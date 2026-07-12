-- api.type() reaches into the C struct binder: left callable, a script could
-- re-expose the members the declarations withheld.

local h = require("harness")
local test, raises = h.test, h.raises

test("an undeclared member is unreachable before the seal", function()
  assert(trx.items[1].box_num == nil, "box_num was never declared")
end)

test("sealing rejects further declarations", function()
  trx.api.seal()

  raises(function()
    trx.api.define("items.evil", { impl = function() end })
  end, "sealed")

  raises(function()
    trx.api.type("items.Item", {
      backing = "ITEM",
      fields = { box_num = { from = "box_num", type = "integer" } },
    })
  end, "sealed")

  -- A rejected declaration must not have bound anything on its way out.
  assert(trx.items[1].box_num == nil, "box_num must still be unreachable")
end)

return h.report()
