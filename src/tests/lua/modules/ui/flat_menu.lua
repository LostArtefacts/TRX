-- The shipped flat menu: what it shows, how it turns, and what it does with
-- the entry the player picks. The menu system setting picks it over the ring.

local h = require("harness")
local test = h.test

require("common.ui.menu").setup()
local objects = trx.catalog.objects

-- The ring draws only the entries declared to it.
for object, inv_pos in pairs({
  [objects.SMALL_MEDIPACK_OPTION] = 14,
  [objects.LARGE_MEDIPACK_OPTION] = 15,
  [objects.BINOCULARS_OPTION] = 16,
}) do
  require("common.ui.ring.entries").declare({
    object_id = trx.catalog.key(trx.catalog.Context.OBJECTS, object),
    inv_pos = inv_pos,
  })
end
require("common.ui.ring.entries").declare({
  object_id = "compass_option",
  inv_pos = 0,
  frames_total = 25,
  open_frame = 10,
})
local Role = trx.input.Role
local MENU = trx.ui.Screen.MENU
local MenuMode = trx.ui.MenuMode
local CHOICE_CANCEL = 1
local CHOICE_CONFIRM = 2

-- Long enough for any fade or slide of the menu to end.
local SETTLE_TICKS = 40

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

local function models()
  local count = 0
  for _, op in ipairs(fake.scene()) do
    if op:match("^world_model ") then
      count = count + 1
    end
  end
  return count
end

local function open(mode, object)
  trx.config.set("ui.text_scale", 1)
  trx.config.set("ui.menu_system", "flat")
  assert(fake.offer(MENU, { mode = mode, object = object }), "no menu opened")
  ticks(SETTLE_TICKS)
end

local function finish()
  fake.release(MENU)
  fake.take_choice(MENU)
  fake.release_all()
  trx.config.set("ui.menu_system", "ring")
  assert(trx.ui.layers.count() == 0, "a layer outlived the menu")
  assert(fake.errors() == 0, "the menu raised")
end

local function carry()
  trx.inventory.give(objects.SMALL_MEDIPACK_OPTION, 2)
  trx.inventory.give(objects.LARGE_MEDIPACK_OPTION)
end

test("the flat menu draws what Lara carries in a row", function()
  carry()
  open(MenuMode.GAME)
  assert(models() > 0, "the menu drew nothing")
  local heading = require("common.ui.ring").signals.heading:get()
  assert(heading == nil, "the ring opened instead")
  finish()
end)

test("the row fades in with the backdrop", function()
  carry()
  trx.config.set("visuals.enable_fade_effects", true)
  trx.config.set("ui.inventory_fade_effects", true)
  trx.config.set("ui.text_scale", 1)
  trx.config.set("ui.menu_system", "flat")
  assert(fake.offer(MENU, { mode = MenuMode.GAME }), "no menu opened")
  fake.tick()
  assert(models() == 0, "the row showed before it faded in")
  ticks(SETTLE_TICKS)
  assert(models() > 0, "the row did not fade in")
  finish()
end)

local function draws(object)
  local wanted = ("^world_model obj=%d "):format(object)
  for _, op in ipairs(fake.scene()) do
    if op:match(wanted) then
      return true
    end
  end
  return false
end

test("the binoculars leave the row while they are turned off", function()
  carry()
  trx.inventory.give(objects.BINOCULARS_OPTION)
  trx.config.set("gameplay.enable_binoculars", false)
  open(MenuMode.GAME, objects.BINOCULARS_OPTION)
  assert(not draws(objects.BINOCULARS_OPTION), "the binoculars showed")
  finish()
  trx.config.set("gameplay.enable_binoculars", true)
  open(MenuMode.GAME, objects.BINOCULARS_OPTION)
  assert(draws(objects.BINOCULARS_OPTION), "the binoculars did not show")
  finish()
  trx.inventory.set_count(objects.BINOCULARS_OPTION, 0)
end)

test("a combine fades the row out and back in on what it made", function()
  carry()
  fake.set_combine_partners({ objects.LARGE_MEDIPACK_OPTION })
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  press(Role.MENU_CONFIRM)
  local faded = false
  for _ = 1, SETTLE_TICKS do
    fake.tick()
    faded = faded or models() == 0
  end
  fake.set_combine_partners({})
  local object, partner = fake.combined()
  assert(object == objects.SMALL_MEDIPACK_OPTION, "nothing was combined")
  assert(partner == objects.LARGE_MEDIPACK_OPTION, "the wrong partner")
  assert(faded, "the row did not fade out")
  assert(models() > 0, "the row did not come back")
  assert(fake.take_choice(MENU) == 0, "the menu closed")
  finish()
end)

-- Returns the middle of a line of text the scene draws on its own, by the
-- fake's measure of eight units a character.
local function text_middle(wanted)
  for _, op in ipairs(fake.scene()) do
    local x, text = op:match("^text x=(%-?%d+) .- text=(.*)$")
    if
      text ~= nil
      and text:find(wanted, 1, true) ~= nil
      and text:find("\n", 1, true) == nil
    then
      return tonumber(x) + #text * 8 * 1.1 / 2
    end
  end
  return nil
