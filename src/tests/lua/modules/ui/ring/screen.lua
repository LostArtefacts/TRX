-- The shipped inventory ring: how it opens, turns and closes, and what it does
-- with the entry the player picks.

local h = require("harness")
local test = h.test

local entries = require("common.ui.ring.entries")
local ring = require("common.ui.ring")
require("common.ui.menu").setup()
local objects = trx.catalog.objects
local Role = trx.input.Role
local MENU = trx.ui.Screen.MENU
local MenuMode = trx.ui.MenuMode
local CHOICE_NONE = 0
local CHOICE_CONFIRM = 2

-- Long enough for any motion of the ring to end.
local SETTLE_TICKS = 40

local function declare(object, inv_pos, extra)
  local spec = {
    object_id = trx.catalog.key(trx.catalog.Context.OBJECTS, object),
    inv_pos = inv_pos,
  }
  for key, value in pairs(extra or {}) do
    spec[key] = value
  end
  entries.declare(spec)
end

declare(objects.COMPASS_OPTION, 0)
declare(
  objects.SMALL_MEDIPACK_OPTION,
  14,
  { frames_total = 4, open_frame = 3 }
)
declare(objects.LARGE_MEDIPACK_OPTION, 15)
declare(objects.LEAD_BAR_OPTION, 16)
declare(objects.KEY_OPTION_1, 110)
declare(objects.KEY_OPTION_2, 111)
declare(objects.KEY_OPTION_3, 112)

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

local function plays()
  local play = fake.calls().play
  return play ~= nil and play.count or 0
end

local function open(mode, object)
  trx.config.set("ui.text_scale", 1)
  assert(fake.offer(MENU, { mode = mode, object = object }), "no ring opened")
  ticks(SETTLE_TICKS)
end

local function finish()
  fake.release(MENU)
  fake.take_choice(MENU)
  fake.release_all()
  assert(trx.ui.layers.count() == 0, "a layer outlived the ring")
  assert(fake.errors() == 0, "the ring raised")
end

local function carry()
  trx.inventory.give(objects.COMPASS_OPTION)
  trx.inventory.give(objects.SMALL_MEDIPACK_OPTION, 2)
  trx.inventory.give(objects.LARGE_MEDIPACK_OPTION)
end

test("the ring draws an entry for each thing Lara carries", function()
  carry()
  open(MenuMode.GAME)
  assert(models() == 3, ("the ring drew %d entries"):format(models()))
  finish()
end)

test("back closes the ring and uses nothing", function()
  carry()
  open(MenuMode.GAME)
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the ring stayed open")
  assert(fake.calls().use_item.count == 0, "something was used")
  finish()
end)

test("picking a medipack uses it once the ring closes", function()
  carry()
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  local before = plays()
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the ring stayed open")
  local used = fake.calls().use_item
  assert(used.count == 1, "nothing was used")
  assert(used.object_id == objects.SMALL_MEDIPACK_OPTION, "the wrong one")
  -- The choice is heard, and the ring closes after it without a sound.
  assert(plays() == before + 1, "the ring was heard closing")
  finish()
end)

test("picking a plain item from the main ring uses it at once", function()
  local lead_bar = trx.objects[objects.LEAD_BAR_OPTION]
  lead_bar:add_family("generic_inv_option")
  carry()
  trx.inventory.give(objects.LEAD_BAR_OPTION)
  open(MenuMode.GAME, objects.LEAD_BAR_OPTION)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "a second press was needed")
  local used = fake.calls().use_item
  assert(used.count == 1, "nothing was used")
  assert(used.object_id == objects.LEAD_BAR_OPTION, "the wrong one")
  trx.inventory.set_count(objects.LEAD_BAR_OPTION, 0)
  lead_bar:remove_family("generic_inv_option")
  finish()
end)

test("an entry declared to be used at once is used on one press", function()
  declare(objects.FLARES_BOX_OPTION, 12, { is_used_at_once = true })
  carry()
  trx.inventory.give(objects.FLARES_BOX_OPTION)
  open(MenuMode.GAME, objects.FLARES_BOX_OPTION)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "a second press was needed")
  local used = fake.calls().use_item
  assert(used.count == 1, "nothing was used")
  assert(used.object_id == objects.FLARES_BOX_OPTION, "the wrong one")
  trx.inventory.set_count(objects.FLARES_BOX_OPTION, 0)
  finish()
end)

