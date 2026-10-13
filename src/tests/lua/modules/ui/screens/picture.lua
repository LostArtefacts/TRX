-- The shipped picture module: fading in from black or from the frame before,
-- the loading bar, skipping, fading out, and handing a loading picture over
-- to the level.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local PICTURE = trx.ui.Screen.PICTURE

local CHOICE_NONE = 0
local CHOICE_CONFIRM = 2

local DISPLAY = 20
local FADE_IN = 5
local FADE_OUT = 4
local LEVEL_FADE_IN = 16

require("common.ui.fade").setup()
require("common.ui.level_start").setup()
require("common.ui.screens.picture").setup()

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

-- The alpha of the black that fills the screen from its corner, or nil.
local function black()
  for _, op in ipairs(fake.scene()) do
    local alpha = op:match("^quad x=0 y=0 z=0 .* color=000000(%x%x)$")
    if alpha ~= nil then
      return tonumber(alpha, 16)
    end
  end
  return nil
end

local function has(pattern)
  for _, op in ipairs(fake.scene()) do
    if op:match(pattern) then
      return true
    end
  end
  return false
end

local function set_transition(mode)
  trx.config.set("visuals.level_start_transition", mode)
end

local function open(is_loading, can_cross_fade_in)
  trx.config.set("visuals.enable_fade_effects", true)
  assert(
    fake.offer(PICTURE, {
      background = "picture.webp",
      display_ticks = DISPLAY,
      fade_in_ticks = FADE_IN,
      fade_out_ticks = FADE_OUT,
      is_loading = is_loading,
      can_cross_fade_in = can_cross_fade_in,
    }),
    "the picture was not taken"
  )
end

local function finish()
  fake.release(PICTURE)
  fake.take_choice(PICTURE)
  fake.release_all()
  fake.start_title()
  assert(trx.ui.layers.count() == 0, "a layer outlived the screen")
  assert(black() == nil, "a fade outlived the screen")
  assert(not has("^snapshot "), "a blend outlived the screen")
  assert(fake.errors() == 0, "the module raised")
end

test("a picture fades in from black and draws itself", function()
  open(false, false)
  assert(black() == 0xFF, "the picture did not start black")
  assert(has("^background path=picture.webp"), "the picture was not drawn")
  ticks(FADE_IN + 1)
  assert(black() == nil, "the black stayed after the fade in")
  finish()
end)

test("a picture fades out after its time and ends", function()
  open(false, false)
  ticks(FADE_IN + DISPLAY + 2)
  assert(black() ~= nil, "the picture did not start to fade out")
  ticks(FADE_OUT - 3)
  assert(fake.take_choice(PICTURE) == CHOICE_NONE, "it ended too soon")
  ticks(2)
  assert(fake.take_choice(PICTURE) == CHOICE_CONFIRM, "it did not end")
  finish()
end)

test("a skip fades out, and a second skip ends the picture", function()
  open(false, false)
  ticks(FADE_IN + 2)
  press(Role.MENU_SKIP)
  ticks(2)
  assert(black() ~= nil, "the skip did not fade out")
  assert(fake.take_choice(PICTURE) == CHOICE_NONE, "the first skip ended it")
  press(Role.MENU_SKIP)
  assert(fake.take_choice(PICTURE) == CHOICE_CONFIRM, "the second did not")
  finish()
end)

test("a loading picture fades in from the frame before it", function()
  open(true, true)
  assert(has("^snapshot slot=%d+ opacity=1"), "no blend from the frame before")
  assert(black() == nil, "it faded in from black")
  finish()
end)

test("a loading picture shows the loading bar it is set to", function()
  trx.config.set("ui.show_loading_bar", true)
  open(true, false)
  ticks(FADE_IN + 2)
  assert(has("^quad ") and has("^quad x=[1-9]"), "no loading bar")
  finish()
  trx.config.set("ui.show_loading_bar", false)
  open(true, false)
  ticks(FADE_IN + 2)
  assert(not has("^quad x=[1-9]"), "a loading bar against the setting")
  finish()
end)

