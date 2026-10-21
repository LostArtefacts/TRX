-- The shipped ammunition row of the inventory ring: offered on a weapon that
-- takes more than one kind, as examining is on an entry, and the kind it
-- chooses.

local h = require("harness")
local test = h.test

local entries = require("common.ui.ring.entries")
require("common.ui.menu").setup()

local Role = trx.input.Role
local MENU = trx.ui.Screen.MENU
local MenuMode = trx.ui.MenuMode
local CHOICE_NONE = 0
local CHOICE_CONFIRM = 2

local shotgun = trx.weapons.shotgun
local SHOTGUN = trx.inventory.icon_of(shotgun.object) or shotgun.object
local NORMAL, WIDESHOT = table.unpack(shotgun.ammo_objects)
local uzis = trx.weapons.uzis
local UZIS = trx.inventory.icon_of(uzis.object) or uzis.object

-- Long enough for any motion of the ring to end.
local SETTLE_TICKS = 40
local SLIDE_TICKS = 8

for object, inv_pos in pairs({ [SHOTGUN] = 3, [UZIS] = 4 }) do
  entries.declare({
    object_id = trx.catalog.key(trx.catalog.Context.OBJECTS, object),
    inv_pos = inv_pos,
  })
end

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
  fake.tick()
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

local function shows(wanted)
  return texts():find(wanted, 1, true) ~= nil
end

local function choose_ammo_hint()
  return h.ring_hint()
    :find(trx.locale.get("general/actions/choose_ammo"), 1, true) ~= nil
end

-- Gives Lara both weapons, and shots of each kind of shell, firing the normal
-- ones.
local function carry()
  trx.inventory.give(shotgun.object)
  trx.inventory.give(uzis.object)
  trx.inventory.set_ammo_kind(shotgun.id, WIDESHOT)
  trx.inventory.set_shots(shotgun.id, 5)
  trx.inventory.set_ammo_kind(shotgun.id, NORMAL)
  trx.inventory.set_shots(shotgun.id, 3)
end

local function open(object)
  trx.config.set("ui.text_scale", 100)
  assert(fake.offer(MENU, { mode = MenuMode.GAME, object = object }))
  ticks(SETTLE_TICKS)
end

-- Opens the row, and waits for the shotgun to rest and the row to show.
local function open_row()
  press(Role.MENU_SHOW_INFO)
  ticks(SETTLE_TICKS)
end

local function finish()
  fake.release(MENU)
  fake.take_choice(MENU)
  fake.release_all()
  trx.inventory.set_ammo_kind(shotgun.id, WIDESHOT)
  trx.inventory.set_shots(shotgun.id, 0)
  trx.inventory.set_ammo_kind(shotgun.id, NORMAL)
  trx.inventory.set_shots(shotgun.id, 0)
  trx.inventory.take(shotgun.object)
  trx.inventory.take(uzis.object)
  assert(trx.ui.layers.count() == 0, "a layer outlived the ring")
  assert(fake.errors() == 0, "the ring raised")
end

test("the ring offers to choose the ammo of the shotgun", function()
  carry()
  open(SHOTGUN)
  assert(choose_ammo_hint(), h.ring_hint())
  finish()
end)

test("a weapon with one kind offers no choice", function()
  carry()
  open(UZIS)
  assert(not choose_ammo_hint(), h.ring_hint())
  finish()
end)

test("the row counts the kind the weapon fires", function()
  carry()
  open(SHOTGUN)
  open_row()
  assert(shows("3 x "), "the normal shells are not counted")
  assert(
    shows(trx.locale.get("general/actions/choose_ammo")),
    "the frame shows no hints"
  )
  assert(h.ring_hint() == "", "the ring shows hints over the frame")
  local ring = require("common.ui.ring")
  assert(ring.signals.count:get() == nil, "the ring shows a count")
  finish()
end)

test("a chosen kind is fired, and the shotgun goes back", function()
  carry()
  open(SHOTGUN)
  open_row()
  press(Role.MENU_RIGHT)
  ticks(SLIDE_TICKS)
  assert(shows("5 x "), "right did not bring the wideshot shells")
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(trx.inventory.ammo_kind(shotgun.id) == WIDESHOT, "nothing chosen")
  assert(fake.take_choice(MENU) == CHOICE_NONE, "the ring closed")
  assert(fake.calls().use_item.count == 0, "the shotgun was drawn")
  assert(choose_ammo_hint(), "the ring did not take the shotgun back")
  finish()
end)

test("back keeps the kind the weapon fired", function()
  carry()
  open(SHOTGUN)
  open_row()
  press(Role.MENU_RIGHT)
  ticks(SLIDE_TICKS)
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  assert(trx.inventory.ammo_kind(shotgun.id) == NORMAL, "the kind changed")
  assert(fake.take_choice(MENU) == CHOICE_NONE, "the ring closed")
  finish()
end)

test("picking the shotgun still draws it at once", function()
  carry()
  open(SHOTGUN)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the ring stayed open")
  assert(
    fake.calls().use_item.object_id == SHOTGUN,
    "the shotgun was not drawn"
  )
  finish()
end)

-- Returns where the ring draws its count down the screen, or nil where it
-- draws none.
local function count_y()
  local count = require("common.ui.ring").signals.count:get()
  if count == nil then
    return nil
  end
  for _, op in ipairs(fake.scene()) do
    local y, text = op:match("^text x=%-?%d+ y=(%-?%d+) .- text=(.*)$")
    if text == count then
      return tonumber(y)
    end
  end
  return nil
end

test("the count stays where it is as the hints go", function()
  require("common.ui.captions").setup()
  require("common.ui.ring_hints").setup()
  carry()
  open(SHOTGUN)
  local before = count_y()
  assert(before ~= nil, "the ring drew no count")
  fake.release_all()
  fake.press(Role.MENU_CONFIRM)
  fake.tick()
  fake.release_all()
  fake.tick()
  assert(h.ring_hint() == "", "the hints did not go")
  local after = count_y()
  assert(
    after == before,
    ("the count moved from %d to %s"):format(before, after)
  )
  finish()
end)

return h.report()
