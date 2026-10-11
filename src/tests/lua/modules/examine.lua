-- The shipped examine module: the description, a page at a time, hiding it to
-- turn the entry, and putting the entry away.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local EXAMINE = trx.ui.Screen.EXAMINE
local OBJECT = trx.catalog.objects.COMPASS_OPTION

local CHOICE_CANCEL = 1

require("common.examine").setup()

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
  return table.concat(result, "\n")
end

local function open(description)
  -- A page keeps room for its lines at the player's text size, which the fake
  -- config leaves unset.
  trx.config.set("ui.text_scale", 100)
  fake.set_description(OBJECT, description)
  fake.open_ring(trx.inventory_ring.Mode.GAME, OBJECT, 0, 10)
  assert(fake.offer(EXAMINE, OBJECT), "the entry was not taken")
  -- The press that opened the entry, then the first tick it rests open.
  fake.tick()
  fake.tick()
end

local function finish()
  fake.release(EXAMINE)
  fake.take_choice(EXAMINE)
  fake.close_ring()
  fake.release_all()
  assert(trx.ui.layers.count() == 0, "a layer outlived the screen")
  assert(fake.errors() == 0, "the module raised")
end

test("the description shows once the entry rests open", function()
  open("An old map of the caves.")
  assert(texts():find("An old map", 1, true), "the description is missing")
  assert(fake.ring_hint() == "", "a hint showed over the description")
  finish()
end)

test("show info hides the description and offers to turn the entry", function()
  open("An old map of the caves.")
  press(Role.MENU_SHOW_INFO)
  fake.tick()
  assert(not texts():find("An old map", 1, true), "the description stayed")
  assert(fake.ring_hint() ~= "", "nothing offered to turn the entry")
  assert(fake.is_held(EXAMINE))
  finish()
end)

test("show info again puts the entry away", function()
  open("An old map of the caves.")
  press(Role.MENU_SHOW_INFO)
  fake.tick()
  press(Role.MENU_SHOW_INFO)
  assert(fake.take_choice(EXAMINE) == CHOICE_CANCEL)
  finish()
end)

test("back puts the entry away", function()
  open("An old map of the caves.")
  press(Role.MENU_BACK)
  assert(fake.take_choice(EXAMINE) == CHOICE_CANCEL)
  finish()
end)

test("an entry with nothing to read is turned at once", function()
  open("")
  assert(fake.ring_hint() ~= "", "nothing offered to turn the entry")
  press(Role.MENU_CONFIRM)
  assert(fake.take_choice(EXAMINE) == CHOICE_CANCEL)
  finish()
end)

test("right turns to the next page", function()
  local lines = {}
  for i = 1, 30 do
    lines[i] = "line" .. i
  end
  open(table.concat(lines, "\n"))
  assert(texts():find("line1\n", 1, true), "the first page is missing")
  press(Role.MENU_RIGHT)
  fake.tick()
  local second = texts()
  assert(not second:find("line1\n", 1, true), "the first page stayed")
  assert(second:find("line30", 1, true) or second:find("line13", 1, true))
  finish()
end)

return h.report()
