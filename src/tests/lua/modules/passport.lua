-- The shipped passport module, run against a faked ring and faked saves: which
-- page the book opens on, how it turns, what a choice asks of the game, and
-- what the scene draws.

local h = require("harness")
local test = h.test

local Mode = trx.inventory_ring.Mode
local Pool = trx.savegame.Pool
local Role = trx.input.Role
local RING_ENTRY = trx.ui.Screen.RING_ENTRY
local SAVE_LOAD = trx.ui.Screen.SAVE_LOAD
local PASSPORT = trx.catalog.objects.PASSPORT_OPTION
local CRYSTAL = trx.catalog.objects.SAVE_CRYSTAL_OPTION
local CRYSTAL_ITEM = trx.catalog.objects.SAVE_CRYSTAL_ITEM
local CHOICE_CANCEL = 1
local CHOICE_CONFIRM = 2

-- The book opens on frame 10, and each page sits five frames further on.
local OPEN_FRAME = 10
local FRAME_COUNT = 30

-- The strings files are not loaded, so the caption and count formats fall
-- back to the shipped ones.
trx.locale.declare({
  ["general/inventory_ring/object_name_fmt"] = "%s",
  ["general/inventory_ring/item_count_fmt"] = "\\{small}%s",
})

require("common.passport").setup()

-- One tick of the game: the scripts read their input, and then the ring runs
-- the book to the frame they asked for.
local function tick()
  fake.tick()
  fake.settle_ring()
end

local function press(role)
  fake.release_all()
  fake.press(role)
  tick()
  fake.release_all()
end

local L = trx.locale.get

-- Takes down whatever an earlier case left, so that one failure does not
-- fail every case after it.
local function clean()
  fake.release(RING_ENTRY)
  fake.take_choice(RING_ENTRY)
  fake.release(SAVE_LOAD)
  fake.take_choice(SAVE_LOAD)
  fake.close_ring()
  fake.release_all()
end

local function open(mode, tr_version)
  clean()
  fake.set_tr_version(tr_version or 2)
  fake.open_ring(mode, PASSPORT, OPEN_FRAME, FRAME_COUNT)
  assert(fake.offer(RING_ENTRY, PASSPORT), "the passport was not taken")
  -- The press that opened the book.
  tick()
end

local function finish()
  clean()
  fake.set_current_level(nil)
  assert(trx.ui.layers.count() == 0, "a layer outlived the book")
  assert(fake.errors() == 0, "the module raised")
end

local function texts()
  local result = {}
  for _, op in ipairs(fake.scene()) do
    local text = op:match("^text .- text=(.*)$")
    if text ~= nil then
      result[#result + 1] = text
    end
  end
  return result
end

local function count_of(list, wanted)
  local count = 0
  for _, text in ipairs(list) do
    if text == wanted then
      count = count + 1
    end
  end
  return count
end

local function has(list, wanted)
  return count_of(list, wanted) > 0
end

test("the title opens on the save list", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  local drawn = texts()
  -- Once as the page's heading, and once as the caption at the foot.
  assert(
    count_of(drawn, L("general/passport/load_game")) == 2,
    table.concat(drawn, ",")
  )
  finish()
end)

test("the caption arrows say which way the book turns", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  local drawn = texts()
  assert(not has(drawn, "\\{button left}"), "the first page turned left")
  assert(has(drawn, "\\{button right}"))
  finish()
end)

test("the book turns to the next page", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  press(Role.MENU_RIGHT)
  -- The press picks the page, and the book starts to turn on the next tick.
  fake.tick()
  assert(trx.inventory_ring.selection_anim().goal_frame == OPEN_FRAME + 5)
  fake.settle_ring()
  tick()
  assert(has(texts(), L("general/passport/new_game")))
  finish()
end)

test("the caption stands down while the book turns", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  assert(has(texts(), L("general/passport/load_game")))
  press(Role.MENU_RIGHT)
  -- The frame drawn right after the press, before the book starts to turn.
  local drawn = texts()
  assert(
    not has(drawn, L("general/passport/new_game")),
    "the caption named the next page as the press landed"
  )
  -- The book starts to turn and has not reached the next page yet.
  fake.tick()
  drawn = texts()
  assert(
    not has(drawn, L("general/passport/new_game")),
    "the caption named a page the book has not reached"
  )
  assert(
    not has(drawn, L("general/passport/load_game")),
    "the caption kept the page the book left"
  )
  assert(not has(drawn, "\\{button left}"))
  fake.settle_ring()
  tick()
  assert(
    has(texts(), L("general/passport/new_game")),
    "the caption did not return"
  )
  finish()
end)

test("TR1 shows only the page name until the player opens it", function()
  fake.set_current_level(nil)
  open(Mode.TITLE, 1)
  tick()
  local drawn = texts()
  assert(
    count_of(drawn, L("general/passport/load_game")) == 1,
    "the list opened unasked"
  )
  press(Role.MENU_CONFIRM)
  tick()
  assert(
    count_of(texts(), L("general/passport/load_game")) == 2,
    "the list did not open"
  )
  finish()
end)

test("loading a save asks the game for it and puts the book away", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  press(Role.MENU_CONFIRM)
  local calls = fake.calls()
  assert(calls.load.count == 1, "nothing was loaded")
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CONFIRM)
  finish()
end)