-- Shows a loading picture for its time, until it asks to hand over.
local function show_loading()
  open(true, false)
  ticks(FADE_IN + 1)
  fake.scene()
  ticks(DISPLAY + 1)
end

test("a loading picture hands over to the level once drawn", function()
  set_transition("cross-fade")
  open(true, false)
  ticks(FADE_IN + 1)
  fake.scene()
  ticks(DISPLAY + 1)
  assert(fake.take_choice(PICTURE) == CHOICE_CONFIRM, "it did not hand over")
  fake.release(PICTURE)
  assert(has("^snapshot slot=%d+ opacity=1"), "the picture did not stay up")
  fake.start_game(false)
  fake.control()
  assert(black() == nil, "the level faded in from black under the blend")
  ticks(FADE_OUT + 2)
  assert(not has("^snapshot "), "the blend did not clear")
  finish()
end)

test("through black, a loading picture fades out before it ends", function()
  set_transition("black")
  show_loading()
  assert(fake.take_choice(PICTURE) == CHOICE_NONE, "it ended before the fade")
  ticks(FADE_OUT + 1)
  assert(fake.take_choice(PICTURE) == CHOICE_CONFIRM, "it did not end")
  fake.release(PICTURE)
  assert(black() == 0xFF, "the screen did not stay black")
  fake.start_game(false)
  fake.control()
  ticks(LEVEL_FADE_IN / 2)
  local alpha = black()
  assert(alpha ~= nil and alpha < 0xFF, "the level did not fade in")
  ticks(LEVEL_FADE_IN / 2 + 1)
  assert(black() == nil, "the black stayed over the level")
  assert(not has("^snapshot "), "it blended as well")
  finish()
end)

test("through black, a skip while it fades out ends it at once", function()
  set_transition("black")
  show_loading()
  press(Role.MENU_SKIP)
  assert(
    fake.take_choice(PICTURE) == CHOICE_CONFIRM,
    "the skip did not end it"
  )
  fake.release(PICTURE)
  fake.start_game(false)
  ticks(LEVEL_FADE_IN + 1)
  assert(black() == nil, "the black stayed over the level")
  finish()
end)

test("with no transition, a loading picture ends at once", function()
  set_transition("disabled")
  show_loading()
  assert(fake.take_choice(PICTURE) == CHOICE_CONFIRM, "it did not end")
  fake.release(PICTURE)
  fake.start_game(false)
  fake.control()
  assert(black() == nil, "the level faded in")
  assert(not has("^snapshot "), "the level blended in")
  finish()
end)

test("a level without a loading picture fades in from black", function()
  trx.config.set("visuals.enable_fade_effects", true)
  set_transition("cross-fade")
  fake.start_game(false)
  fake.control()
  assert(black() == 0xFF, "the level did not start black")
  fake.control()
  ticks(LEVEL_FADE_IN + 1)
  assert(black() == nil, "the black stayed over the level")

  set_transition("disabled")
  fake.start_game(false)
  fake.control()
  assert(black() == nil, "the level faded in with no transition")
  finish()
end)

test("a held fade is dropped when the title comes next", function()
  set_transition("black")
  show_loading()
  ticks(FADE_OUT + 1)
  fake.take_choice(PICTURE)
  fake.release(PICTURE)
  fake.start_title()
  assert(black() == nil, "the black outlived the picture")
  finish()
end)

test(
  "a loading picture skipped before it is drawn waits to hand over",
  function()
    set_transition("cross-fade")
    open(true, false)
    fake.tick()
    press(Role.MENU_SKIP)
    assert(fake.take_choice(PICTURE) == CHOICE_NONE, "it handed over undrawn")
    fake.scene()
    fake.tick()
    assert(fake.take_choice(PICTURE) == CHOICE_CONFIRM, "it did not hand over")
    finish()
  end
)

return h.report()
