-- The shipped controls dialog, opened from the controls entry of the
-- inventory ring. Only the keyboard is enabled here, so the editor opens
-- alone.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local RING_ENTRY = trx.ui.Screen.RING_ENTRY
local CONTROLS = trx.catalog.objects.CONTROLS_OPTION
local CHOICE_CANCEL = 1
local CHOICE_CONFIRM = 2

require("common.ui.screens.settings").setup()

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
  fake.tick()
end

local function hold(role)
  fake.release_all()
  fake.press(role)
  for _ = 1, 45 do
    fake.hold(role, true)
    fake.tick()
  end
  fake.release_all()
  fake.tick()
end

local function open(mode)
  trx.config.set("ui.text_scale", 1.0)
  trx.config.set("input.keyboard_layout", trx.input.Layout.DEFAULT)
  fake.open_ring(mode or trx.inventory_ring.Mode.GAME, CONTROLS, 0, 10)
  assert(
    fake.offer(RING_ENTRY, { object = CONTROLS }),
    "the entry was not taken"
  )
  fake.tick()
  fake.tick()
end

local function finish()
  fake.release(RING_ENTRY)
  fake.take_choice(RING_ENTRY)
  fake.close_ring()
  fake.release_all()
  assert(fake.errors() == 0, "the module raised")
end

-- Switches to the first custom layout and puts the cursor on the first row.
local function to_rows()
  press(Role.MENU_RIGHT)
  press(Role.MENU_DOWN)
  press(Role.MENU_DOWN)
end

test("backing out puts the entry away", function()
  open()
  press(Role.MENU_BACK)
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CANCEL)
  local anim = trx.inventory_ring.selection_anim()
  assert(anim.goal_frame == 0 and anim.direction == -1, "not put away")
  finish()
end)

test("confirming on the bars leaves the ring", function()
  open()
  press(Role.MENU_CONFIRM)
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CONFIRM)
  finish()
end)

test("confirming on the title screen puts the entry away", function()
  open(trx.inventory_ring.Mode.TITLE)
  press(Role.MENU_CONFIRM)
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CANCEL)
  finish()
end)

test("the layout bar picks the layout", function()
  open()
  press(Role.MENU_RIGHT)
  assert(trx.config.get("input.keyboard_layout") == trx.input.Layout.CUSTOM_1)
  press(Role.MENU_LEFT)
  assert(trx.config.get("input.keyboard_layout") == trx.input.Layout.DEFAULT)
  finish()
end)

test("the default layout keeps the cursor off the rows", function()
  open()
  press(Role.MENU_DOWN)
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CONFIRM, "not on the bars")
  finish()
end)

test("a key is bound and saved", function()
  open()
  to_rows()
  local before = fake.bindings()
  press(Role.MENU_CONFIRM)
  fake.tick()
  assert(fake.bindings().listening, "not listening")
  fake.land_capture()
  fake.tick()
  local after = fake.bindings()
  assert(after.bound == before.bound + 1, "not bound")
  assert(after.saved == before.saved + 1, "not saved")
  assert(not after.listening, "still listening")
  finish()
end)

test("closing the dialog ends a capture", function()
  open()
  to_rows()
  press(Role.MENU_CONFIRM)
  fake.tick()
  assert(fake.bindings().listening, "not listening")
  finish()
  assert(not fake.bindings().listening, "still listening")
end)

test("holding a key unbinds the binding under the cursor", function()
  open()
  to_rows()
  local before = fake.bindings().unbound
  hold(Role.UNBIND_KEY)
  assert(fake.bindings().unbound == before + 1)
  finish()
end)

test("holding a key resets a custom layout", function()
  open()
  local before = fake.bindings().reset
  hold(Role.RESET_BINDINGS)
  assert(fake.bindings().reset == before, "the default layout was reset")
  press(Role.MENU_RIGHT)
  hold(Role.RESET_BINDINGS)
  assert(fake.bindings().reset == before + 1, "not reset")
  finish()
end)

test("the groups switch from the rows", function()
  open()
  to_rows()
  press(Role.MENU_TAB_RIGHT)
  press(Role.MENU_UP)
  press(Role.MENU_UP)
  press(Role.MENU_LEFT)
  assert(trx.config.get("input.keyboard_layout") == trx.input.Layout.DEFAULT)
  finish()
end)

test("the reset hint shows as the editor opens", function()
  trx.config.set("ui.text_scale", 1.0)
  trx.config.set("input.keyboard_layout", trx.input.Layout.CUSTOM_1)
  fake.open_ring(trx.inventory_ring.Mode.GAME, CONTROLS, 0, 10)
  assert(fake.offer(RING_ENTRY, { object = CONTROLS }))
  fake.tick()
  local shown = false
  for _, op in ipairs(fake.scene()) do
    shown = shown or op:find("reset_defaults", 1, true) ~= nil
  end
  assert(shown, "the reset hint waits for input")
  finish()
end)

return h.report()
