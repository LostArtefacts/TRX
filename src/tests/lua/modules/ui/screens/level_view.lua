-- The shipped level view module: fading in and out through a fade layer, the
-- loading bar, skipping, and closing its fade with the screen.

local h = require("harness")
local test = h.test

h.load_themes()

local Role = trx.input.Role
local VIEW = trx.ui.Screen.LEVEL_VIEW

local CHOICE_NONE = 0
local CHOICE_CONFIRM = 2

local DISPLAY = 20
local FADE_IN = 5
local FADE_OUT = 4
local HOLD = 3

require("common.ui.screens.level_view").setup()

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

-- The alpha of the view's fade layer, which fills the screen from its corner,
-- or nil once it is clear.
local function black()
  for _, op in ipairs(fake.scene()) do
    local alpha = op:match("^quad x=0 y=0 z=0 .* color=000000(%x%x)$")
    if alpha ~= nil then
      return tonumber(alpha, 16)
    end
  end
  return nil
end

local function has_bar()
  -- Every quad but the fade layer's belongs to the loading bar.
  for _, op in ipairs(fake.scene()) do
    if op:match("^quad ") and not op:match("^quad x=0 y=0 z=0 ") then
      return true
    end
  end
  return false
end

local function open()
  assert(
    fake.offer(VIEW, {
      display_ticks = DISPLAY,
      fade_in_ticks = FADE_IN,
      fade_out_ticks = FADE_OUT,
    }),
    "the view was not taken"
  )
end

local function finish()
  fake.release(VIEW)
  fake.take_choice(VIEW)
  fake.release_all()
  assert(trx.ui.layers.count() == 0, "a layer outlived the screen")
  assert(black() == nil, "the fade outlived the screen")
  assert(fake.errors() == 0, "the module raised")
end

test("the view fades in from black", function()
  trx.config.set("visuals.enable_fade_effects", true)
  open()
  assert(black() == 0xFF, "the view did not start black")
  ticks(FADE_IN + 1)
  assert(black() == nil, "the black stayed after the fade in")
  finish()
end)

test("the view shows for its time, fades out and ends", function()
  trx.config.set("visuals.enable_fade_effects", true)
  open()
  ticks(FADE_IN + DISPLAY + 2)
  assert(black() ~= nil, "the view did not start to fade out")
  ticks(FADE_OUT + HOLD - 3)
  assert(fake.take_choice(VIEW) == CHOICE_NONE, "the view ended too soon")
  fake.tick()
  assert(fake.take_choice(VIEW) == CHOICE_CONFIRM, "the view did not end")
  finish()
end)

test("a skip fades out, and a second skip ends the view", function()
  trx.config.set("visuals.enable_fade_effects", true)
  open()
  ticks(FADE_IN + 2)
  press(Role.MENU_SKIP)
  ticks(2)
  assert(black() ~= nil, "the skip did not fade the view out")
  assert(fake.take_choice(VIEW) == CHOICE_NONE, "the first skip ended it")
  press(Role.MENU_SKIP)
  assert(fake.take_choice(VIEW) == CHOICE_CONFIRM, "the second skip did not")
  finish()
end)

test("the loading bar follows its setting", function()
  trx.config.set("ui.show_loading_bar", true)
  open()
  ticks(FADE_IN + 2)
  assert(has_bar(), "no loading bar")
  finish()

  trx.config.set("ui.show_loading_bar", false)
  open()
  ticks(FADE_IN + 2)
  assert(not has_bar(), "a loading bar against the setting")
  finish()
end)

test("a PS1 loading bar carries a caption above it", function()
  local function caption()
    for _, op in ipairs(fake.scene()) do
      if op:match("^text .- text=LOADING$") then
        return true
      end
    end
    return false
  end

  trx.config.set("ui.show_loading_bar", true)
  trx.config.set("ui.bar_look", "ps1")
  open()
  ticks(FADE_IN + 2)
  assert(caption(), "no caption on a PS1 bar")
  finish()

  trx.config.set("ui.bar_look", "pc")
  open()
  ticks(FADE_IN + 2)
  assert(not caption(), "a caption on a PC bar")
  finish()
end)

test("with fade effects off the view comes in and goes at once", function()
  trx.config.set("visuals.enable_fade_effects", false)
  open()
  fake.tick()
  assert(black() == nil, "the view faded in")
  ticks(DISPLAY + 2)
  assert(fake.take_choice(VIEW) == CHOICE_CONFIRM, "the view did not end")
  finish()
  trx.config.set("visuals.enable_fade_effects", true)
end)

return h.report()