end

test("each option stands centred on its own line", function()
  carry()
  fake.set_combine_partners({ objects.LARGE_MEDIPACK_OPTION })
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  local use = text_middle(trx.locale.get("general/flat_menu/use"))
  local combine = text_middle(trx.locale.get("general/flat_menu/combine"))
  fake.set_combine_partners({})
  finish()
  assert(use ~= nil and combine ~= nil, "the options did not show")
  assert(math.abs(use - combine) <= 1, "the options are not centred")
end)

test("an entry that combines can be used as well", function()
  carry()
  fake.set_combine_partners({ objects.LARGE_MEDIPACK_OPTION })
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  fake.set_combine_partners({})
  local used = fake.calls().use_item
  assert(used.count == 1, "nothing was used")
  assert(used.object_id == objects.SMALL_MEDIPACK_OPTION, "the wrong one")
  finish()
end)

test("an entry with nothing to use it on can be used", function()
  carry()
  fake.set_unusable(true)
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  fake.set_unusable(false)
  local used = fake.calls().use_item
  assert(used.count == 1, "nothing was used")
  assert(used.object_id == objects.SMALL_MEDIPACK_OPTION, "the wrong one")
  finish()
end)

test("the press that uses an entry does not act in the game", function()
  carry()
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  fake.release_all()
  fake.press(Role.MENU_CONFIRM)
  fake.press(Role.ACTION)
  ticks(SETTLE_TICKS)
  local held_off = fake.held_off(Role.ACTION)
  fake.release_all()
  finish()
  assert(held_off, "the action key acted as the menu closed")
end)

test("using an entry uses it once the menu closes", function()
  carry()
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the menu stayed open")
  local used = fake.calls().use_item
  assert(used.count == 1, "nothing was used")
  assert(used.object_id == objects.SMALL_MEDIPACK_OPTION, "the wrong one")
  finish()
end)

test("the compass is drawn open", function()
  carry()
  trx.inventory.give(objects.COMPASS_OPTION)
  fake.set_loaded(objects.COMPASS_OPTION, true)
  open(MenuMode.GAME)
  local wanted = ("^world_model obj=%d .- frame=10 "):format(
    objects.COMPASS_OPTION
  )
  local found = false
  for _, op in ipairs(fake.scene()) do
    found = found or op:match(wanted) ~= nil
  end
  trx.inventory.set_count(objects.COMPASS_OPTION, 0)
  fake.set_loaded(objects.COMPASS_OPTION, false)
  finish()
  assert(found, "the compass was not drawn open")
end)

