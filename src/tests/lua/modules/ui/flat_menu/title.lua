-- The title menu of the flat menu system: what it lists, and what each of its
-- choices starts.

local accent = require("common.ui.accent")
local h = require("harness")
local test = h.test

require("common.ui.menu").setup()

local Role = trx.input.Role
local MENU = trx.ui.Screen.MENU
local MenuMode = trx.ui.MenuMode
local CHOICE_CONFIRM = 2
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

local function open()
  trx.config.set("ui.text_scale", 1)
  trx.config.set("ui.menu_system", "flat")
  trx.config.set("gameplay.game_modes_policy", "never")
  trx.config.set("flow.play_any_level", false)
  assert(fake.offer(MENU, { mode = MenuMode.TITLE }), "no menu opened")
  ticks(SETTLE_TICKS)
end

local function finish()
  fake.release(MENU)
  fake.take_choice(MENU)
  fake.release_all()
  trx.config.set("ui.menu_system", "ring")
  assert(trx.ui.layers.count() == 0, "a layer outlived the menu")
  assert(fake.errors() == 0, "the menu raised")
end

test("the flat title menu lists what the original lists", function()
  open()
  assert(shows(L("general/passport/new_game")), "no new game")
  assert(shows(L("general/flat_menu/options")), "no options")
  assert(shows(L("general/flat_menu/exit")), "no exit")
  local heading = require("common.ui.ring").signals.heading:get()
  assert(heading == nil, "the ring opened instead")
  finish()
end)

test("the title menu stands in the middle of the free room", function()
  open()
  local top, bottom = math.huge, -math.huge
  for _, op in ipairs(fake.scene()) do
    local y = op:match("^text .- y=(%-?%d+)")
    if y ~= nil then
      top = math.min(top, tonumber(y))
      bottom = math.max(bottom, tonumber(y))
    end
  end
  local _, height = trx.ui.primitive.measure_text("Ag")
  local area = trx.ui.safe_area
  local middle = (top + bottom + height) / 2
  assert(
    math.abs(middle - (area.y + area.height / 2)) <= height,
    ("the menu stands at %d to %d"):format(top, bottom)
  )
  finish()
end)

test("new game starts a new game", function()
  open()
  press(Role.MENU_CONFIRM)
  -- The saves offer more ways to start than a new game, so the choices list.
  if shows(L("general/passport/select_mode")) then
    press(Role.MENU_CONFIRM)
  end
  ticks(SETTLE_TICKS)
  assert(fake.calls().new_game.count == 1, "no new game started")
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the menu stayed open")
  finish()
end)

test("load game lists the empty slots with no saves", function()
  fake.set_slot_count(trx.savegame.Pool.NORMAL, 1)
  fake.set_slot_free(1, trx.savegame.Pool.NORMAL)
  open()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(shows(L("general/flat_menu/empty_slot")), "the list did not open")
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  finish()
end)

-- Returns the opacity of the black quad a fade draws over the whole screen,
-- from 0 to 255, or nil where no fade draws.
local function black()
  for _, op in ipairs(fake.scene()) do
    local alpha = op:match("^quad x=0 y=0 .* color=000000(%x%x)$")
    if alpha ~= nil then
      return tonumber(alpha, 16)
    end
  end
  return nil
end

test("the title fades in with the inventory fade effects off", function()
  trx.config.set("ui.inventory_fade_effects", false)
  trx.config.set("ui.text_scale", 1)
  trx.config.set("ui.menu_system", "flat")
  assert(fake.offer(MENU, { mode = MenuMode.TITLE }), "no menu opened")
  local partial = false
  for _ = 1, SETTLE_TICKS do
    fake.tick()
    local value = black()
    partial = partial or (value ~= nil and value > 0x10 and value < 0xF0)
  end
  trx.config.set("ui.inventory_fade_effects", true)
  finish()
  assert(partial, "the title cut in rather than fading in")
end)

test("a new game fades the title to black", function()
  trx.config.set("ui.inventory_fade_effects", false)
  open()
  local darkest = 0
  press(Role.MENU_CONFIRM)
  if shows(L("general/passport/select_mode")) then
    press(Role.MENU_CONFIRM)
  end
  for _ = 1, SETTLE_TICKS do
    darkest = math.max(darkest, black() or 0)
    fake.tick()
  end
  trx.config.set("ui.inventory_fade_effects", true)
  finish()
  assert(darkest >= 0xC0, "the title did not fade to black")
end)

test("loading a save leaves the title menu hidden as it fades", function()
  open()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  press(Role.MENU_CONFIRM)
  local loads = fake.calls().load.count
  local shown = false
  for _ = 1, SETTLE_TICKS do
    shown = shown or shows(L("general/passport/new_game"))
    fake.tick()
  end
  finish()
  assert(loads == 1, "nothing was loaded")
  assert(not shown, "the title menu showed again as it faded")
end)

test("back on the title menu does nothing", function()
  open()
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  assert(shows(L("general/passport/new_game")), "the menu went away")
  finish()
end)

test("the level select lists the levels and starts the one picked", function()
  open()
  trx.config.set("flow.play_any_level", true)
  press(Role.MENU_CONFIRM)
  assert(shows(L("general/passport/select_level")), "no level select")
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  -- A level picked from the level select starts as the engine's select command.
  local started = fake.calls().play_gym
  assert(started.count == 1, "no level started")
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the menu stayed open")
  finish()
end)

test("options open the settings and come back to the title", function()
  open()
  for _ = 1, 4 do
    if
      shows(accent.text(L("general/flat_menu/options"), accent.Role.PICKED))
    then
      break
    end
    press(Role.MENU_DOWN)
  end
  press(Role.MENU_CONFIRM)
  local heading = L("general/settings/graphic_settings/title")
  assert(shows(heading), "the options list no settings")
  press(Role.MENU_BACK)
  assert(shows(L("general/passport/new_game")), "the title did not return")
  finish()
end)

test("the title shows the logo on its main menu alone", function()
  local shows_main = require("common.ui.title_page").shows_main
  fake.set_slot_count(trx.savegame.Pool.NORMAL, 1)
  fake.set_slot_free(1, trx.savegame.Pool.NORMAL)
  open()
  assert(shows_main:get(), "the main menu hid the logo")
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(not shows_main:get(), "the list of saves kept the logo")
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  assert(shows_main:get(), "the logo did not come back")
  for _ = 1, 4 do
    if
      shows(accent.text(L("general/flat_menu/options"), accent.Role.PICKED))
    then
      break
    end
    press(Role.MENU_DOWN)
  end
  press(Role.MENU_CONFIRM)
  assert(not shows_main:get(), "the options kept the logo")
  press(Role.MENU_BACK)
  assert(shows_main:get(), "the logo did not come back")
  finish()
  assert(shows_main:get(), "the closed menu hid the logo")
end)

return h.report()
