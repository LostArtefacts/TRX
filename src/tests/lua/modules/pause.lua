-- The shipped pause module: the question it asks, the second question behind
-- it, and the choice each answer gives the pause screen.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local PAUSE = trx.ui.Screen.PAUSE
local L = trx.locale.get

local CHOICE_CANCEL = 1
local CHOICE_RESUME = 3
local CHOICE_EXIT_TO_TITLE = 4

require("common.pause").setup()

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
end

local function open()
  fake.release(PAUSE)
  fake.take_choice(PAUSE)
  assert(fake.offer(PAUSE), "the question was not taken")
  -- The press that opened the question.
  fake.tick()
end

local function finish()
  fake.release(PAUSE)
  fake.take_choice(PAUSE)
  fake.release_all()
  assert(trx.ui.layers.count() == 0, "a layer outlived the question")
  assert(fake.errors() == 0, "the module raised")
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

test("the question asks whether to leave", function()
  open()
  local drawn = texts()
  assert(has(drawn, L("general/pause/exit_to_title")))
  assert(has(drawn, L("general/pause/continue")))
  assert(has(drawn, L("general/pause/quit")))
  finish()
end)

test("continue returns to the game", function()
  open()
  press(Role.MENU_CONFIRM)
  assert(fake.take_choice(PAUSE) == CHOICE_RESUME)
  finish()
end)

test("the back key drops the question and stays paused", function()
  open()
  press(Role.MENU_BACK)
  assert(fake.take_choice(PAUSE) == CHOICE_CANCEL)
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
  assert(fake.take_choice(PAUSE) == CHOICE_EXIT_TO_TITLE)
  finish()
end)

test("no returns to the game", function()
  open()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  fake.tick()
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  assert(fake.take_choice(PAUSE) == CHOICE_RESUME)
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