test("the examine screen closes back to the row", function()
  require("common.ui.screens.examine").setup()
  carry()
  fake.set_examinable(true)
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  local hint = require("common.ui.ring").signals.hint
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(models() == 1, "the row showed behind the examined entry")
  assert(draws(objects.SMALL_MEDIPACK_OPTION), "the entry did not show")
  assert(#hint:get() > 0, "turning the entry was not offered")
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(#hint:get() == 0, "the hints outlived the examine screen")
  fake.set_examinable(false)
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the examine screen hung")
  assert(fake.calls().use_item.count == 1, "nothing was used")
  finish()
end)

test("a medipack shows Lara's health bar", function()
  carry()
  local health_bar = require("common.ui.ring").signals.health_bar
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  assert(health_bar:get(), "the health bar did not show")
  fake.press(Role.MENU_LEFT)
  fake.tick()
  fake.release_all()
  assert(not health_bar:get(), "the health bar stayed as the row slid")
  ticks(SETTLE_TICKS)
  fake.press(Role.MENU_LEFT)
  for _ = 1, SETTLE_TICKS * 2 do
    fake.tick()
    assert(
      not health_bar:get(),
      "the health bar showed as the row kept turning"
    )
  end
  fake.release_all()
  ticks(SETTLE_TICKS)
  finish()
  assert(not health_bar:get(), "the health bar outlived the menu")
end)

test("turning picks the next entry", function()
  carry()
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  press(Role.MENU_RIGHT)
  ticks(SETTLE_TICKS)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  local used = fake.calls().use_item
  assert(used.count == 1, "nothing was used")
  assert(used.object_id == objects.LARGE_MEDIPACK_OPTION, "the wrong one")
  finish()
end)

test("the menu opens on the entry it was left on", function()
  carry()
  trx.config.set("gameplay.ring_memory_mode", "main_ring")
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  press(Role.MENU_RIGHT)
  ticks(SETTLE_TICKS)
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  finish()
  open(MenuMode.GAME)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  local used = fake.calls().use_item
  assert(used.object_id == objects.LARGE_MEDIPACK_OPTION, "the wrong one")
  finish()
  trx.config.set("gameplay.ring_memory_mode", "off")
end)

test("the menu opens on its first entry unless asked to remember", function()
  carry()
  trx.config.set("gameplay.ring_memory_mode", "off")
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  press(Role.MENU_RIGHT)
  ticks(SETTLE_TICKS)
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  finish()
  open(MenuMode.GAME)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  local used = fake.calls().use_item
  assert(used.object_id == objects.SMALL_MEDIPACK_OPTION, "the wrong one")
  finish()
end)

test("back closes the menu and uses nothing", function()
  carry()
  open(MenuMode.GAME)
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the menu stayed open")
  assert(fake.calls().use_item.count == 0, "something was used")
  finish()
end)

test("a save from the game saves into the picked slot", function()
  open(MenuMode.SAVE)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.calls().save.count == 1, "nothing was saved")
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the menu stayed open")
  finish()
end)

test("the quick save screen saves into the picked slot", function()
  local SAVE_LOAD = trx.ui.Screen.SAVE_LOAD
  trx.config.set("ui.menu_system", "flat")
  assert(fake.offer(SAVE_LOAD, { mode = MenuMode.SAVE }), "no list opened")
  ticks(2)
  press(Role.MENU_CONFIRM)
  assert(fake.calls().save.count == 1, "nothing was saved")
  assert(fake.take_choice(SAVE_LOAD) == CHOICE_CANCEL, "the screen stayed")
  fake.release(SAVE_LOAD)
  finish()
end)

test("the list of saves opens on the slot it was left on", function()
  local SAVE_LOAD = trx.ui.Screen.SAVE_LOAD
  trx.config.set("ui.menu_system", "flat")
  assert(fake.offer(SAVE_LOAD, { mode = MenuMode.SAVE }), "no list opened")
  ticks(2)
  press(Role.MENU_DOWN)
  press(Role.MENU_DOWN)
  press(Role.MENU_BACK)
  fake.release(SAVE_LOAD)
  fake.take_choice(SAVE_LOAD)
  assert(fake.offer(SAVE_LOAD, { mode = MenuMode.SAVE }), "no list opened")
  ticks(2)
  press(Role.MENU_CONFIRM)
  assert(fake.calls().save.index == 3, "the list did not keep its slot")
  fake.release(SAVE_LOAD)
  finish()
end)

test("the list of saves loads a quick save", function()
  local SAVE_LOAD = trx.ui.Screen.SAVE_LOAD
  trx.config.set("ui.menu_system", "flat")
  assert(fake.offer(SAVE_LOAD, { mode = MenuMode.LOAD }), "no list opened")
  ticks(2)
  local listed = false
  for _, op in ipairs(fake.scene()) do
    listed = listed or op:find("QS ", 1, true) ~= nil
  end
  -- The quick saves come first, at the top of the list.
  trx.config.set("ui.enable_wraparound", false)
  for _ = 1, 100 do
    press(Role.MENU_UP)
  end
  press(Role.MENU_CONFIRM)
  local load = fake.calls().load
  fake.release(SAVE_LOAD)
  finish()
  assert(listed, "no quick save was listed")
  assert(load.count == 1, "nothing was loaded")
  assert(load.pool == trx.savegame.Pool.QUICK, "the quick save was not loaded")
end)

test("the list of saves saves over no quick save", function()
  local SAVE_LOAD = trx.ui.Screen.SAVE_LOAD
  trx.config.set("ui.menu_system", "flat")
  trx.config.set("ui.enable_wraparound", false)
  assert(fake.offer(SAVE_LOAD, { mode = MenuMode.SAVE }), "no list opened")
  ticks(2)
  for _ = 1, 100 do
    press(Role.MENU_UP)
  end
  press(Role.MENU_CONFIRM)
  assert(fake.calls().save.count == 0, "a quick save was saved over")
  press(Role.MENU_BACK)
  fake.release(SAVE_LOAD)
  finish()
end)

test("the ring draws the menu unless the flat menu is picked", function()
  carry()
  trx.config.set("ui.menu_system", "ring")
  assert(fake.offer(MENU, { mode = MenuMode.GAME }), "no menu opened")
  ticks(SETTLE_TICKS)
  local heading = require("common.ui.ring").signals.heading:get()
  assert(heading ~= nil, "the ring did not open")
  finish()
end)

test("the flat menu system draws no frames", function()
  trx.config.set("ui.menu_system", "flat")
  assert(not trx.ui.frames_shown, "the frames still show")
  trx.config.set("ui.menu_system", "ring")
  assert(trx.ui.frames_shown, "the frames did not come back")
end)

test("a change of the menu system waits for the open menu to close", function()
  carry()
  trx.config.set("ui.menu_system", "ring")
  assert(fake.offer(MENU, { mode = MenuMode.GAME }), "no menu opened")
  ticks(SETTLE_TICKS)
  trx.config.set("ui.menu_system", "flat")
  assert(trx.ui.frames_shown, "the open ring lost its frames")
  fake.release(MENU)
  fake.take_choice(MENU)
  assert(not trx.ui.frames_shown, "the change did not follow the ring closing")
  finish()
end)

return h.report()
