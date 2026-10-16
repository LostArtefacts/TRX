-- The shipped settings screens: the graphics, sound and gameplay settings
-- entries of the inventory ring.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local RING_ENTRY = trx.ui.Screen.RING_ENTRY
local SOUND = trx.catalog.objects.SOUND_OPTION
local CHOICE_CANCEL = 1

require("common.ui.screens.settings").setup()

local function press(role)
  fake.release_all()
  fake.press(role)
  fake.tick()
  fake.release_all()
  fake.tick()
end

local function open(object)
  trx.config.set("ui.text_scale", 1.0)
  h.open_ring(trx.ui.MenuMode.GAME, object, 0, 10)
  assert(
    fake.offer(RING_ENTRY, { object = object }),
    "the entry was not taken"
  )
  fake.tick()
  fake.tick()
end

-- Puts the entry away. The level ends with it unless `keep` is set, so that
-- the next dialog opens from the top.
local function finish(keep)
  fake.release(RING_ENTRY)
  fake.take_choice(RING_ENTRY)
  h.close_ring()
  fake.release_all()
  if not keep then
    fake.end_level()
  end
  assert(fake.errors() == 0, "the module raised")
end

test("every settings entry is taken", function()
  for _, object in ipairs({
    trx.catalog.objects.GRAPHICS_OPTION,
    trx.catalog.objects.SOUND_OPTION,
    trx.catalog.objects.PDA_OPTION,
  }) do
    open(object)
    finish()
  end
end)

test("the player changes a setting and backs out", function()
  trx.config.set("audio.master_volume", 50)
  open(SOUND)
  press(Role.MENU_DOWN)
  press(Role.MENU_RIGHT)
  assert(math.abs(trx.config.get("audio.master_volume") - 0.6) < 0.001)
  press(Role.MENU_BACK)
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CANCEL)
  local anim = require("common.ui.ring").selection_anim()
  assert(anim.goal_frame == 0 and anim.direction == -1, "not put away")
  finish()
end)

test("a declared setting takes its row in its tab", function()
  trx.config.declare({
    key = "mod.echo",
    kind = "boolean",
    default = false,
    ui = { tab = "sound_volume", after = "audio.master_volume" },
  })
  open(SOUND)
  press(Role.MENU_DOWN)
  press(Role.MENU_DOWN)
  press(Role.MENU_RIGHT)
  assert(trx.config.get("mod.echo") == true)
  finish()
end)

test("a row on a tab that does not exist is left out", function()
  trx.config.declare({
    key = "mod.nowhere",
    kind = "boolean",
    default = false,
    ui = { tab = "nowhere" },
  })
  open(SOUND)
  finish()
end)

test("a closed dialog leaves nothing following the game", function()
  local function listeners_of(signal)
    local count = 0
    for _ in pairs(rawget(signal, "_listeners")) do
      count = count + 1
    end
    return count
  end
  local changed = listeners_of(trx.config.signals.changed)
  local layout = listeners_of(trx.ui.signals.layout)
  for _ = 1, 3 do
    open(trx.catalog.objects.PDA_OPTION)
    press(Role.MENU_DOWN)
    finish()
  end
  assert(listeners_of(trx.config.signals.changed) == changed)
  assert(listeners_of(trx.ui.signals.layout) == layout)
end)

-- The strings load nothing here, so declaring the text as the dialog shows
-- stands in for the player picking another language.
test("the heading and the tabs follow the player's language", function()
  open(SOUND)
  local function shows(wanted)
    for _, op in ipairs(fake.scene()) do
      if op:find(wanted, 1, true) then
        return true
      end
    end
    return false
  end
  assert(not shows("Noises") and not shows("Loudness"))
  trx.locale.declare({
    ["general/settings/sound/title"] = "Noises",
    ["general/settings/sound/tabs/volume"] = "Loudness",
  })
  trx.config.set("audio.master_volume", 40)
  fake.tick()
  assert(shows("Noises"), "the heading kept its old text")
  assert(shows("Loudness"), "the tab kept its old name")
  finish()
end)

test("a dialog opens again where the player left it", function()
  local PDA = trx.catalog.objects.PDA_OPTION
  trx.config.set("gameplay.enable_legal", true)
  trx.config.set("gameplay.enable_credits", true)
  open(PDA)
  press(Role.MENU_DOWN)
  press(Role.MENU_DOWN)
  finish(true)

  open(PDA)
  press(Role.MENU_RIGHT)
  assert(trx.config.get("gameplay.enable_credits") == false, "not on credits")
  assert(trx.config.get("gameplay.enable_legal") == true)
  finish()

  -- The level ended with the entry, which starts every dialog from the top.
  open(PDA)
  press(Role.MENU_DOWN)
  press(Role.MENU_RIGHT)
  assert(trx.config.get("gameplay.enable_legal") == false, "not on legal")
  finish()
end)

return h.report()
