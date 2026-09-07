-- The shipped overlay module and the photo mode panel it requires: that they
-- load, and that drawing every region reaches no name the API does not have.
--
-- Every field, enum spelling and config key the modules name is unchecked
-- until something calls it, so the whole point here is to call it.

local h = require("harness")
local test = h.test

-- Draws all overlay regions and returns the recorded scene.
local function draw()
  local description, balanced = fake.draw_regions()
  assert(balanced, "a widget was left open for the next frame to draw into")
  -- The dispatcher logs a handler's error and carries on, so a region that
  -- raised would otherwise read as a region that drew nothing.
  assert(
    fake.errors() == 0,
    ("%d region(s) raised - see the errors above"):format(fake.errors())
  )
  return description
end

-- Leaves the panel as the only thing the regions hold.
local function only_the_panel()
  trx.config.set("ui.text_scale", 1)
  trx.config.set("ui.enable_game_ui", false)
  trx.config.set("ui.enable_photo_mode_ui", true)
end

test("the module registers a draw handler", function()
  assert(type(fake.draw_regions) == "function")
end)

test("drawing every region reaches only names the API has", function()
  -- Every size the widgets measure is a multiple of these, and a reset leaves
  -- the settings at zero, which measures the whole overlay away.
  trx.config.set("ui.text_scale", 1)
  trx.config.set("ui.bar_scale", 1)
  trx.config.set("ui.enable_game_ui", true)
  trx.config.set("ui.show_bars", true)

  fake.tick()

  assert(draw():match("quad"), "the overlay drew nothing")
end)

test("the photo mode panel stays off the screen outside photo mode", function()
  only_the_panel()
  fake.set_photo_mode(false)
  fake.tick()

  assert(draw() == "", "the panel drew while photo mode was closed")
end)

test("the photo mode panel is drawn while photo mode is open", function()
  only_the_panel()
  fake.set_photo_mode(true)
  fake.tick()

  assert(draw():match("text_background"), "the panel drew no frame")
end)

test("the photo mode panel follows what photo mode is steering", function()
  only_the_panel()
  fake.set_photo_mode(true, false)
  fake.tick()
  local camera = draw()

  fake.set_photo_mode(true, true)
  fake.tick()
  assert(draw() ~= camera, "the panel read the same with Lara steered")
end)

test("the help setting takes the photo mode panel off the screen", function()
  only_the_panel()
  fake.set_photo_mode(true)
  trx.config.set("ui.enable_photo_mode_ui", false)
  fake.tick()

  assert(draw() == "", "the panel drew with its setting turned off")
end)

return h.report()
