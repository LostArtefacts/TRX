-- The shipped pause module: the backdrop the game stands still behind, the word
-- that says it is paused, the question it asks, the second question behind
-- it, and the choice each answer gives the pause screen.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local PAUSE = trx.ui.Screen.PAUSE
local L = trx.locale.get

local CHOICE_RESUME = 3
local CHOICE_EXIT_TO_TITLE = 4
local CHOICE_NONE = 0

-- The game is in a level, which the question needs.
local CAVES = 2

require("common.ui.screens.pause").setup()

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
end

local function ticks(count)
  for _ = 1, count do
    fake.tick()
  end
end

local function caption()
  local call = fake.calls().set_caption
  return call ~= nil and call.caption or nil
end

-- Pauses the game, and waits for the backdrop to come in.
local function pause()
  trx.config.set("ui.pause_fade_effects", true)
  trx.config.set("ui.pause_background_style", "transparent")
  fake.set_current_level(CAVES)
  fake.release(PAUSE)
  fake.take_choice(PAUSE)
  assert(fake.offer(PAUSE), "the pause screen was not taken")
  ticks(14)
end

-- Pauses the game and opens the question.
local function open()
  pause()
  press(Role.OPTION)
  -- The press that opened the question.
  fake.tick()
end

local function finish()
  fake.release(PAUSE)
  fake.take_choice(PAUSE)
  fake.release_all()
  assert(trx.ui.layers.count() == 0, "a layer outlived the screen")
  assert(fake.errors() == 0, "the module raised")
end

-- Waits for the screen to leave, and returns the choice it left with.
local function left()
  ticks(20)
  return fake.take_choice(PAUSE)
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

local function has(list, wanted)
  for _, text in ipairs(list) do
    if text == wanted then
      return true
    end
  end
  return false
end

test("the backdrop comes in, then the screen says it is paused", function()
  trx.config.set("ui.pause_fade_effects", true)
  trx.config.set("ui.pause_background_style", "transparent")
  fake.set_current_level(CAVES)
  assert(fake.offer(PAUSE), "the pause screen was not taken")
  local backdrop = nil
  for _, op in ipairs(fake.scene()) do
    if op:match("^backdrop ") then
      backdrop = op
    end
  end
  assert(backdrop ~= nil, "no backdrop came in")
  assert(caption() ~= L("general/pause/paused"), "the word came too soon")
  ticks(14)
  assert(caption() == L("general/pause/paused"), "the word did not come")
  finish()
end)

test("the pause key returns to the game", function()
  pause()
  press(Role.PAUSE)
  assert(left() == CHOICE_RESUME)
  finish()
end)

test("the inventory key asks whether to leave", function()
  open()
  local drawn = texts()
  assert(has(drawn, L("general/pause/exit_to_title")))
  assert(has(drawn, L("general/pause/continue")))
  assert(has(drawn, L("general/pause/quit")))
  finish()
end)

test("outside a level the inventory key returns to the game", function()
  pause()
  fake.set_current_level(nil)
  press(Role.OPTION)
  assert(left() == CHOICE_RESUME)
  finish()
end)

test("continue returns to the game", function()
  open()
  press(Role.MENU_CONFIRM)
  assert(left() == CHOICE_RESUME)
  finish()
end)

test("the back key drops the question and stays paused", function()
  open()
  press(Role.MENU_BACK)
  assert(left() == CHOICE_NONE, "the screen left")
  assert(not has(texts(), L("general/pause/exit_to_title")))
  assert(fake.is_held(PAUSE))
  finish()
end)

test("quit asks whether the player is sure", function()
  open()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  local drawn = texts()
  assert(has(drawn, L("general/pause/are_you_sure")))
  assert(
    not has(drawn, L("general/pause/exit_to_title")),
    "the first question showed behind the second"
  )
  assert(fake.is_held(PAUSE))
  finish()
end)

test("yes leaves for the title screen", function()
  open()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  fake.tick()
  press(Role.MENU_CONFIRM)
  assert(left() == CHOICE_EXIT_TO_TITLE)
  finish()
end)

test("the question stays on screen while the screen leaves", function()
  open()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  fake.tick()
  press(Role.MENU_CONFIRM)
  ticks(5)
  assert(
    has(texts(), L("general/pause/are_you_sure")),
    "the question went before the screen"
  )
  assert(
    caption() == L("general/pause/paused"),
    "the caption went, which moves the question"
  )
  assert(left() == CHOICE_EXIT_TO_TITLE)
  finish()
end)

test("no returns to the game", function()
  open()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  fake.tick()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  assert(left() == CHOICE_RESUME)
  finish()
end)

test("backing out of the second question keeps the cursor on quit", function()
  open()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  fake.tick()
  press(Role.MENU_BACK)
  assert(has(texts(), L("general/pause/exit_to_title")))
  assert(fake.is_held(PAUSE))
  -- The cursor is still on quit, so confirming asks again.
  press(Role.MENU_CONFIRM)
  assert(has(texts(), L("general/pause/are_you_sure")))
  finish()
end)

return h.report()
