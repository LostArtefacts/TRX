-- The shipped color editor, which a color setting opens.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local color_editor = require("common.ui.settings.color_editor")

local KEY = "visuals.water_color"

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
  fake.tick()
end

local function open()
  trx.config.set("ui.text_scale", 1.0)
  local layer = color_editor.open(trx.ui.layers.push, KEY)
  fake.tick()
  return layer
end

local function hex()
  local c = trx.config.get(KEY)
  return ("%02X%02X%02X"):format(c.r, c.g, c.b)
end

test("a press writes the new color to the setting", function()
  trx.config.set(KEY, "336699")
  local layer = open()
  press(Role.MENU_RIGHT)
  assert(hex() ~= "336699", hex())
  layer:close()
end)

test("each slider changes the color", function()
  trx.config.set(KEY, "336699")
  local layer = open()
  local seen = { [hex()] = true }
  for _ = 1, 3 do
    press(Role.MENU_LEFT)
    assert(not seen[hex()], hex())
    seen[hex()] = true
    press(Role.MENU_DOWN)
  end
  layer:close()
end)

test("fine adjustment moves a slider a smaller step", function()
  trx.config.set(KEY, "336699")
  local layer = open()
  local l0 = trx.math.to_oklch(trx.config.get(KEY))
  press(Role.MENU_DOWN)
  press(Role.MENU_DOWN)
  press(Role.MENU_RIGHT)
  local l1 = trx.math.to_oklch(trx.config.get(KEY))
  fake.release_all()
  fake.hold(Role.MENU_FINE_ADJUST, true)
  fake.press(Role.MENU_RIGHT)
  fake.tick()
  fake.release_all()
  fake.tick()
  local l2 = trx.math.to_oklch(trx.config.get(KEY))
  assert(math.abs((l1 - l0) - 0.05) < 0.01, l1 - l0)
  assert(math.abs((l2 - l1) - 0.01) < 0.01, l2 - l1)
  layer:close()
end)

test("unbind puts the default color back", function()
  trx.config.reset(KEY)
  local default = hex()
  trx.config.set(KEY, "336699")
  local layer = open()
  press(Role.UNBIND_KEY)
  assert(hex() == default)
  layer:close()
end)

test("back closes the editor", function()
  local layer = open()
  press(Role.MENU_BACK)
  assert(not layer.is_open)
end)

return h.report()