test("a box of flares is opened before it is used", function()
  declare(objects.FLARES_BOX_OPTION, 12, { is_used_at_once = false })
  carry()
  trx.inventory.give(objects.FLARES_BOX_OPTION)
  open(MenuMode.GAME, objects.FLARES_BOX_OPTION)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(MENU) == CHOICE_NONE, "one press used the flares")
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.take_choice(MENU) == CHOICE_CONFIRM, "the ring stayed open")
  local used = fake.calls().use_item
  assert(used.count == 1, "nothing was used")
  assert(used.object_id == objects.FLARES_BOX_OPTION, "the wrong one")
  trx.inventory.set_count(objects.FLARES_BOX_OPTION, 0)
  finish()
end)

test("a screen open on an entry keeps the hints it shows", function()
  local hint = { { input = Role.MENU_SHOW_INFO, text = "Rotate" } }
  local ring = require("common.ui.ring")
  trx.ui.screens.define(trx.ui.Screen.EXAMINE, function(ctx)
    ring.set_hint(hint)
    return ctx:push({
      root = trx.ui.widgets.Spacer(),
      on_input = function() end,
    })
  end)
  fake.set_examinable(true)
  carry()
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  press(Role.MENU_SHOW_INFO)
  ticks(SETTLE_TICKS)
  local shown = ring.signals.hint:get()
  fake.set_examinable(false)
  assert(#shown == 1, "the ring took the screen's hints away")
  finish()
end)

test("the controls item opens its screen whatever model it shows", function()
  require("common.ui.controls_model").setup()
  local model =
    trx.catalog.mint(trx.catalog.Context.OBJECTS, "controls_option_keys")
  declare(objects.CONTROLS_OPTION, 203)
  declare(model, 203)
  fake.set_loaded(model, true)
  trx.config.set("visuals.controls_model", "keyboard")
  local opened = false
  trx.ui.screens.define(trx.ui.Screen.RING_ENTRY, function()
    opened = true
  end, { object = objects.CONTROLS_OPTION })
  open(MenuMode.TITLE, objects.CONTROLS_OPTION)
  local wanted = ("^world_model obj=%d "):format(model)
  local drawn = false
  for _, op in ipairs(fake.scene()) do
    drawn = drawn or op:match(wanted) ~= nil
  end
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  trx.config.set("visuals.controls_model", "original")
  assert(drawn, "the ring did not draw the chosen model")
  assert(opened, "the controls screen did not open")
  finish()
end)

test("the controls item swaps its model while the ring rests", function()
  local keys = trx.catalog.objects.controls_option_keys
  local saturn =
    trx.catalog.mint(trx.catalog.Context.OBJECTS, "controls_option_saturn")
  declare(saturn, 203)
  fake.set_loaded(keys, true)
  fake.set_loaded(saturn, true)
  trx.config.set("visuals.controls_model", "keyboard")
  open(MenuMode.TITLE, objects.CONTROLS_OPTION)
  local function draws(object)
    local wanted = ("^world_model obj=%d "):format(object)
    for _, op in ipairs(fake.scene()) do
      if op:match(wanted) then
        return true
      end
    end
    return false
  end
  local before = models()
  assert(draws(keys), "the ring did not draw the keyboard")
  trx.config.set("visuals.controls_model", "saturn")
  ticks(1)
  local swapped = draws(saturn) and not draws(keys)
  local after = models()
  trx.config.set("visuals.controls_model", "original")
  assert(swapped, "the ring kept the old model")
  assert(after == before, "the ring lost entries")
  finish()
end)

test(
  "the controls item follows the controls while its dialog is open",
  function()
    local keys = trx.catalog.objects.controls_option_keys
    local saturn = trx.catalog.objects.controls_option_saturn
    fake.set_loaded(keys, true)
    fake.set_loaded(saturn, true)
    trx.config.set("visuals.controls_model", "auto")
    trx.config.set("input.backend", "keyboard")
    trx.ui.screens.define(trx.ui.Screen.RING_ENTRY, function(ctx)
      return ctx:push({
        root = trx.ui.widgets.Spacer(),
        on_input = function() end,
      })
    end, { object = objects.CONTROLS_OPTION, override = true })
    open(MenuMode.TITLE, objects.CONTROLS_OPTION)
    press(Role.MENU_CONFIRM)
    ticks(SETTLE_TICKS)
    local function draws(object)
      local wanted = ("^world_model obj=%d "):format(object)
      for _, op in ipairs(fake.scene()) do
        if op:match(wanted) then
          return true
        end
      end
      return false
    end
    local before = draws(keys)
    trx.config.set("input.backend", "controller")
    ticks(1)
    local after = draws(saturn) and not draws(keys)
    trx.config.set("input.backend", "keyboard")
    trx.config.set("visuals.controls_model", "original")
    assert(before, "the dialog did not open over the keyboard")
    assert(after, "the model waited for the dialog to close")
    finish()
  end
)

test("a picked entry holds still until its animation runs", function()
  carry()
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  fake.press(Role.MENU_CONFIRM)
  fake.tick()
  fake.release_all()
  local wanted = ("^world_model obj=%d "):format(objects.SMALL_MEDIPACK_OPTION)
  for _ = 1, SETTLE_TICKS do
    for _, op in ipairs(fake.scene()) do
      if op:match(wanted) then
        assert(
          not op:match(" frame=0 next=1 "),
          "the entry leaned into a frame it had not reached"
        )
      end
    end
    fake.tick()
  end
  finish()
end)

test("turning the ring rests it on another entry", function()
  carry()
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  fake.press(Role.MENU_RIGHT)
  fake.tick()
  fake.release_all()
  ticks(SETTLE_TICKS)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  local used = fake.calls().use_item
  assert(used.count == 0 or used.object_id ~= objects.SMALL_MEDIPACK_OPTION)
  finish()
end)

test("the ring goes dark behind combining only with the frame", function()
  require("common.ui.screens.combine").setup()
  local partner = objects.LARGE_MEDIPACK_OPTION
  -- The frame draws the partner as a model of its own, apart from the ring.
  local partner_model = ("^model obj=%d "):format(partner)
  fake.set_combine_partners({ partner })
  carry()
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  press(Role.MENU_CONFIRM)
  local framed = false
  for _ = 1, SETTLE_TICKS do
    local dark = false
    local named = false
    for _, op in ipairs(fake.scene()) do
      dark = dark or op:match("^darken ") ~= nil
      named = named or op:match(partner_model) ~= nil
    end
    assert(not dark or named, "the ring went dark before the frame showed")
    framed = framed or (dark and named)
    fake.tick()
  end
  assert(framed, "the frame never showed")
  fake.set_combine_partners({})
  press(Role.MENU_BACK)
  ticks(SETTLE_TICKS)
  finish()
end)

test("Lara's Home fades the title to black before the gym loads", function()
  declare(objects.PHOTO_OPTION, 205)
  fake.set_gym_present(true)
  open(MenuMode.TITLE, objects.PHOTO_OPTION)
  -- The fade draws as a black quad over the whole screen. It is read after
  -- every tick, since the gym loads on the tick the screen turns black.
  local darkest = 0
  local function tick()
    fake.tick()
    for _, op in ipairs(fake.scene()) do
      local alpha = op:match("^quad x=0 y=0 .* color=000000(%x%x)$")
      if alpha ~= nil then
        darkest = math.max(darkest, tonumber(alpha, 16))
      end
    end
    return fake.calls().play_gym.count > 0
  end
  for _ = 1, SETTLE_TICKS * 2 do
    fake.release_all()
    fake.press(Role.MENU_CONFIRM)
    if tick() then
      break
    end
    fake.release_all()
    if tick() then
      break
    end
  end
  assert(fake.calls().play_gym.count == 1, "the gym was not asked for")
  assert(darkest >= 0xC0, "the title did not fade to black before the gym")
  fake.set_gym_present(false)
  finish()
end)

test("the keys ring opens on the entry asked for", function()
  trx.inventory.give(objects.KEY_OPTION_1)
  trx.inventory.give(objects.KEY_OPTION_2)
  open(MenuMode.KEYS, objects.KEY_OPTION_2)
  assert(models() == 2, "the keys ring drew the wrong entries")
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(fake.calls().use_item.object_id == objects.KEY_OPTION_2)
  finish()
end)

-- The objects the ring draws, each once.
local function drawn()
  local result = {}
  for _, op in ipairs(fake.scene()) do
    local object = op:match("^world_model obj=(%d+) ")
    if object ~= nil then
      result[tonumber(object)] = true
    end
  end
  return result
end

local function same_set(set, list)
  local count = 0
  for _ in pairs(set) do
    count = count + 1
  end
  if count ~= #list then
    return false
  end
  for _, object in ipairs(list) do
    if not set[object] then
      return false
    end
  end
  return true
end

-- Uses up a key at a keyhole: picks it on a keys ring opened on it and takes
-- it away, as the keyhole does.
local function use_up(object)
  open(MenuMode.KEYS, object)
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  finish()
  trx.inventory.set_count(object, 0)
end

-- Opens the ring and moves up to the keys ring.
local function open_keys_from_main()
  carry()
  open(MenuMode.GAME)
  press(Role.MENU_UP)
  ticks(SETTLE_TICKS)
end

-- Picks the entry the ring rests on and returns what that uses.
local function pick_and_close()
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  local used = fake.calls().use_item.object_id
  finish()
  return used
end

local function with_glitch(fixed, run)
  trx.config.set("gameplay.fix_item_duplication_glitch", fixed)
  run()
  trx.config.set("gameplay.fix_item_duplication_glitch", true)
  trx.inventory.set_count(objects.KEY_OPTION_1, 0)
  trx.inventory.set_count(objects.KEY_OPTION_2, 0)
end

test("a key used up stays usable through the keys ring", function()
  with_glitch(false, function()
    trx.inventory.give(objects.KEY_OPTION_1)
    trx.inventory.give(objects.KEY_OPTION_2)
    use_up(objects.KEY_OPTION_2)
    for _ = 1, 2 do
      open_keys_from_main()
      assert(
        same_set(drawn(), { objects.KEY_OPTION_1 }),
        "the keys ring drew a key Lara no longer carries"
      )
      assert(ring.selection() == objects.KEY_OPTION_1, "the wrong key rests")
      assert(
        pick_and_close() == objects.KEY_OPTION_2,
        "the used up key was not used again"
      )
    end
  end)
end)

test("a key used up stays used up with the glitch fixed", function()
  with_glitch(true, function()
    trx.inventory.give(objects.KEY_OPTION_1)
    trx.inventory.give(objects.KEY_OPTION_2)
    use_up(objects.KEY_OPTION_2)
    for _ = 1, 2 do
      open_keys_from_main()
      assert(
        same_set(drawn(), { objects.KEY_OPTION_1 }),
        "the keys ring drew a key Lara no longer carries"
      )
      assert(ring.selection() == objects.KEY_OPTION_1, "the wrong key rests")
      assert(
        pick_and_close() == objects.KEY_OPTION_1,
        "the used up key was used"
      )
    end
  end)
end)

test("a key used up between two others is not used again", function()
  with_glitch(false, function()
    trx.inventory.give(objects.KEY_OPTION_1)
    trx.inventory.give(objects.KEY_OPTION_2)
    trx.inventory.give(objects.KEY_OPTION_3)
    use_up(objects.KEY_OPTION_2)
    open_keys_from_main()
    assert(
      same_set(drawn(), { objects.KEY_OPTION_1, objects.KEY_OPTION_3 }),
      "the keys ring drew the wrong keys"
    )
    assert(ring.selection() == objects.KEY_OPTION_3, "the wrong key rests")
    assert(
      pick_and_close() == objects.KEY_OPTION_3,
      "the used up key was used"
    )
  end)
end)

test("the last key used up leaves no keys ring to move to", function()
  with_glitch(false, function()
    trx.inventory.give(objects.KEY_OPTION_1)
    use_up(objects.KEY_OPTION_1)
    local before = fake.calls().use_item.count
    open_keys_from_main()
    local shown = drawn()
    assert(not shown[objects.KEY_OPTION_1], "the used up key was drawn")
    assert(
      ring.selection() ~= objects.KEY_OPTION_1,
      "the ring rests on the used up key"
    )
    press(Role.MENU_BACK)
    ticks(SETTLE_TICKS)
    assert(fake.calls().use_item.count == before, "something was used")
    finish()
  end)
end)

test("turning the keys ring loses the used up key", function()
  with_glitch(false, function()
    trx.inventory.give(objects.KEY_OPTION_1)
    trx.inventory.give(objects.KEY_OPTION_2)
    trx.inventory.give(objects.KEY_OPTION_3)
    use_up(objects.KEY_OPTION_3)
    open_keys_from_main()
    assert(
      same_set(drawn(), { objects.KEY_OPTION_1, objects.KEY_OPTION_2 }),
      "the keys ring drew the wrong keys"
    )
    assert(ring.selection() == objects.KEY_OPTION_1, "the wrong key rests")
    press(Role.MENU_RIGHT)
    ticks(SETTLE_TICKS)
    press(Role.MENU_LEFT)
    ticks(SETTLE_TICKS)
    assert(ring.selection() == objects.KEY_OPTION_1, "the ring did not return")
    assert(
      pick_and_close() == objects.KEY_OPTION_1,
      "the used up key was used"
    )
  end)
end)

test("a keys ring opened at a keyhole loses the used up key", function()
  with_glitch(false, function()
    trx.inventory.give(objects.KEY_OPTION_1)
    trx.inventory.give(objects.KEY_OPTION_2)
    use_up(objects.KEY_OPTION_2)
    open(MenuMode.KEYS)
    assert(ring.selection() == objects.KEY_OPTION_1, "the wrong key rests")
    assert(
      pick_and_close() == objects.KEY_OPTION_1,
      "the used up key was used"
    )
    open_keys_from_main()
    assert(
      pick_and_close() == objects.KEY_OPTION_1,
      "the used up key came back"
    )
  end)
end)

test("the title fades in with the inventory fade effects off", function()
  declare(objects.PHOTO_OPTION, 205)
  fake.set_gym_present(true)
  trx.config.set("ui.inventory_fade_effects", false)
  trx.config.set("ui.text_scale", 1)
  assert(fake.offer(MENU, { mode = MenuMode.TITLE }), "no ring opened")
  -- The fade draws as a black quad over the whole screen.
  local partial = false
  for _ = 1, SETTLE_TICKS do
    fake.tick()
    for _, op in ipairs(fake.scene()) do
      local alpha = op:match("^quad x=0 y=0 .* color=000000(%x%x)$")
      if alpha ~= nil then
        local value = tonumber(alpha, 16)
        partial = partial or (value > 0x10 and value < 0xF0)
      end
    end
  end
  assert(partial, "the title cut in rather than fading in")
  trx.config.set("ui.inventory_fade_effects", true)
  fake.set_gym_present(false)
  finish()
end)

test("a keys ring with nothing on it does not open", function()
  assert(not fake.offer(MENU, { mode = MenuMode.KEYS }), "it opened")
  assert(fake.take_choice(MENU) == CHOICE_NONE)
  finish()
end)

test("a ring that the engine ends early fades to black", function()
  carry()
  open(MenuMode.GAME)
  fake.release(MENU)
  assert(fake.holds_screen(), "the next screen did not wait for a fade")
  ticks(SETTLE_TICKS)
  assert(not fake.holds_screen(), "the fade never let go of the screen")
  finish()
end)

test("the ring goes on moving as the game exits", function()
  carry()
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  press(Role.MENU_CONFIRM)
  fake.exit()
  local before = table.concat(fake.scene(), "\n")
  ticks(SETTLE_TICKS)
  assert(table.concat(fake.scene(), "\n") ~= before, "the ring stopped")
  assert(fake.take_choice(MENU) == CHOICE_NONE, "the ring finished")
  assert(fake.calls().use_item.count == 0, "something was used")
  finish()
end)

test("the ring ignores the player as the game exits", function()
  carry()
  open(MenuMode.GAME, objects.SMALL_MEDIPACK_OPTION)
  fake.exit()
  local before = plays()
  press(Role.MENU_CONFIRM)
  ticks(SETTLE_TICKS)
  assert(plays() == before, "the ring took the press")
  finish()
end)

test("a ring that the game ends as it exits fades no further", function()
  carry()
  open(MenuMode.GAME)
  fake.exit()
  ticks(SETTLE_TICKS)
  assert(not fake.holds_screen(), "the exit fade never let go of the screen")
  fake.release(MENU)
  assert(not fake.holds_screen(), "the ring faded to black again")
  finish()
end)

test("a ring told to exit the game closes before the game exits", function()
  carry()
  open(MenuMode.GAME)
  ring.exit_game()
  ticks(SETTLE_TICKS)
  assert(
    fake.calls().exit_game.count == 0,
    "the game exited with the ring open"
  )
  press(Role.MENU_BACK)
  assert(
    fake.calls().exit_game.count == 0,
    "the game exited as the ring closed"
  )
  ticks(SETTLE_TICKS)
  assert(fake.calls().exit_game.count == 1, "the game did not exit")
  assert(fake.take_choice(MENU) == CHOICE_NONE, "the ring finished")
  fake.exit()
  ticks(SETTLE_TICKS)
  fake.release(MENU)
  assert(not fake.holds_screen(), "the ring faded to black again")
  finish()
end)

return h.report()
