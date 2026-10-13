-- The shipped globe module: which areas open, turning between them, and
-- starting the adventure picked.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local GLOBE = trx.ui.Screen.GLOBE_SELECT
local Arrow = trx.overlay.Arrow

local CHOICE_NONE = 0

-- The caption is the area's name and nothing else, so it reads straight back.
trx.locale.declare({ ["general/inventory_ring/object_name_fmt"] = "%s" })
require("common.ui.screens.globe").setup()

-- A tick, with the game's own clock moving on with it, which the screen's
-- fades are timed by.
local function tick()
  fake.advance_sim_time(1 / trx.game.LOGIC_FPS)
  fake.tick()
end

local function press(role)
  fake.release_all()
  fake.press(role)
  tick()
  fake.release_all()
end

local function ticks(count)
  for _ = 1, count do
    tick()
  end
end

-- Area 1 starts on level 1; area 2 starts on level 2 once area 1 is done; area
-- 3 has no adventure.
local AREAS = {
  { start = 1, finish = 1, mesh = 1, rot = { 0, 0, 0 } },
  { start = 2, finish = 2, mesh = 2, rot = { 0, 16384, 0 }, prereqs = { 1 } },
  { mesh = 3, rot = { 0, -16384, 0 } },
}

local function open(completed, areas)
  fake.set_globe(areas or AREAS)
  fake.set_completed(completed or {})
  assert(fake.offer(GLOBE, 0), "the globe was not taken")
  -- The globe comes in on the ring before it names anything.
  ticks(40)
end

local function last_caption()
  return fake.calls().set_caption.caption
end

local function arrows_shown()
  -- The log answers any name it lacks, so ipairs would never stop on it.
  local calls = fake.calls()
  local shown = {}
  local i = 1
  while rawget(calls, i) ~= nil do
    local call = rawget(calls, i)
    if call.name == "show_arrow" then
      shown[call.arrow] = call.show
    end
    i = i + 1
  end
  return shown[Arrow.CAPTION_LEFT], shown[Arrow.CAPTION_RIGHT]
end

local function models()
  local result = {}
  for _, op in ipairs(fake.scene()) do
    if op:match("^world_model ") then
      result[#result + 1] = op
    end
  end
  return result
end

local function finish()
  fake.release(GLOBE)
  fake.take_choice(GLOBE)
  fake.release_all()
  assert(trx.ui.layers.count() == 0, "a layer outlived the screen")
  assert(fake.errors() == 0, "the module raised")
end

test("an area opens once the areas it needs are done", function()
  open({})
  assert(last_caption() == trx.locale.get("general/globe_select/area_1"))
  local left, right = arrows_shown()
  assert(left == false and right == false, "arrows with nowhere else to go")
  finish()
end)

test("a finished area gives way to the next", function()
  open({ 1 })
  assert(last_caption() == trx.locale.get("general/globe_select/area_2"))
  finish()
end)

test("the arrows show where there is a choice", function()
  open({}, {
    { start = 1, finish = 1, mesh = 1 },
    { start = 2, finish = 2, mesh = 2 },
  })
  local left, right = arrows_shown()
  assert(left and right, "no arrows with two areas open")
  ticks(60)
  press(Role.MENU_RIGHT)
  assert(last_caption() == trx.locale.get("general/globe_select/area_2"))
  finish()
end)

test("turning skips the areas that are shut", function()
  open({}, {
    { start = 1, finish = 1, mesh = 1 },
    { start = 2, finish = 2, mesh = 2, prereqs = { 1 } },
    { start = 2, finish = 2, mesh = 3 },
  })
  ticks(60)
  press(Role.MENU_RIGHT)
  assert(last_caption() == trx.locale.get("general/globe_select/area_3"))
  finish()
end)

test("confirm starts the adventure once the screen goes dark", function()
  open({})
  ticks(60)
  press(Role.MENU_CONFIRM)
  assert(fake.calls().play_level.count == 0, "it left before going dark")
  assert(last_caption() == "", "the caption stayed while leaving")
  assert(trx.ui.layers.count() == 1, "the heading stayed while leaving")
  -- The globe goes back onto the ring, the ring closes, and the screen goes
  -- dark.
  ticks(30)
  assert(fake.calls().play_level.count == 0, "it left before the ring closed")
  ticks(40)
  local calls = fake.calls().play_level
  assert(calls.count == 1, ("%d level(s) started"):format(calls.count))
  assert(calls.num == 1, "the wrong level started")
  assert(fake.take_choice(GLOBE) == CHOICE_NONE, "the screen ended itself")
  finish()
end)

-- The alpha of the black over the screen, or nil once it is clear.
local function black()
  for _, op in ipairs(fake.scene()) do
    local alpha = op:match("^quad x=0 y=0 z=0 .* color=000000(%x%x)$")
    if alpha ~= nil then
      return tonumber(alpha, 16)
    end
  end
  return nil
end

test("the globe comes in from black and goes dark to leave", function()
  trx.config.set("visuals.enable_fade_effects", true)
  fake.set_globe(AREAS)
  fake.set_completed({})
  assert(fake.offer(GLOBE, 0), "the globe was not taken")
  assert(black() == 0xFF, "the globe did not start black")
  ticks(31)
  assert(black() == nil, "the black stayed")
  ticks(60)
  press(Role.MENU_CONFIRM)
  ticks(30)
  while fake.calls().play_level.count == 0 do
    tick()
  end
  assert(black() == 0xFF, "the adventure started before the screen went dark")
  finish()
end)

test("the globe stands on the screen's picture", function()
  fake.set_background(GLOBE, "india.webp")
  open({})
  local drawn = false
  for _, op in ipairs(fake.scene()) do
    if op:match("^background path=india.webp mono=1 dim=0.50") then
      drawn = true
    end
  end
  fake.set_background(GLOBE, nil)
  assert(drawn, "the picture was not drawn")
  finish()
end)

test(
  "the globe lights its markers and draws only areas with adventures",
  function()
    open({})
    local line = models()[1]
    assert(line ~= nil, "the globe was not drawn")
    -- Area 3 has no adventure, so its marker is left out.
    local mask = tonumber(line:match(" mask=(%d+) "))
    assert(mask & (1 << 3) == 0, "a marker drew for an area with no adventure")
    assert(mask & (1 << 1) ~= 0, "the marker of an open area is missing")
    -- Every mesh takes a light of its own: the sea, the land and the markers,
    -- from the three lights the ring lit the globe by.
    assert(line:match(" lit=4095 "), line)
    assert(line:match(" lights=3 "), line)
    -- It stands on the ring, seen through the ring's camera.
    assert(line:match(" fov=" .. 80 * trx.math.DEG_1), line)
    finish()
  end
)

test("the caption and arrows go with the screen", function()
  open({})
  finish()
  assert(last_caption() == "", "the caption stayed")
  local left, right = arrows_shown()
  assert(left == false and right == false)
end)

return h.report()
