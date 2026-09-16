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

-------------------------------------------------------------------------------
-- the announcements that slide in
-------------------------------------------------------------------------------

-- The canvas is 640x480 here and the scale is one, so the cell the first
-- announcement settles in works out exactly:
--   height = 480 / 6 = 80, width = 80 * 5/4 = 100
--   margins = 480/16 = 30 down, 40 across
--   centre  = (640 - 40 - 50, 480 - 30 - 40) = (550, 410)
local SETTLED = "x=500.0 y=370.0 w=100.0 h=80.0"

local function announce(object)
  trx.config.set("ui.show_pickups_overlay", true)
  trx.config.set("visuals.enable_3d_pickups", true)
  fake.show_pickup(object)
end

local function drawn()
  local model = fake.render(0)
  for line in model:gmatch("[^\n]+") do
    if line:match("^mesh_slot ") then
      return line
    end
  end
  for line in fake.paint():gmatch("[^\n]+") do
    if line:match("^sprite ") then
      return line
    end
  end
end

test("nothing is announced until something asks for it", function()
  assert(drawn() == nil)
end)

test("an announcement settles into the first cell", function()
  announce(trx.catalog.objects.KEY_ITEM_1)
  for _ = 1, trx.game.LOGIC_FPS do
    fake.tick()
  end
  local line = drawn()
  assert(line ~= nil, "nothing was announced")
  -- The box is what the engine worked out before the announcements moved to a
  -- script, so a change of arithmetic here shows up as a moved announcement.
  assert(line:match("^mesh_slot "), line)
  assert(line:match(SETTLED), line)
end)

test("an announcement is gone once its time is up", function()
  announce(trx.catalog.objects.KEY_ITEM_1)
  -- Ease in, hold, ease out, and one tick over.
  for _ = 1, trx.game.LOGIC_FPS * 4 do
    fake.tick()
  end
  assert(drawn() == nil, drawn())
end)

test("the setting takes the announcements off the screen", function()
  announce(trx.catalog.objects.KEY_ITEM_1)
  fake.tick()
  trx.config.set("ui.show_pickups_overlay", false)
  fake.tick()
  assert(drawn() == nil, drawn())
  trx.config.set("ui.show_pickups_overlay", true)
end)
-- Runs out whatever an earlier test left on the screen.
local function drain()
  for _ = 1, trx.game.LOGIC_FPS * 4 do
    fake.tick()
  end
  assert(drawn() == nil, "an announcement outlived its time")
end

test("a second announcement takes the next cell", function()
  drain()
  announce(trx.catalog.objects.KEY_ITEM_1)
  announce(trx.catalog.objects.PUZZLE_ITEM_1)
  for _ = 1, trx.game.LOGIC_FPS do
    fake.tick()
  end
  local cells = {}
  for line in fake.render(0):gmatch("[^\n]+") do
    if line:match("^mesh_slot ") then
      cells[#cells + 1] = line:match("x=[%d%.]+")
    end
  end
  assert(#cells == 2, ("%d announcement(s) drew"):format(#cells))
  assert(cells[1] ~= cells[2], "both announcements settled in one cell")
end)

test("an announcement is a sprite with the models turned off", function()
  drain()
  fake.define_sprite(trx.catalog.objects.KEY_ITEM_1, 1, 32, 32)
  trx.config.set("ui.show_pickups_overlay", true)
  trx.config.set("visuals.enable_3d_pickups", false)
  fake.show_pickup(trx.catalog.objects.KEY_ITEM_1)
  fake.tick()
  local line = drawn()
  assert(line ~= nil, "nothing was announced")
  assert(line:match("^sprite "), line)
end)

return h.report()
