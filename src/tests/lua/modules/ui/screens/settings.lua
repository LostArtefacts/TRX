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
  fake.open_ring(trx.inventory_ring.Mode.GAME, object, 0, 10)
  assert(
    fake.offer(RING_ENTRY, { object = object }),
    "the entry was not taken"
  )
  fake.tick()
  fake.tick()
end

local function finish()
  fake.release(RING_ENTRY)
  fake.take_choice(RING_ENTRY)
  fake.close_ring()
  fake.release_all()
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
  local anim = trx.inventory_ring.selection_anim()
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

return h.report()
