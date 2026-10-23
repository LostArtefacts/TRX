-- The pause screen of the flat menu system: what it lists, and where each
-- row of it leads.

local accent = require("common.ui.accent")
local h = require("harness")
local test = h.test

require("common.ui.menu").setup()

local Role = trx.input.Role

-- Long enough for any fade of the screen to end.
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

local PAUSE = trx.ui.Screen.PAUSE
local CHOICE_RESUME = 3
local CHOICE_EXIT_TO_TITLE = 4
local CAVES = 2
local L = trx.locale.get

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

-- The deepest black that filled the screen over some ticks.
local function darkest(count)
  local result = 0
  for _ = 1, count do
    fake.tick()
    for _, op in ipairs(fake.scene()) do
      local alpha = op:match("^quad x=0 y=0 z=0 .* color=000000(%x%x)$")
      if alpha ~= nil then
        result = math.max(result, tonumber(alpha, 16))
      end
    end
  end
  return result
end

local function pause()
  trx.config.set("ui.text_scale", 1)
  trx.config.set("ui.menu_system", "flat")
  fake.set_current_level(CAVES)
  assert(
    fake.offer(PAUSE, { can_exit_to_title = true }),
    "the pause screen was not taken"
  )
  ticks(SETTLE_TICKS)
end

local function finish()
  fake.release(PAUSE)
  fake.take_choice(PAUSE)
  fake.release_all()
  fake.set_current_level(nil)
  trx.config.set("ui.menu_system", "ring")
  assert(trx.ui.layers.count() == 0, "a layer outlived the screen")
  assert(fake.errors() == 0, "the screen raised")
end

test("the flat pause screen lists what the original lists", function()
  pause()
  assert(shows(L("general/pause/paused")), "no heading")
  assert(shows(L("general/flat_menu/statistics")), "no statistics")
  assert(shows(L("general/flat_menu/options")), "no options")
  assert(shows(L("general/flat_menu/exit_to_title")), "no way out")
  finish()
end)

test("an FMV's pause screen lists only the options", function()
  trx.config.set("ui.menu_system", "flat")
  fake.set_current_level(nil)
  assert(fake.offer(PAUSE), "the pause screen was not taken")
  ticks(SETTLE_TICKS)
  assert(shows(L("general/flat_menu/options")), "no options")
  assert(not shows(L("general/flat_menu/statistics")), "it lists statistics")
  assert(not shows(L("general/flat_menu/exit_to_title")), "it lists a way out")
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  assert(shows(L("general/settings/graphic_settings/title")), "no settings")
  press(Role.MENU_BACK)
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(PAUSE) == CHOICE_RESUME, "the screen stayed")
  finish()
end)

test("back returns to the game from the flat pause screen", function()
  pause()
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(PAUSE) == CHOICE_RESUME, "the screen stayed")
  finish()
end)

test("exit to title leaves the flat pause screen for the title", function()
  pause()
  press(Role.MENU_DOWN)
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(PAUSE) == CHOICE_EXIT_TO_TITLE, "it did not leave")
  finish()
end)

test("exit to title fades to black over a pattern backdrop", function()
  trx.config.set("ui.pause_fade_effects", true)
  trx.config.set("ui.pause_background_style", "pattern_wave")
  pause()
  press(Role.MENU_DOWN)
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  assert(darkest(SETTLE_TICKS) == 0xFF, "the screen did not go black")
  assert(fake.take_choice(PAUSE) == CHOICE_EXIT_TO_TITLE, "it did not leave")
  trx.config.set("ui.pause_background_style", "transparent")
  finish()
end)

test("the options of the flat pause screen open the settings", function()
  pause()
  local before = trx.ui.layers.count()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  local heading = L("general/settings/graphic_settings/title")
  assert(shows(heading), "the options list no settings")
  press(Role.MENU_CONFIRM)
  assert(trx.ui.layers.count() > before, "no dialog opened")
  assert(not shows(L("general/flat_menu/options")), "the list still shows")
  press(Role.MENU_BACK)
  assert(trx.ui.layers.count() == before, "the dialog stayed")
  assert(shows(heading), "the options did not come back")
  press(Role.MENU_BACK)
  assert(shows(L("general/flat_menu/statistics")), "the list did not return")
  finish()
end)

test("a setting's description hides the dialog under it", function()
  pause()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  press(Role.MENU_CONFIRM)
  local title = L("general/settings/graphic_settings/title")
  assert(shows(title), "no dialog opened")
  press(Role.MENU_DOWN)
  press(Role.MENU_SHOW_INFO)
  assert(not shows(title), "the dialog still shows")
  press(Role.MENU_BACK)
  assert(shows(title), "the dialog did not come back")
  press(Role.MENU_BACK)
  press(Role.MENU_BACK)
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  finish()
end)

test("the statistics of the flat pause screen show alone", function()
  pause()
  local before = trx.ui.layers.count()
  press(Role.MENU_CONFIRM)
  assert(trx.ui.layers.count() > before, "no statistics opened")
  assert(not shows(L("general/flat_menu/options")), "the list still shows")
  assert(
    shows(
      accent.text(L("general/flat_menu/statistics"), accent.Role.MENU_HEADING)
    ),
    "the heading did not show"
  )
  assert(
    shows(accent.text(L("general/stats/time_taken"), accent.Role.MENU_ROW)),
    "the counters did not show"
  )
  press(Role.MENU_BACK)
  assert(trx.ui.layers.count() == before, "the statistics stayed")
  assert(shows(L("general/flat_menu/options")), "the list did not return")
  finish()
end)

return h.report()
