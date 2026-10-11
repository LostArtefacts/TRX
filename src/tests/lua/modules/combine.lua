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
local SLIDE_TICKS = trx.game.LOGIC_FPS // 4

require("common.combine").setup()

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
  fake.open_ring(trx.inventory_ring.Mode.GAME, OBJECT, 0, 10)
  assert(fake.offer(COMBINE, OBJECT), "the entry was not taken")
  -- The press that opened the entry, then the first tick it rests open.
  fake.tick()
  fake.tick()
end

local function free_slots()
  local slots = {}
  while true do
    local slot = trx.ui.mesh_slot()
    if slot == nil then
      break
    end
    slots[#slots + 1] = slot
  end
  for _, slot in ipairs(slots) do
    slot:release()
  end
  return #slots
end

local function finish()
  fake.release(COMBINE)
  fake.take_choice(COMBINE)
  fake.close_ring()
  fake.release_all()
  assert(trx.ui.layers.count() == 0, "a layer outlived the screen")
  assert(fake.errors() == 0, "the module raised")
end

local all_slots = free_slots()

test("the frame names the entry in the middle", function()
  open(PARTNERS)
  assert(shows_name(PARTNERS[1]))
  assert(fake.ring_hint():find(" | ", 1, true), "the hint lost a part")
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
  assert(fake.take_choice(COMBINE) == CHOICE_CONFIRM)
  assert(fake.choice_value(COMBINE) == PARTNERS[2], "the wrong partner")
  finish()
end)

test("back puts the entry away", function()
  open(PARTNERS)
  press(Role.MENU_BACK)
  assert(fake.take_choice(COMBINE) == CHOICE_CANCEL)
  finish()
end)

test("an entry with nothing to combine with is put away", function()
  open({})
  assert(fake.take_choice(COMBINE) == CHOICE_CANCEL)
  finish()
end)

test("the models are given back when the entry is put away", function()
  open(PARTNERS)
  assert(free_slots() < all_slots, "the row took no models")
  press(Role.MENU_BACK)
  finish()
  assert(free_slots() == all_slots, "a model outlived the screen")
end)

return h.report()
