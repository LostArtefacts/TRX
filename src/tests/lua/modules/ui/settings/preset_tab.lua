-- The shipped presets tab: the list of presets and the question a picked one
-- asks.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local preset_tab = require("common.ui.settings.preset_tab")

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
  fake.tick()
end

local function make(list)
  trx.config.set("ui.text_scale", 1.0)
  local pushed = {}
  local tab = preset_tab.new({
    title = "Presets",
    editing = trx.signal.new(true),
    push = function(settings)
      local layer = trx.ui.layers.push(settings)
      pushed[#pushed + 1] = layer
      return layer
    end,
  })
  tab.presets = list
  tab.scroll:set_count(#list)
  tab:fit(10)
  local layer = trx.ui.layers.push({
    root = tab.widget,
    on_input = function(_, keys)
      tab:control(keys)
    end,
  })
  fake.tick()
  return tab, pushed, layer
end

local WRAP_OFF = {
  title = "Wrap off",
  settings = { { key = "ui.enable_wraparound", value = false } },
}

test("an empty list says so and leaves for the tab bar", function()
  trx.config.set("ui.text_scale", 1.0)
  local tab = preset_tab.new({
    title = "Presets",
    editing = trx.signal.new(true),
    push = trx.ui.layers.push,
  })
  assert(#tab.presets == 0)
  assert(tab:fit(10) == 0)
end)

test("a preset with changes asks before it applies them", function()
  trx.config.set("ui.enable_wraparound", true)
  local tab, pushed, layer = make({ WRAP_OFF })
  tab.scroll:select(1)
  press(Role.MENU_CONFIRM)
  assert(#pushed == 1)
  assert(trx.config.get("ui.enable_wraparound") == true)
  press(Role.MENU_CONFIRM)
  assert(trx.config.get("ui.enable_wraparound") == false)
  assert(not pushed[1].is_open)
  assert(#pushed == 2 and pushed[2].is_open)
  press(Role.MENU_BACK)
  assert(not pushed[2].is_open)
  trx.config.set("ui.enable_wraparound", true)
  layer:close()
end)

test("going back from the question changes nothing", function()
  trx.config.set("ui.enable_wraparound", true)
  local tab, pushed, layer = make({ WRAP_OFF })
  tab.scroll:select(1)
  press(Role.MENU_CONFIRM)
  press(Role.MENU_RIGHT)
  press(Role.MENU_CONFIRM)
  assert(trx.config.get("ui.enable_wraparound") == true)
  assert(not pushed[1].is_open and #pushed == 1)
  layer:close()
end)

test("a preset with nothing to change says so", function()
  trx.config.set("ui.enable_wraparound", false)
  local tab, pushed, layer = make({ WRAP_OFF })
  tab.scroll:select(1)
  press(Role.MENU_CONFIRM)
  assert(#pushed == 1)
  press(Role.MENU_CONFIRM)
  assert(not pushed[1].is_open)
  trx.config.set("ui.enable_wraparound", true)
  layer:close()
end)

return h.report()
