-- The shipped settings editor: the rows of one tab of a settings dialog.

local h = require("harness")
local test = h.test

h.load_themes()

local Role = trx.input.Role
local editor = require("common.ui.settings.editor")

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
  fake.tick()
end

local function make(keys)
  trx.config.set("ui.text_scale", 1.0)
  local pushed = {}
  local tab = editor.new({
    title = "Tab",
    keys = keys,
    editing = trx.signal.new(true),
    width = trx.signal.new(300),
    dialog_width = trx.signal.new(320),
    changed = trx.config.signals.changed,
    layout = trx.ui.signals.layout,
    push = function(settings)
      local layer = trx.ui.layers.push(settings)
      pushed[#pushed + 1] = layer
      return layer
    end,
  })
  tab.scroll:set_visible(10)
  return tab, pushed
end

local function on_layer(tab)
  local layer = trx.ui.layers.push({
    root = editor.rows({
      tabs = { tab },
      active = trx.signal.new(1),
      editing = trx.signal.new(true),
      width = trx.signal.new(300),
      changed = trx.config.signals.changed,
    }),
    on_input = function(_, keys)
      tab:control(keys)
    end,
  })
  fake.tick()
  return layer
end

test("a setting the game lacks has no row", function()
  local tab = make({ "visuals.fov", "visuals.no_such_setting" })
  assert(#tab.keys == 1)
  tab:close()
end)

test("a row goes and comes back with the setting that hides it", function()
  trx.config.set("ui.show_loading_bar", true)
  local tab = make({ "ui.show_loading_bar", "ui.progressbar_color" })
  assert(#tab.shown:get() == 2)
  trx.config.set("ui.show_loading_bar", false)
  assert(#tab.shown:get() == 1)
  assert(tab.scroll.count:get() == 1)
  trx.config.set("ui.show_loading_bar", true)
  assert(#tab.shown:get() == 2)
  tab:close()
end)

test("show info opens the description over the rows", function()
  local tab, pushed = make({ "visuals.fov" })
  local layer = on_layer(tab)
  tab.scroll:select(1)
  press(Role.MENU_SHOW_INFO)
  assert(#pushed == 1)
  press(Role.MENU_BACK)
  assert(not pushed[1].is_open)
  layer:close()
  tab:close()
end)

test("unbind restores the setting under the cursor", function()
  local tab = make({ "visuals.fov" })
  local layer = on_layer(tab)
  tab.scroll:select(1)
  trx.config.set("visuals.fov", 70)
  if not trx.config.is_default("visuals.fov") then
    press(Role.UNBIND_KEY)
    assert(trx.config.is_default("visuals.fov"))
  end
  layer:close()
  tab:close()
end)

-- The strings load nothing here, so declaring the text as the dialog shows
-- stands in for the player picking another language.
test("the hints follow the player's language", function()
  trx.config.set("visuals.fov", 80)
  local tab = make({ "visuals.fov" })
  tab.scroll:select(1)
  local layer = trx.ui.layers.push({ root = tab.footer })
  fake.tick()
  local function shows(wanted)
    for _, op in ipairs(fake.scene()) do
      if op:find(wanted, 1, true) then
        return true
      end
    end
    return false
  end
  assert(not shows("Put it back"))
  trx.locale.declare({
    ["general/settings/common/restore_default"] = "Put it back",
  })
  trx.config.set("visuals.fov", 81)
  fake.tick()
  assert(shows("Put it back"), "the hint kept its old text")
  layer:close()
  tab:close()
end)

test("the tab measures its widest row", function()
  local tab = make({ "visuals.fov", "ui.enable_wraparound" })
  local content, least = tab:widths()
  assert(content >= least and least > 0, content .. " " .. least)
  tab:close()
end)

return h.report()
