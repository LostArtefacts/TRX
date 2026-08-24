-- The timer a racing level runs, which TR4 shows in Race for the Iris.
--
-- The elapsed time lives in the level store, so it survives a save. It counts
-- once the level's guide sets off and holds while a cutscene plays, so the
-- scenes the race runs between cost the player nothing. The race ends it for
-- good when the finish is reached, before the scenes that close the level.
--
--   local race_timer = require("tr4.race_timer")

local M = {}

-- Von Croy's walk, which is the state the original engine starts the timer on,
-- and the run he goes into from it.
local GUIDE_WALK = 2
local GUIDE_RUN = 3

-- An hour, after which the original engine stops drawing the timer.
local MAX_FRAMES = 60 * 60 * trx.game.LOGIC_FPS

-- The store is the same table for the life of the session, so taking it once
-- is safe: a load refills it rather than replacing it. It is also the only
-- place the race is kept, so a script that runs again over a level already
-- under way, as a load makes it, picks the race up where it stands.
local store = trx.store.level

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
  if store.race_done then
    return
  end

  if not store.race_running then
    store.race_running = guide_has_set_off()
  elseif not trx.cutscenes.is_playing then
    store.race_frames = (store.race_frames or 0) + 1
  end
end

-- Ends the race. The clock stops and leaves the screen for the rest of the
-- level, so it cannot come back between the scenes that close it.
function M.finish()
  store.race_done = true
  store.race_running = false
end

-- The clock as it reads now, or nil while it has nothing to show. It stays
-- hidden during the race cutscenes and once the race is over.
local function clock()
  local frames = store.race_frames or 0
  if store.race_done or frames <= 0 or frames >= MAX_FRAMES then
    return nil
  end
  if not trx.game.is_playable or trx.cutscenes.is_playing then
    return nil
  end
  return format_clock(frames)
end

-- The clock is a single label at the top of the screen.
local text = trx.signal.polled(clock)

trx.ui.regions.place(
  trx.ui.Region.TOP_CENTER,
  trx.ui.widgets.Label({
    text = text:map(function(value)
      return value or ""
    end),
    shown = text:map(function(value)
      return value ~= nil
    end),
  })
)

trx.events.before_control(tick)

return M
