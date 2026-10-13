-- How a level comes in when it starts or a save is loaded.
--
-- Usage:
--   local level_start = require("common.ui.level_start")
--   level_start.setup()         -- once, from a global script
--   level_start.hand_over(ctx)  -- from a loading picture's screen

local fade = require("common.ui.fade")

local M = {}

-- The transition in force: none while fade effects are off.
local function transition()
  if not fade.enabled() then
    return "disabled"
  end
  return "cross-fade"
end

---@type trx.ui.FadeLayer?
local held = nil
local held_ticks = 0

local function drop_level()
  held = nil
end

-- Ends a loading picture's screen, handing what is on screen over to the
-- level as the player picked.
function M.hand_over(ctx)
  if held ~= nil then
    held:close()
  end
  drop_level()
  local mode = transition()
  local drops = { "title", "unload", "cutscene", "demo" }
  if mode == "cross-fade" then
    held = fade.across({ layer = { snapshot = true }, drop_on = drops })
    held_ticks = ctx.fade_out_ticks
    ctx:confirm()
  else
    ctx:confirm()
  end
end

local function on_game_start()
  local level = trx.game.current_level
  if
    level ~= nil
    and (
      level.type == trx.game.LevelType.CUTSCENE
      or level.type == trx.game.LevelType.DEMO
    )
  then
    drop_level()
    return
  end
  if held ~= nil and held.is_open then
    fade.move(held, 0, held_ticks)
  end
  held = nil
end

-- Listens for the level starting. Called once, from a global script, after
-- `fade.setup`.
function M.setup()
  trx.events.on_game_start(on_game_start)
  trx.events.on_title_start(drop_level)
  trx.events.on_level_unload(drop_level)
end

return M
