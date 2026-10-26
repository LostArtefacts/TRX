-- The caption a level opens with, which TR4 shows in Angkor Wat and in The
-- Tomb of Seth.
--
-- A level that wants one names its text. A `trx.locale.Text` follows the
-- level's own strings, which are applied after the script is built, and the
-- player's language.
--
-- Usage:
--   require("common.ui.hud.legend").setup(trx.locale.text("general/legend"))
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
    and not trx.cutscenes.is_active
    and not trx.overlay.has_letterbox
    -- A flyby draws no bars of its own yet, so the sequence itself answers for
    -- the ones it will draw.
    and not trx.camera.is_flyby_active
end

-- Shows the caption the level opens with.
---@param text string|trx.locale.Text
function M.setup(text)
  local running = trx.signal.new(false)
  -- Read only while the caption still has time left, so a caption that has
  -- gone costs nothing.
  local clear = trx.signal.polled_while(running, is_clear, false)
  local left = 0
  ---@type trx.events.Listener?
  local counting = nil

  local function stop()
    left = 0
    running:set(false)
    if counting ~= nil then
      counting:detach()
      counting = nil
    end
  end

  local function count()
    if left > 0 and clear:get() then
      left = left - 1
      if left == 0 then
        stop()
      end
    end
  end

  trx.events.on_game_start(function(is_save)
    stop()
    if not is_save then
      left = FRAMES
      running:set(true)
      counting = trx.events.before_control(count)
    end
  end)

  trx.ui.regions.place(
    trx.ui.Region.BOTTOM_CENTER,
    trx.ui.widgets.Label({
      text = text,
      visible = clear & running,
    })
  )
end

return M