-- Turns from the first page to the next one and lets the book settle.
local function turn_right()
  press(Role.MENU_RIGHT)
  fake.tick()
  fake.settle_ring()
  tick()
end

test("saving writes the slot and leaves the ring", function()
  fake.set_current_level(2)
  open(Mode.GAME)
  tick()
  turn_right()
  press(Role.MENU_CONFIRM)
  local calls = fake.calls()
  assert(calls.save.count == 1, "nothing was saved")
  assert(calls.save.pool == Pool.NORMAL, "the save went to a quick slot")
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CONFIRM)
  finish()
end)

test("a quick save is not a slot to save to", function()
  fake.set_current_level(2)
  open(Mode.GAME)
  tick()
  turn_right()
  -- The cursor rests on the first numbered slot, under the quick saves.
  press(Role.MENU_UP)
  press(Role.MENU_CONFIRM)
  assert(fake.calls().save.count == 0, "a quick save was overwritten")
  assert(fake.is_held(RING_ENTRY), "the book closed")
  finish()
end)

test("TR1 does not turn the page while a page is open", function()
  fake.set_current_level(nil)
  open(Mode.TITLE, 1)
  tick()
  press(Role.MENU_CONFIRM)
  tick()
  press(Role.MENU_RIGHT)
  fake.tick()
  assert(
    trx.inventory_ring.selection_anim().goal_frame == OPEN_FRAME,
    "the book turned"
  )
  assert(
    count_of(texts(), L("general/passport/load_game")) == 2,
    "the list closed"
  )
  finish()
end)

test("TR1 backs out of a list to the pages when Lara is dead", function()
  fake.set_current_level(2)
  open(Mode.DEATH, 1)
  tick()
  press(Role.MENU_CONFIRM)
  tick()
  press(Role.MENU_BACK)
  tick()
  assert(fake.is_held(RING_ENTRY), "the book closed with Lara dead")
  assert(
    count_of(texts(), L("general/passport/load_game")) == 1,
    "the list stayed open"
  )
  press(Role.MENU_RIGHT)
  fake.tick()
  assert(
    trx.inventory_ring.selection_anim().goal_frame == OPEN_FRAME + 5,
    "the pages did not turn"
  )
  finish()
end)

test("TR1 restarts the level with one confirm when Lara is dead", function()
  fake.set_current_level(2)
  open(Mode.DEATH, 1)
  tick()
  turn_right()
  press(Role.MENU_CONFIRM)
  assert(fake.calls().restart_level.count == 1, "the level did not restart")
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CONFIRM)
  finish()
end)

test(
  "TR1 starts a game with one confirm where it is the only choice",
  function()
    fake.set_current_level(nil)
    local policy = trx.config.get("gameplay.game_modes_policy")
    local play_prev = trx.config.get("gameplay.enable_play_previous_levels")
    trx.config.set("gameplay.game_modes_policy", "never")
    trx.config.set("gameplay.enable_play_previous_levels", false)
    open(Mode.TITLE, 1)
    tick()
    turn_right()
    press(Role.MENU_CONFIRM)
    tick()
    trx.config.set("gameplay.game_modes_policy", policy)
    trx.config.set("gameplay.enable_play_previous_levels", play_prev)
    assert(fake.calls().new_game.count == 1, "the game did not start")
    assert(fake.take_choice(RING_ENTRY) == CHOICE_CONFIRM)
    finish()
  end
)

test("the back key puts the book away", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  press(Role.MENU_BACK)
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CANCEL)
  finish()
end)

