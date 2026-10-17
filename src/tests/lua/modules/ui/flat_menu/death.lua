-- The menu of the flat menu system once Lara has died: what it lists, and
-- where each of its rows leads.

local h = require("harness")
local test = h.test

require("common.ui.menu").setup()

local Role = trx.input.Role
local MENU = trx.ui.Screen.MENU
local MenuMode = trx.ui.MenuMode
local CHOICE_CONFIRM = 2
local CAVES = 2
local L = trx.locale.get

-- Long enough for any fade of the menu to end.
local SETTLE_TICKS = 40

local function ticks(count)
  for _ = 1, count do
    fake.tick()
  end
end

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
  fake.tick()
end

local function texts()
  local result = {}
  for _, op in ipairs(fake.scene()) do
    local text = op:match("^text .- text=(.*)$")
    if text ~= nil then
      result[#result + 1] = text
    end
  end
  return table.concat(result, "\n")
end

local function shows(text)
  return texts():find(text, 1, true) ~= nil
end

-- Lara dies in a level she can restart, with saving turned on or off.
local function die(saving)
  trx.config.set("ui.text_scale", 1)
  trx.config.set("ui.menu_system", "flat")
  trx.config.set("flow.load_save_disabled", not saving)
  fake.set_current_level(CAVES)
  fake.set_restart_available(true)
  assert(fake.offer(MENU, { mode = MenuMode.DEATH }), "no menu opened")
  ticks(SETTLE_TICKS)
end

local function finish()
  fake.release(MENU)
  fake.take_choice(MENU)
  fake.release_all()
  fake.set_current_level(nil)
  trx.config.set("flow.load_save_disabled", false)
  trx.config.set("ui.menu_system", "ring")
  assert(trx.ui.layers.count() == 0, "a layer outlived the menu")
  assert(fake.errors() == 0, "the menu raised")
end

test("the flat death menu lists what the original lists", function()
  die(true)
  assert(shows(L("general/inventory_ring/heading_game_over")), "no heading")
  assert(shows(L("general/passport/load_game")), "no load game")
  assert(shows(L("general/passport/exit_to_title")), "no way out")
  finish()
end)

test("restart level starts the level again", function()
  die(false)
  assert(not shows(L("general/passport/load_game")), "it offers a load")
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.calls().restart_level.count == 1, "the level did not restart")
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the menu stayed open")
  finish()
end)

test("exit to title leaves the menu", function()
  die(false)
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.calls().restart_level.count == 0, "the level restarted")
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the menu stayed open")
  finish()
end)

test("exit to title fades to black with the inventory fades off", function()
  trx.config.set("ui.inventory_fade_effects", false)
  die(false)
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  local darkest = 0
  for _ = 1, SETTLE_TICKS do
    for _, op in ipairs(fake.scene()) do
      local alpha = op:match("^quad x=0 y=0 .* color=000000(%x%x)$")
      if alpha ~= nil then
        darkest = math.max(darkest, tonumber(alpha, 16))
      end
    end
    fake.tick()
  end
  trx.config.set("ui.inventory_fade_effects", true)
  finish()
  assert(darkest >= 0xC0, "the menu did not fade to black")
end)

test("game over alone leaves for the title on its own", function()
  die(false)
  fake.release(MENU)
  fake.take_choice(MENU)
  fake.set_restart_available(false)
  assert(fake.offer(MENU, { mode = MenuMode.DEATH }), "no menu opened")
  ticks(SETTLE_TICKS)
  assert(shows(L("general/inventory_ring/heading_game_over")), "no heading")
  assert(not shows(L("general/passport/exit_to_title")), "it lists rows")
  assert(fake.take_choice(MENU) == 0, "it left at once")
  ticks(300)
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "it did not leave")
  finish()
end)

return h.report()
