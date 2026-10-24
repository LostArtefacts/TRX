-- The shipped combine module: what an entry combines with, in a row, slid
-- along to the one wanted, and the choice that ends it.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local COMBINE = trx.ui.Screen.COMBINE
local objects = trx.catalog.objects
local OBJECT = objects.PUZZLE_ITEM_1
local PARTNERS = { objects.KEY_ITEM_1, objects.CROWBAR_ITEM }

local CHOICE_CANCEL = 1
local CHOICE_CONFIRM = 2
local SLIDE_TICKS = 8

require("common.ui.screens.combine").setup()

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

local function shows_name(object)
  return texts():find(trx.objects[object].name, 1, true) ~= nil
end

local function open(partners)
  trx.config.set("ui.text_scale", 100)
  fake.set_combine_partners(partners)
  h.open_ring(trx.ui.MenuMode.GAME, OBJECT, 0, 10)
  assert(fake.offer(COMBINE, { object = OBJECT }), "the entry was not taken")
  -- The press that opened the entry, then the first tick it rests open.
  fake.tick()
  fake.tick()
end

local function models()
  local result = {}
  for _, op in ipairs(fake.scene()) do
    local object, w = op:match("^model obj=(%d+) .- w=(%d+) ")
    if object ~= nil then
      result[#result + 1] = { object = tonumber(object), w = tonumber(w) }
    end
  end
  return result
end

local function finish()
  fake.release(COMBINE)
  fake.take_choice(COMBINE)
  h.close_ring()
  fake.release_all()
  assert(trx.ui.layers.count() == 0, "a layer outlived the screen")
  assert(fake.errors() == 0, "the module raised")
end

test("the frame names the entry in the middle", function()
  open(PARTNERS)
  assert(shows_name(PARTNERS[1]))
  assert(
    h.ring_hint()
      == "\\{input menu_confirm} "
        .. trx.locale.get("general/actions/combine_item")
        .. " | \\{input menu_back} "
        .. trx.locale.get("general/actions/cancel"),
    h.ring_hint()
  )
  finish()
end)

test("right and left slide the row", function()
  open(PARTNERS)
  press(Role.MENU_RIGHT)
  ticks(SLIDE_TICKS)
  assert(shows_name(PARTNERS[2]), "right did not bring the next entry")
  press(Role.MENU_LEFT)
  ticks(SLIDE_TICKS)
  assert(shows_name(PARTNERS[1]), "left did not bring it back")
  finish()
end)

test("the frame names the next entry once it reaches the middle", function()
  open(PARTNERS)
  press(Role.MENU_RIGHT)
  assert(shows_name(PARTNERS[1]), "the name changed as the row started")
  ticks(SLIDE_TICKS)
  assert(shows_name(PARTNERS[2]), "the name did not follow the row")
  finish()
end)

-- Returns how far the model of an object is turned, or nil where it is not
-- drawn.
local function turn_of(object)
  for _, op in ipairs(fake.scene()) do
    local drawn, rot_y = op:match("^model obj=(%d+) .- rot_y=(%-?%d+) ")
    if drawn ~= nil and tonumber(drawn) == object then
      return tonumber(rot_y)
    end
  end
  return nil
end

test("the next entry starts to spin once it reaches the middle", function()
  open(PARTNERS)
  local rest = turn_of(PARTNERS[2])
  assert(rest ~= nil, "the next entry is not drawn")
  press(Role.MENU_RIGHT)
  ticks(3)
  assert(turn_of(PARTNERS[2]) == rest, "the next entry spun as it slid")
  ticks(SLIDE_TICKS)
  assert(turn_of(PARTNERS[2]) ~= rest, "the entry in the middle stood still")
  finish()
end)

test("the row wraps round", function()
  open(PARTNERS)
  press(Role.MENU_LEFT)
  ticks(SLIDE_TICKS)
  assert(shows_name(PARTNERS[2]), "left from the first did not wrap")
  finish()
end)

test("a single partner does not slide", function()
  open({ PARTNERS[1] })
  press(Role.MENU_RIGHT)
  ticks(SLIDE_TICKS)
  assert(shows_name(PARTNERS[1]))
  finish()
end)

test("confirm combines with the entry in the middle", function()
  open(PARTNERS)
  press(Role.MENU_RIGHT)
  ticks(SLIDE_TICKS)
  press(Role.MENU_CONFIRM)
  local object, partner = fake.combined()
  assert(object == OBJECT, "the held entry was not combined")
  assert(partner == PARTNERS[2], "the wrong partner")
  assert(fake.take_choice(COMBINE) == CHOICE_CONFIRM)
  finish()
end)

local function plays()
  local play = fake.calls().play
  return play ~= nil and play.count or 0
end

test("the ring goes dark only while the frame shows", function()
  local ring = require("common.ui.ring")
  trx.config.set("ui.text_scale", 100)
  fake.set_combine_partners(PARTNERS)
  h.open_ring(trx.ui.MenuMode.GAME, OBJECT, 0, 10)
  assert(fake.offer(COMBINE, { object = OBJECT }), "the entry was not taken")
  assert(not ring.signals.darkened:get(), "the ring went dark too early")
  fake.tick()
  fake.tick()
  assert(shows_name(PARTNERS[1]), "the frame did not show")
  assert(ring.signals.darkened:get(), "the ring did not go dark")
  press(Role.MENU_BACK)
  assert(not ring.signals.darkened:get(), "the ring stayed dark")
  finish()
end)

test("back puts the entry away", function()
  open(PARTNERS)
  local before = plays()
  press(Role.MENU_BACK)
  assert(fake.take_choice(COMBINE) == CHOICE_CANCEL)
  -- The ring sounds the entry going away, so the screen does not as well.
  assert(plays() == before, "the screen sounded the entry going away")
  finish()
end)

test("an entry with nothing to combine with is put away", function()
  open({})
  assert(fake.take_choice(COMBINE) == CHOICE_CANCEL)
  finish()
end)

test("the entry in the middle is drawn largest", function()
  open({ PARTNERS[1], PARTNERS[2], objects.SMALL_MEDIPACK_ITEM })
  -- The row is laid out on the tick after it is first painted.
  fake.scene()
  fake.tick()
  local drawn = models()
  assert(#drawn == 3, ("%d model(s) drawn"):format(#drawn))
  local largest = drawn[1]
  for _, model in ipairs(drawn) do
    if model.w > largest.w then
      largest = model
    end
  end
  assert(largest.object == PARTNERS[1], "the middle entry is not the largest")
  finish()
end)

test("nothing is drawn once the entry is put away", function()
  open(PARTNERS)
  fake.scene()
  fake.tick()
  assert(#models() > 0, "the row drew nothing")
  press(Role.MENU_BACK)
  finish()
  assert(#models() == 0, "a model outlived the screen")
end)

test("the frame stays where it first shows", function()
  require("common.ui.captions").setup()
  require("common.ui.ring_hints").setup()
  trx.config.set("ui.text_scale", 100)
  fake.set_combine_partners(PARTNERS)
  h.open_ring(trx.ui.MenuMode.GAME, OBJECT, 0, 10)
  trx.overlay.set_caption("caption")
  assert(fake.offer(COMBINE, { object = OBJECT }), "the entry was not taken")

  local name = trx.objects[PARTNERS[1]].name
  local first = nil
  for _ = 1, 6 do
    fake.tick()
    for _, op in ipairs(fake.scene()) do
      local y, text = op:match("^text x=%-?%d+ y=(%-?%d+) .- text=(.*)$")
      if text == name then
        first = first or y
        assert(y == first, ("the frame moved from %s to %s"):format(first, y))
      end
    end
  end
  assert(first ~= nil, "the frame never showed")
  finish()
end)

return h.report()
