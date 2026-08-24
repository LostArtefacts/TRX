-- Drawing on top of the game.
--
-- The module hands the body of a widget to the engine rather than opening and
-- closing it by hand, so what matters here is that the scene comes back whole
-- whatever the body does, and that nothing draws outside a scene at all.

local h = require("harness")
local test, raises = h.test, h.raises

test("a label reaches the scene", function()
  local balanced = fake.draw(function()
    trx.ui.label("iris")
  end)
  assert(balanced, "the scene was left open")
  assert(fake.last_label() == "iris")
end)

test("nothing draws outside a scene", function()
  raises(function()
    trx.ui.label("iris")
  end, "on_ui_draw")
  raises(function()
    trx.ui.stack({}, function() end)
  end, "on_ui_draw")
end)

test("a stack holds what its body draws", function()
  local balanced = fake.draw(function()
    trx.ui.stack({}, function()
      trx.ui.label("one")
    end)
  end)
  assert(balanced, "the scene was left open")
  assert(fake.last_label() == "one", "the body never drew")
end)

test("a stack takes the layout enums", function()
  local balanced = fake.draw(function()
    trx.ui.stack({
      orientation = trx.ui.Orientation.HORIZONTAL,
      align = { h = trx.ui.HAlign.CENTER, v = trx.ui.VAlign.BOTTOM },
      spacing = { h = 4, v = 2 },
    }, function()
      trx.ui.label("one")
    end)
  end)
  assert(balanced, "the scene was left open")
end)

test("a stack refuses a setting it does not take", function()
  fake.draw(function()
    raises(function()
      trx.ui.stack({ orientation = 99 }, function() end)
    end, "orientation")
    raises(function()
      trx.ui.stack({ align = { h = 99 } }, function() end)
    end, "h")
  end)
end)

test("an error in a body still closes the widget", function()
  local balanced, err = fake.draw(function()
    trx.ui.stack({}, function()
      error("the body gave up")
    end)
  end)
  assert(balanced, "an error left the scene open")
  assert(err ~= nil, "the error was swallowed")
end)

test("nested stacks come back to where they started", function()
  local balanced = fake.draw(function()
    trx.ui.stack({}, function()
      trx.ui.stack({ orientation = trx.ui.Orientation.HORIZONTAL }, function()
        trx.ui.label("deep")
      end)
    end)
  end)
  assert(balanced, "the scene was left open")
  assert(fake.last_label() == "deep")
end)

test("the regions are the ones the game builds", function()
  assert(trx.ui.Region.TOP_CENTER ~= trx.ui.Region.BOTTOM_CENTER)
  assert(trx.ui.Region.TOP_LEFT ~= nil)
  assert(trx.ui.Region.BOTTOM_RIGHT ~= nil)
end)

test("a label takes settings", function()
  local balanced = fake.draw(function()
    trx.ui.label("iris", { scale = 2.0 })
  end)
  assert(balanced, "the scene was left open")
  assert(fake.last_label() == "iris")
end)

test("measuring follows the scale it is given", function()
  local w, h = trx.ui.measure("iris")
  local wide, tall = trx.ui.measure("iris", { scale = 2.0 })
  assert(w > 0 and h > 0, "a measurement came back empty")
  assert(wide == w * 2, "the width ignored the scale")
  assert(tall == h * 2, "the height ignored the scale")
end)

test("measuring works outside a scene", function()
  local w = trx.ui.measure("iris")
  assert(w > 0, "measuring needed a scene")
end)

test("a bar reaches the scene", function()
  local balanced = fake.draw(function()
    trx.ui.bar({ type = trx.ui.BarType.PROGRESS, value = 40, max_value = 100 })
  end)
  assert(balanced, "the scene was left open")
  assert(fake.last_bar() == 40)
end)

test("a bar refuses a type it does not take", function()
  fake.draw(function()
    raises(function()
      trx.ui.bar({ type = 99 })
    end, "type")
  end)
end)

test("every widget that holds another comes back balanced", function()
  local cases = {
    function(body)
      trx.ui.anchor(0.5, 0.5, body)
    end,
    function(body)
      trx.ui.pad({ x = 2, y = 2 }, body)
    end,
    function(body)
      trx.ui.hide(true, body)
    end,
    function(body)
      trx.ui.resize({ w = 40, h = 10 }, body)
    end,
    function(body)
      trx.ui.frame(trx.ui.FrameStyle.OUTLINE_ONLY, body)
    end,
    function(body)
      trx.ui.offset(1, 1, body)
    end,
    function(body)
      trx.ui.span(body)
    end,
  }

  for i, open_one in ipairs(cases) do
    local balanced = fake.draw(function()
      open_one(function()
        trx.ui.label("deep")
      end)
    end)
    assert(balanced, "widget " .. i .. " left the scene open")
    assert(
      fake.last_label() == "deep",
      "widget " .. i .. " swallowed its body"
    )
  end
end)

test("an error inside any widget still closes it", function()
  local balanced, err = fake.draw(function()
    trx.ui.pad({ x = 1 }, function()
      trx.ui.span(function()
        error("the body gave up")
      end)
    end)
  end)
  assert(balanced, "an error left the scene open")
  assert(err ~= nil, "the error was swallowed")
end)

test("nothing but measuring draws outside a scene", function()
  raises(function()
    trx.ui.spacer(1, 1)
  end, "on_ui_draw")
  raises(function()
    trx.ui.span(function() end)
  end, "on_ui_draw")
  raises(function()
    trx.ui.bar({})
  end, "on_ui_draw")
end)

test("the canvas answers its own size", function()
  local canvas = trx.ui.canvas
  assert(canvas.width > 0, "the canvas has no width")
  assert(canvas.height > 0, "the canvas has no height")
  assert(canvas.x == 0 and canvas.y == 0, "the canvas does not start at 0, 0")
end)

test("the safe area sits inside the canvas", function()
  local canvas = trx.ui.canvas
  local safe = trx.ui.safe_area
  assert(safe.width <= canvas.width, "the safe area is wider than the canvas")
  assert(safe.x >= 0, "the safe area starts left of the canvas")
  assert(
    safe.x + safe.width <= canvas.width,
    "the safe area runs off the canvas"
  )
end)

return h.report()
