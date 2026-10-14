-- The cookbook: each example in docs/trx/lua/examples runs as the level script
-- it is, and the cases check what the player sees, what the player can do, and
-- what is left once the level ends.

local h = require("harness")
local test = h.test

local Role = trx.input.Role

local CHOICE_RESUME = 3
local CHOICE_EXIT_TO_TITLE = 4

local ROOT = package.path:match("([^;]*)/src/tests/lua/%?%.lua")

-- The shipped pause screen, which the screen example takes over.
require("common.ui.screens.pause").setup()

-- The engine's own signals last the session once a script reads one, so they
-- are made here, before a case counts what follows the tick.
assert(trx.game.signals.is_photo_mode ~= nil)

local function run(name)
  local path = ROOT .. "/docs/trx/lua/examples/" .. name .. ".lua"
  fake.as_level_script(assert(loadfile(path)))
end

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
end

local function texts()
  local result = {}
  for _, op in ipairs(fake.scene()) do
    local text = op:match("^text .- text=(.*)$")
    if text ~= nil then
      result[#result + 1] = text
    end
  end
  return result
end

local function shows(text)
  for _, shown in ipairs(texts()) do
    if shown == text then
      return true
    end
  end
  return false
end

local function quads()
  local result = {}
  for _, op in ipairs(fake.scene()) do
    if op:match("^quad ") then
      result[#result + 1] = op
    end
  end
  return result
end

local function x_of(text)
  for _, op in ipairs(fake.scene()) do
    local x, shown = op:match("^text x=(%-?%d+) .- text=(.*)$")
    if shown == text then
      return tonumber(x)
    end
  end
  return nil
end

-- How many listeners the engine tick has, which grows by one for each signal
-- that follows it and has not stopped.
local function tick_listeners()
  local count = 0
  for _ in pairs(rawget(trx.signal.tick, "_listeners")) do
    count = count + 1
  end
  return count
end

-- Ends the level and checks that the example left nothing behind.
local function finish(listeners)
  fake.release(trx.ui.Screen.PAUSE)
  fake.take_choice(trx.ui.Screen.PAUSE)
  fake.release_all()
  fake.end_level()
  assert(trx.ui.layers.count() == 0, "a layer outlived the level")
  assert(not trx.input.is_grabbed(), "the game did not get the keys back")
  assert(#texts() == 0, "text outlived the level: " .. tostring(texts()[1]))
  assert(#quads() == 0, "a quad outlived the level")
  if listeners ~= nil then
    assert(
      tick_listeners() == listeners,
      ("the tick has %d listeners, not %d"):format(tick_listeners(), listeners)
    )
  end
  assert(fake.errors() == 0, "the example raised")
end

-------------------------------------------------------------------------------
-- ui/hud_widget
-------------------------------------------------------------------------------

test("ui/hud_widget shows in the top right for the whole level", function()
  run("ui/hud_widget")
  assert(shows("Find the three keys"), "the objective is not shown")
  assert(
    x_of("Find the three keys") > 320,
    "the objective is not on the right"
  )
  ticks(100)
  assert(shows("Find the three keys"), "the objective went away")
  finish()
end)

-------------------------------------------------------------------------------
-- ui/dialog
-------------------------------------------------------------------------------

test("ui/dialog shows a note that confirming closes", function()
  run("ui/dialog")
  assert(trx.ui.layers.count() == 1)
  assert(shows("The gate is locked."), "the note is not shown")
  assert(shows("\\{input menu_confirm} Close"), "the hint is not shown")
  assert(trx.input.is_grabbed(), "the game still reads the keys")
  fake.tick()
  press(Role.MENU_CONFIRM)
  assert(trx.ui.layers.count() == 0, "the note did not close")
  assert(not shows("The gate is locked."), "the note is still drawn")
  finish()
end)

test("ui/dialog closes when the player backs out", function()
  run("ui/dialog")
  fake.tick()
  press(Role.MENU_BACK)
  assert(trx.ui.layers.count() == 0, "the note did not close")
  finish()
end)

test("ui/dialog ignores the press that is down as it opens", function()
  fake.press(Role.MENU_CONFIRM)
  run("ui/dialog")
  fake.tick()
  assert(trx.ui.layers.count() == 1, "the note closed at once")
  finish()
end)

test("ui/dialog closes with the level", function()
  run("ui/dialog")
  finish()
end)

-------------------------------------------------------------------------------
-- ui/menu
-------------------------------------------------------------------------------

local function logging(fn)
  local logged = {}
  local info = trx.log.info
  trx.log.info = function(message)
    logged[#logged + 1] = message
  end
  local ok, err = pcall(fn, logged)
  trx.log.info = info
  assert(ok, err)
end

test("ui/menu logs the difficulty the player picks", function()
  logging(function(logged)
    run("ui/menu")
    assert(shows("Difficulty") and shows("Normal"), "the menu is not shown")
    fake.tick()
    press(Role.MENU_DOWN)
    press(Role.MENU_CONFIRM)
    assert(logged[1] == "Difficulty: Hard", tostring(logged[1]))
    assert(trx.ui.layers.count() == 0, "the menu did not close")
  end)
  finish()
end)

test(
  "ui/menu keeps the normal difficulty when the player backs out",
  function()
    logging(function(logged)
      run("ui/menu")
      fake.tick()
      press(Role.MENU_BACK)
      assert(logged[1] == "Difficulty: Normal", tostring(logged[1]))
      assert(#logged == 1, "the menu logged twice")
    end)
    finish()
  end
)

-------------------------------------------------------------------------------
-- ui/custom_widget
-------------------------------------------------------------------------------

local function count_color(ops, color)
  local count = 0
  for _, op in ipairs(ops) do
    if op:match(" color=(.*)$") == color then
      count = count + 1
    end
  end
  return count
end

test("ui/custom_widget drains one cell every five seconds", function()
  local listeners = tick_listeners()
  run("ui/custom_widget")
  local cells = quads()
  assert(#cells == 5, "drawn " .. #cells .. " cells")
  local full = cells[1]:match(" color=(.*)$")
  assert(count_color(cells, full) == 5, "a cell started empty")
  ticks(5 * trx.game.LOGIC_FPS)
  assert(count_color(quads(), full) == 4, "no cell drained")
  ticks(30 * trx.game.LOGIC_FPS)
  assert(count_color(quads(), full) == 0, "the battery is not empty")
  assert(#quads() == 5, "the empty cells are not drawn")
  finish(listeners)
end)

-------------------------------------------------------------------------------
-- ui/screen
-------------------------------------------------------------------------------

test("ui/screen draws the pause screen in place of the shipped one", function()
  run("ui/screen")
  assert(fake.offer(trx.ui.Screen.PAUSE), "the pause screen was not taken")
  assert(shows("Paused") and shows("Resume"), "the screen is not shown")
  fake.tick()
  press(Role.MENU_CONFIRM)
  assert(fake.take_choice(trx.ui.Screen.PAUSE) == CHOICE_RESUME)
  finish()
end)

test("ui/screen exits to the title from its second row", function()
  run("ui/screen")
  assert(fake.offer(trx.ui.Screen.PAUSE))
  fake.tick()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  assert(fake.take_choice(trx.ui.Screen.PAUSE) == CHOICE_EXIT_TO_TITLE)
  finish()
end)

test("ui/screen resumes when the player backs out", function()
  run("ui/screen")
  assert(fake.offer(trx.ui.Screen.PAUSE))
  fake.tick()
  press(Role.MENU_BACK)
  assert(fake.take_choice(trx.ui.Screen.PAUSE) == CHOICE_RESUME)
  finish()
end)

test("ui/screen gives the pause screen back with the level", function()
  run("ui/screen")
  finish()
  assert(fake.offer(trx.ui.Screen.PAUSE), "the shipped screen is gone")
  assert(not shows("Exit to title"), "the example still draws the screen")
  fake.release(trx.ui.Screen.PAUSE)
  fake.take_choice(trx.ui.Screen.PAUSE)
  ticks(30)
  assert(trx.ui.layers.count() == 0, "the shipped screen left a layer")
end)

-------------------------------------------------------------------------------
-- ui/signals
-------------------------------------------------------------------------------

test("ui/signals counts the time the level has run", function()
  local listeners = tick_listeners()
  run("ui/signals")
  assert(shows("0:00"), "the time does not start at 0:00")
  ticks(65 * trx.game.LOGIC_FPS)
  assert(shows("1:05"), "the time is not 1:05: " .. tostring(texts()[1]))
  finish(listeners)
end)

test("ui/signals hides the time in photo mode", function()
  run("ui/signals")
  fake.set_photo_mode(true)
  fake.tick()
  assert(not shows("0:00"), "the time shows in photo mode")
  fake.set_photo_mode(false)
  fake.tick()
  assert(shows("0:00"), "the time did not come back")
  finish()
end)

-------------------------------------------------------------------------------
-- ui/tween
-------------------------------------------------------------------------------

local function y_of(text)
  for _, op in ipairs(fake.scene()) do
    local y, shown = op:match("^text x=%-?%d+ y=(%-?%d+) .- text=(.*)$")
    if shown == text then
      return tonumber(y)
    end
  end
  return nil
end

test("ui/tween slides the banner in, holds it, and slides it out", function()
  local listeners = tick_listeners()
  run("ui/tween")
  local start = y_of("The Lost Valley")
  assert(
    start ~= nil and start < trx.ui.safe_area.y,
    "the banner starts shown"
  )
  ticks(10)
  local held = y_of("The Lost Valley")
  assert(held > trx.ui.safe_area.y, "the banner did not slide in")
  ticks(3 * trx.game.LOGIC_FPS)
  assert(y_of("The Lost Valley") == held, "the banner did not hold")
  ticks(12)
  assert(trx.ui.layers.count() == 0, "the banner did not close")
  finish(listeners)
end)

test("ui/tween leaves the input to the game", function()
  run("ui/tween")
  assert(trx.ui.layers.active() == nil, "the banner reads input")
  assert(not trx.input.is_grabbed(), "the banner took the keys")
  finish()
end)

return h.report()
