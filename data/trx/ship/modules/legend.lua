-- The caption a level opens with, which TR4 shows in Angkor Wat and in The
-- Tomb of Seth.
--
-- A level that wants one names its text, either outright or as a function.
-- A function is read once the level is running and again whenever the player
-- changes language, which is what a caption held in the level's strings needs:
-- those are applied as the level loads, and a script is built before that.
--
--   require("common.legend").setup(function()
--     return trx.locale.get("general/legend")
--   end)
--
-- The caption stands for five seconds of play. The original engine counts it
-- down only while it is on screen, so a caption waits out the opening scenes
-- rather than expiring behind them, and an inventory the player opens costs it
-- nothing.

local M = {}

local FRAMES = 150

-- The bars are up while a cutscene or a flyby holds the screen, which is when
-- the original engine keeps the caption off it.
local function is_clear()
  return trx.game.is_playing
    and not trx.cutscenes.is_playing
    and not trx.overlay.has_letterbox
    -- A flyby draws no bars of its own yet, so the sequence itself answers for
    -- the ones it will draw.
    and not trx.camera.is_flyby_active
end

-- Shows the caption the level opens with. The text is either a string or a
-- function returning one.
function M.setup(source)
  local read = source
  if type(source) ~= "function" then
    read = function()
      return source
    end
  end

  local text = trx.signal.new("")
  local clear = trx.signal.polled(is_clear)
  local running = trx.signal.new(false)
  local left = 0

  local function refresh()
    text:set(read())
  end

  trx.events.on_game_start(function(is_save)
    refresh()
    left = is_save and 0 or FRAMES
    running:set(left > 0)
  end)

  trx.signal.config("language"):on(refresh)

  trx.events.before_control(function()
    if left > 0 and clear:get() then
      left = left - 1
      if left == 0 then
        running:set(false)
      end
    end
  end)

  trx.ui.regions.place(
    trx.ui.Region.BOTTOM_CENTER,
    trx.ui.widgets.Label({
      text = text,
      shown = clear & running,
    })
  )
end

return M
