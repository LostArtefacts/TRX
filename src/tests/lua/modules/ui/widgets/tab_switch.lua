-- The shipped tab bar, which switches the tabs of the game's dialogs.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local tab_switch = require("common.ui.widgets.tab_switch")

local function with_wraparound(enabled, fn)
  trx.config.set("ui.enable_wraparound", enabled)
  local ok, err = pcall(fn)
  trx.config.set("ui.enable_wraparound", true)
  assert(ok, err)
end

-- Puts the bar on a layer that hands it the keys, with or without the arrow
-- keys.
local function on_layer(tabs, arrows)
  local layer = trx.ui.layers.push({
    root = tabs.widget,
    on_input = function(_, keys)
      tabs:control(keys, arrows)
    end,
  })
  -- A layer reads no input on the tick it opens.
  fake.tick()
  return layer
end

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
  fake.tick()
end

test("the tab keys switch tabs", function()
  local tabs = tab_switch.new({ titles = { "A", "B", "C" } })
  local layer = on_layer(tabs)
  press(Role.MENU_TAB_RIGHT)
  assert(tabs.active:get() == 2)
  press(Role.MENU_RIGHT)
  assert(tabs.active:get() == 3)
  press(Role.STEP_LEFT)
  assert(tabs.active:get() == 2)
  layer:close()
end)

test("the bar leaves left and right alone where it is told to", function()
  local tabs = tab_switch.new({ titles = { "A", "B" } })
  local layer = on_layer(tabs, false)
  press(Role.MENU_RIGHT)
  assert(tabs.active:get() == 1)
  press(Role.MENU_TAB_RIGHT)
  assert(tabs.active:get() == 2)
  layer:close()
end)

test("the bar wraps where the setting says so", function()
  local tabs = tab_switch.new({ titles = { "A", "B" } })
  assert(tabs:cycle(-1))
  assert(tabs.active:get() == 2)
  with_wraparound(false, function()
    assert(not tabs:cycle(1))
    assert(tabs.active:get() == 2)
  end)
end)

test("a single tab bar is as wide as its active tab", function()
  local tabs = tab_switch.new({ titles = { "Short", "A much longer name" } })
  local full = tabs.widget:measure()
  tabs.single:set(true)
  local single = tabs.widget:measure()
  assert(single < full)
  tabs.active:set(2)
  assert(tabs.widget:measure() > single)
end)

test("an idle bar keeps its arrows hidden but their room", function()
  local tabs = tab_switch.new({ titles = { "A", "B" } })
  local idle = tabs.widget:measure()
  tabs.mode:set(tab_switch.Mode.FOCUSED)
  assert(tabs.widget:measure() == idle)
end)

return h.report()
