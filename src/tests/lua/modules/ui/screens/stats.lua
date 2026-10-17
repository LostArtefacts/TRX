-- The shipped statistics module: what the screen between levels lists, how the
-- totals add up, and how the compass, the stopwatch and the gym's best times
-- open and close in the inventory ring.

local h = require("harness")
local test = h.test

local Role = trx.input.Role
local STATS = trx.ui.Screen.STATS
local RING_ENTRY = trx.ui.Screen.RING_ENTRY
local COMPASS = trx.catalog.objects.COMPASS_OPTION
local STOPWATCH = trx.catalog.objects.STOPWATCH_OPTION
local L = trx.locale.get

local CHOICE_CANCEL = 1
local CHOICE_CONFIRM = 2
local FINAL = 0x10000
local BARE = 0x20000

-- The fake flow puts the gym first, so the table's second level is the first
-- one a script counts.
local CAVES = 2

require("common.ui.fade").setup()
require("common.ui.screens.stats").setup()

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
  return result
end

local function has(list, wanted)
  for _, text in ipairs(list) do
    if text == wanted then
      return true
    end
  end
  return false
end

local function finish()
  for _, screen in ipairs({ STATS, RING_ENTRY }) do
    fake.release(screen)
    fake.take_choice(screen)
  end
  h.close_ring()
  fake.release_all()
  for _, key in ipairs({
    "ui.stats.show_kills",
    "ui.stats.show_pickups",
    "ui.stats.show_secrets",
    "ui.stats.show_time_taken",
    "ui.stats.show_ammo",
    "ui.stats.show_medipacks_used",
    "ui.stats.show_distance_travelled",
    "ui.stats.show_deaths",
    "ui.stats.show_level_header",
  }) do
    trx.config.reset(key)
  end
  for _, track in ipairs({ trx.assault.Track.COURSE, trx.assault.Track.QUAD }) do
    trx.assault.stats.clear(track)
  end
  fake.set_tr_version(1)
  assert(trx.ui.layers.count() == 0, "a layer outlived the screen")
  assert(fake.errors() == 0, "the module raised")
end

local function hide_every_row()
  for _, key in ipairs({
    "ui.stats.show_kills",
    "ui.stats.show_pickups",
    "ui.stats.show_secrets",
    "ui.stats.show_time_taken",
    "ui.stats.show_ammo",
    "ui.stats.show_medipacks_used",
    "ui.stats.show_distance_travelled",
    "ui.stats.show_deaths",
  }) do
    trx.config.set(key, false)
  end
end

test("the screen between levels lists the level's counts", function()
  fake.set_current_level(CAVES)
  trx.stats.timer = 4 * 30
  trx.stats.pickups.count = 3
  assert(fake.offer(STATS, { level = 1 }), "the screen was not taken")
  local drawn = texts()
  assert(has(drawn, "Caves"), "the title is the level's name")
  assert(has(drawn, L("general/stats/time_taken")))
  assert(has(drawn, "0:04"), "TR1 times are minutes and seconds")
  assert(has(drawn, trx.locale.format("general/stats/detail_fmt", 3, 0)))
  finish()
end)

test("the later games count the hours too", function()
  fake.set_tr_version(2)
  fake.set_current_level(CAVES)
  trx.stats.timer = 4 * 30
  fake.offer(STATS, { level = 1 })
  assert(has(texts(), "00:00:04"))
  finish()
end)

test("the bare look draws its names in capitals", function()
  fake.set_current_level(CAVES)
  fake.offer(STATS, { level = 1, is_bare = true })
  local drawn = texts()
  assert(has(drawn, trx.strings.upper(L("general/stats/time_taken"))))
  assert(has(drawn, "Caves"), "the title keeps its case")
  finish()
end)

test("a level with nothing to show says it is complete", function()
  hide_every_row()
  fake.set_current_level(CAVES)
  fake.offer(STATS, { level = 1 })
  assert(has(texts(), L("general/osd/complete_level")))
  finish()
end)

test("the totals add up every level", function()
  fake.set_current_level(CAVES)
  trx.game.levels[1].stats.pickups.count = 2
  trx.game.levels[2].stats.pickups.count = 3
  fake.offer(STATS, { level = 2, is_final = true })
  local drawn = texts()
  assert(has(drawn, L("general/stats/final_statistics")))
  assert(has(drawn, trx.locale.format("general/stats/detail_fmt", 5, 0)))
  finish()
end)

test("totals with nothing to show leave the screen to end", function()
  hide_every_row()
  assert(
    not fake.offer(STATS, { level = 2, is_final = true }),
    "an empty screen was taken"
  )
  finish()
end)

test("the screen ends on a skip and on no other key", function()
  fake.set_current_level(CAVES)
  fake.offer(STATS, { level = 1 })
  press(Role.MENU_CONFIRM)
  assert(fake.is_held(STATS), "the screen ended on a key it does not read")
  press(Role.MENU_SKIP)
  assert(fake.take_choice(STATS) == CHOICE_CONFIRM, "a skip did not end it")
  finish()
end)

test("a skipped screen fades to black before the next screen", function()
  trx.config.set("ui.stats_fade_effects", true)
  fake.set_current_level(CAVES)
  fake.offer(STATS, { level = 1 })
  press(Role.MENU_SKIP)
  fake.take_choice(STATS)
  fake.change_screen(false)
  assert(fake.holds_screen(), "the next screen did not wait for the fade")
  for _ = 1, 30 do
    fake.tick()
  end
  assert(not fake.holds_screen(), "the fade did not end")
  finish()
end)

