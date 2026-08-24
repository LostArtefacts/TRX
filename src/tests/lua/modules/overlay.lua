-- The shipped overlay module: that it loads, and that drawing every region
-- reaches no name the API does not have.
--
-- Every field, enum spelling and config key the module names is unchecked
-- until something calls it, so the whole point here is to call it.

local h = require("harness")
local test = h.test

test("the module registers a draw handler", function()
  assert(type(fake.draw_regions) == "function")
end)

test("drawing every region reaches only names the API has", function()
  local description, balanced = fake.draw_regions()
  assert(type(description) == "string")
  assert(balanced, "a widget was left open for the next frame to draw into")
  -- The dispatcher logs a handler's error and carries on, so a region that
  -- raised would otherwise read as a region that drew nothing.
  assert(
    fake.errors() == 0,
    ("%d region(s) raised - see the errors above"):format(fake.errors())
  )
end)

return h.report()