test("the book does not close when Lara is dead", function()
  fake.set_current_level(2)
  open(Mode.DEATH)
  tick()
  press(Role.MENU_BACK)
  tick()
  assert(fake.is_held(RING_ENTRY), "the book closed with Lara dead")
  finish()
end)

test("holding the delete button asks before a save goes", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  fake.hold(Role.UNBIND_KEY, true)
  for _ = 1, 45 do
    tick()
  end
  fake.hold(Role.UNBIND_KEY, false)
  tick()
  assert(
    has(texts(), L("general/passport/delete_save_confirm")),
    "the question was not asked"
  )
  -- The cursor rests on No, so the first row is a step up.
  press(Role.MENU_UP)
  press(Role.MENU_CONFIRM)
  assert(fake.calls().delete.count == 1, "the save was not deleted")
  assert(fake.is_held(RING_ENTRY), "the book closed after deleting")
  finish()
end)

test("the delete bar is the game's sleek bar", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  fake.hold(Role.UNBIND_KEY, true)
  for _ = 1, 25 do
    tick()
  end
  local back, fill
  for _, op in ipairs(fake.scene()) do
    back = back or op:match("^quad .* color=060606ff$")
    fill = fill or op:match("^quad .* color=5ab55aff$")
  end
  fake.hold(Role.UNBIND_KEY, false)
  assert(back, "the bar has no dark frame")
  assert(fill, "the bar is not filled in TR2's green")
  finish()
end)