test("a skipped screen cuts to the next with its fades off", function()
  trx.config.set("ui.stats_fade_effects", false)
  fake.set_current_level(CAVES)
  fake.offer(STATS, { level = 1 })
  press(Role.MENU_SKIP)
  fake.take_choice(STATS)
  fake.change_screen(false)
  assert(not fake.holds_screen(), "the next screen waited")
  trx.config.set("ui.stats_fade_effects", true)
  finish()
end)

-- The backdrop the screen stands on, as the fake overlay records it.
local function backdrop()
  for _, op in ipairs(fake.scene()) do
    if op:match("^backdrop ") then
      return op
    end
  end
  return nil
end

local function ticks(count)
  for _ = 1, count do
    fake.tick()
  end
end

test("the screen between levels brings its backdrop in", function()
  trx.config.set("ui.stats_background_style", "transparent")
  trx.config.set("ui.stats_fade_effects", true)
  fake.set_current_level(CAVES)
  fake.offer(STATS, { level = 1 })
  assert(backdrop():match("opacity=0.000"), backdrop())
  ticks(16)
  assert(backdrop():match("opacity=1.000"), backdrop())
  finish()
end)

test("a pattern backdrop shows at once", function()
  trx.config.set("ui.stats_background_style", "pattern_static")
  trx.config.set("ui.stats_fade_effects", true)
  fake.set_current_level(CAVES)
  fake.offer(STATS, { level = 1 })
  assert(backdrop():match("opacity=1.000"), backdrop())
  finish()
  trx.config.set("ui.stats_background_style", "transparent")
end)

test("without stats fades the backdrop shows at once", function()
  trx.config.set("ui.stats_fade_effects", false)
  fake.set_current_level(CAVES)
  fake.offer(STATS, { level = 1 })
  fake.tick()
  assert(backdrop():match("opacity=1.000"), backdrop())
  finish()
  trx.config.set("ui.stats_fade_effects", true)
end)

test("the totals come in from black over their picture", function()
  trx.config.set("ui.stats_fade_effects", false)
  fake.set_current_level(CAVES)
  trx.game.levels[1].stats.pickups.count = 2
  fake.offer(STATS, { level = 2, is_final = true, background = "end.webp" })
  assert(backdrop():match("opacity=1.000 path=end.webp"), backdrop())
  ticks(31)
  assert(backdrop():match("opacity=0.000"), backdrop())
  finish()
  trx.config.set("ui.stats_fade_effects", true)
end)

local function open_entry(object)
  fake.set_current_level(CAVES)
  h.open_ring(trx.ui.MenuMode.GAME, object, 0, 10)
  assert(
    fake.offer(RING_ENTRY, { object = object }),
    "the entry was not taken"
  )
  fake.tick()
end

test("the compass shows the level's counts once it is open", function()
  open_entry(COMPASS)
  fake.tick()
  assert(has(texts(), "Caves"))
  finish()
end)

test("the compass clock runs on while it is open", function()
  fake.set_current_level(CAVES)
  trx.stats.timer = 4 * 30
  open_entry(COMPASS)
  fake.tick()
  assert(has(texts(), "0:04"))
  trx.stats.timer = 5 * 30
  fake.tick()
  local drawn = texts()
  assert(has(drawn, "0:05"), "the clock stood still")
  assert(not has(drawn, "0:04"), "the old time stayed")
  finish()
end)

test("backing out of the compass puts it away", function()
  open_entry(COMPASS)
  press(Role.MENU_BACK)
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CANCEL)
  finish()
end)

test("confirming the stopwatch leaves the ring", function()
  fake.set_tr_version(2)
  open_entry(STOPWATCH)
  press(Role.MENU_CONFIRM)
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CONFIRM)
  finish()
end)

local function open_gym()
  fake.set_tr_version(2)
  fake.set_current_level(1)
  h.open_ring(trx.ui.MenuMode.GAME, STOPWATCH, 0, 10)
  fake.offer(RING_ENTRY, { object = STOPWATCH })
  fake.tick()
end

test("the stopwatch in the gym lists the best times", function()
  trx.assault.stats.add_record(65.5)
  trx.assault.stats.add_record(71.2)
  open_gym()
  fake.tick()
  local drawn = texts()
  assert(has(drawn, L("general/stats/assault_title")))
  assert(
    has(drawn, string.format(" 1: %s 1", L("general/stats/assault_finish"))),
    "the fastest run is the first"
  )
  assert(has(drawn, "01:05.5 "), "the tenths keep the room of two digits")
  finish()
end)

test("scrolling the best times moves the rows on show", function()
  for i = 1, 10 do
    trx.assault.stats.add_record(60 + i)
  end
  open_gym()
  fake.tick()
  local first = string.format(" 1: %s", L("general/stats/assault_finish"))
  local function shows(prefix)
    for _, text in ipairs(texts()) do
      if text:sub(1, #prefix) == prefix then
        return true
      end
    end
    return false
  end
  assert(shows(first), "the fastest run is not on show")
  press(Role.MENU_DOWN)
  assert(not shows(first), "the list did not scroll")
  press(Role.MENU_UP)
  assert(shows(first), "the list did not scroll back")
  finish()
end)

test("holding the reset key clears the best times", function()
  trx.assault.stats.add_record(65.5)
  open_gym()
  fake.release_all()
  fake.press(Role.UNBIND_KEY)
  for _ = 1, 45 do
    fake.hold(Role.UNBIND_KEY, true)
    fake.tick()
  end
  fake.release_all()
  assert(#trx.assault.stats.list_records() == 0, "the times stayed")
  finish()
end)

return h.report()
