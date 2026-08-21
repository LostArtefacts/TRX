-- The timer a racing level runs, which TR4 shows in Race for the Iris.
--
-- The elapsed time lives in the level store, so it survives a save. It counts
-- once the level's guide sets off and holds while a cutscene plays, so the
-- scenes the race runs between cost the player nothing. The race ends it for
-- good when the finish is reached, before the scenes that close the level.
--
--   local race_timer = require("tr4.race_timer")
--
--   trx.events.on_game_start(function(is_save)
--     race_timer.arm(is_save)
--   end)

local M = {}

-- Von Croy's walk, which is the state the original engine starts the timer on,
-- and the run he goes into from it.
local GUIDE_WALK = 2
local GUIDE_RUN = 3

-- An hour, after which the original engine stops drawing the timer.
local MAX_FRAMES = 60 * 60 * trx.game.LOGIC_FPS

-- The store is the same table for the life of the session, so taking it once
-- is safe: a load refills it rather than replacing it.
local store = trx.store.level

-- The clock as it is drawn, built once a logic frame.
local clock = nil

local function guide_has_set_off()
  local racers =
    trx.items.query:of_object(trx.catalog.objects.von_croy):simulated()
  for _, item in ipairs(racers:matches()) do
    if item.anim_state == GUIDE_WALK or item.anim_state == GUIDE_RUN then
      return true
    end
  end
  return false
end

local function format_clock(frames)
  local fps = trx.game.LOGIC_FPS
  local seconds = frames // fps
  return string.format(
    "%02d:%02d:%02d",
    seconds // 60,
    seconds % 60,
    (frames % fps) * 100 // fps
  )
end

local function tick()
  local frames = store.race_frames
  if frames == nil or store.race_done then
    clock = nil
    return
  end

  if not store.race_running then
    if guide_has_set_off() then
      store.race_running = true
    end
  elseif not trx.cutscenes.is_playing then
    frames = frames + 1
    store.race_frames = frames
  end

  clock = frames > 0 and frames < MAX_FRAMES and format_clock(frames) or nil
end

local function draw(location)
  if location ~= trx.ui.Location.TOP_CENTER or clock == nil then
    return
  end
  if not trx.game.is_playable or trx.cutscenes.is_playing then
    return
  end
  trx.ui.label(clock)
end

-- Gives the level a timer. A fresh start puts it back to zero; a load leaves
-- what the save carried.
function M.arm(is_save)
  if not is_save then
    store.race_frames = 0
    store.race_running = false
    store.race_done = false
  end
  clock = nil
end

-- Ends the race. The clock stops and leaves the screen for the rest of the
-- level, so it cannot come back between the scenes that close it.
function M.finish()
  store.race_done = true
  store.race_running = false
  clock = nil
end

trx.events.before_control(tick)
trx.events.on_ui_draw(draw)

return M