test("a save that can go shows the delete button", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  local label = L("general/passport/delete_save") .. ": "
  local found = false
  for _, text in ipairs(texts()) do
    found = found or text:sub(1, #label) == label
  end
  assert(found, "the delete button is not on the screen")
  finish()
end)

test("the delete bar shows nothing until the hold counts", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  for _, op in ipairs(fake.scene()) do
    assert(not op:match("^quad .* color=060606ff$"), "the bar showed unheld")
  end
  finish()
end)

test("backing out of the question keeps the save", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  fake.hold(Role.UNBIND_KEY, true)
  for _ = 1, 45 do
    tick()
  end
  fake.hold(Role.UNBIND_KEY, false)
  tick()
  press(Role.MENU_BACK)
  tick()
  assert(fake.calls().delete.count == 0)
  assert(
    count_of(texts(), L("general/passport/load_game")) == 2,
    "the list did not come back"
  )
  finish()
end)

test("an engine release takes the book down", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  fake.release(RING_ENTRY)
  assert(trx.ui.layers.count() == 0)
  assert(#texts() == 0, "the book kept drawing")
  finish()
end)

test("the page draws over the regions and behind the engine", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  for _, op in ipairs(fake.scene()) do
    local z = tonumber(op:match(" z=(%-?%d+)"))
    if z ~= nil then
      assert(z > 0 and z < 4096, op)
    end
  end
  finish()
end)

test("a list opened from a list hides the one under it", function()
  fake.set_current_level(nil)
  open(Mode.TITLE)
  tick()
  press(Role.MENU_RIGHT)
  fake.tick()
  fake.settle_ring()
  tick()
  -- New Game, New Game+, then the saves to play earlier levels from.
  press(Role.MENU_DOWN)
  press(Role.MENU_DOWN)
  press(Role.MENU_CONFIRM)
  local drawn = texts()
  assert(has(drawn, "City of Vilcabamba"), "the save list did not open")
  assert(
    has(drawn, L("general/passport/play_previous_levels")),
    "the caption does not name the open list"
  )
  -- A list reads nothing on the tick it opens.
  tick()
  press(Role.MENU_CONFIRM)
  tick()
  drawn = texts()
  assert(has(drawn, "Caves"), "the level list did not open")
  assert(has(drawn, "Vilcabamba"), "the save's own level is missing")
  assert(
    not has(drawn, "City of Vilcabamba"),
    "the save list showed under the level list"
  )
  press(Role.MENU_BACK)
  drawn = texts()
  assert(has(drawn, "City of Vilcabamba"), "the save list did not come back")
  assert(not has(drawn, "Caves"))
  finish()
end)

test("the quick load screen shows the save list alone", function()
  clean()
  fake.set_current_level(2)
  assert(fake.offer(SAVE_LOAD, Mode.LOAD), "the screen was not taken")
  tick()
  local drawn = texts()
  -- The heading only: the screen's own title is the engine's.
  assert(count_of(drawn, L("general/passport/load_game")) == 1)
  assert(not has(drawn, "\\{button right}"), "the screen drew page arrows")
  press(Role.MENU_CONFIRM)
  assert(fake.calls().load.count == 1, "nothing was loaded")
  assert(fake.take_choice(SAVE_LOAD) == CHOICE_CANCEL)
  finish()
end)

test("the quick save screen saves to the slot picked", function()
  clean()
  fake.set_current_level(2)
  assert(fake.offer(SAVE_LOAD, Mode.SAVE))
  tick()
  assert(has(texts(), L("general/passport/save_game")))
  press(Role.MENU_CONFIRM)
  assert(fake.calls().save.count == 1, "nothing was saved")
  assert(fake.take_choice(SAVE_LOAD) == CHOICE_CANCEL)
  finish()
end)

test(
  "the quick save screen in the gym is left with one new game choice",
  function()
    clean()
    fake.set_current_level(1)
    local policy = trx.config.get("gameplay.game_modes_policy")
    local play_prev = trx.config.get("gameplay.enable_play_previous_levels")
    trx.config.set("gameplay.game_modes_policy", "never")
    trx.config.set("gameplay.enable_play_previous_levels", false)
    local taken = fake.offer(SAVE_LOAD, Mode.SAVE)
    tick()
    trx.config.set("gameplay.game_modes_policy", policy)
    trx.config.set("gameplay.enable_play_previous_levels", play_prev)
    assert(not taken, "the screen was taken with no list to show")
    finish()
  end
)

test("backing out of the quick screen closes it", function()
  clean()
  fake.set_current_level(2)
  fake.offer(SAVE_LOAD, Mode.LOAD)
  tick()
  press(Role.MENU_BACK)
  assert(fake.take_choice(SAVE_LOAD) == CHOICE_CANCEL)
  finish()
end)

local function open_crystal(crystal_mode, count)
  clean()
  fake.set_current_level(2)
  trx.config.set("gameplay.save_crystal_mode", crystal_mode or "save_pickup")
  trx.inventory.set_count(CRYSTAL_ITEM, count or 1)
  fake.open_ring(Mode.GAME, CRYSTAL, OPEN_FRAME, FRAME_COUNT)
  local taken = fake.offer(RING_ENTRY, CRYSTAL)
  tick()
  return taken
end

test("the save crystal shows the save list and its own name", function()
  assert(open_crystal(), "the crystal was not taken")
  local drawn = texts()
  assert(has(drawn, L("general/passport/save_game")), table.concat(drawn, ","))
  assert(
    has(drawn, "objects/save_crystal_item/name"),
    table.concat(drawn, ",")
  )
  assert(not has(drawn, "\\{button right}"), "the crystal drew page arrows")
  finish()
end)

test("the save crystal shows the count above one crystal", function()
  assert(open_crystal("save_pickup", 3))
  local count = trx.locale.format("general/inventory_ring/item_count_fmt", "3")
  assert(count:find("3", 1, true) ~= nil, "the count format lost the count")
  assert(has(texts(), count), table.concat(texts(), ","))
  finish()
end)

test("the save crystal shows no count for one crystal", function()
  assert(open_crystal())
  local count = trx.locale.format("general/inventory_ring/item_count_fmt", "1")
  assert(not has(texts(), count), table.concat(texts(), ","))
  finish()
end)

test("the save crystal saves, spends a crystal and leaves the ring", function()
  assert(open_crystal())
  press(Role.MENU_CONFIRM)
  local calls = fake.calls()
  assert(calls.save.count == 1, "nothing was saved")
  assert(calls.save.pool == Pool.NORMAL, "the save went to a quick slot")
  assert(trx.inventory.count(CRYSTAL_ITEM) == 0, "no crystal was spent")
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CONFIRM)
  finish()
end)

test("a failed crystal save keeps the crystal", function()
  assert(open_crystal())
  fake.set_save_fails(true)
  press(Role.MENU_CONFIRM)
  fake.set_save_fails(false)
  assert(trx.inventory.count(CRYSTAL_ITEM) == 1, "the crystal was lost")
  finish()
end)

test("backing out of the crystal spends nothing", function()
  assert(open_crystal())
  press(Role.MENU_BACK)
  assert(fake.calls().save.count == 0, "something was saved")
  assert(trx.inventory.count(CRYSTAL_ITEM) == 1, "the crystal was spent")
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CANCEL)
  finish()
end)

test("the crystal saves only in the save pickup mode", function()
  assert(not open_crystal("heal"), "the crystal was taken")
  finish()
end)

return h.report()
