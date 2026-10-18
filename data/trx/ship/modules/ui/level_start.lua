-- How a level comes in when it starts or a save is loaded.
--
-- Usage:
--   local level_start = require("common.ui.level_start")
--   level_start.setup()         -- once, from a global script
--   level_start.hand_over(ctx)  -- from a loading picture's screen

local fade = require("common.ui.fade")

local M = {}

local TRANSITION = "visuals.level_start_transition"

-- Written out in full because the scanner that carries these into the
-- shipped strings reads a key only where it is a literal.
trx.locale.declare({
  ["settings/visuals.level_start_transition/title"] = "Level start transition",
  ["settings/visuals.level_start_transition/description"] = "How a level comes in when it starts or a save is loaded. "
    .. "Cross-fade blends the loading screen into the level. Through black fades the loading screen out to black "
    .. "and the level in from black, as in the original TR4. Without a loading screen, both fade the level in "
    .. "from black. Disabled shows the level at once.",
  ["settings/visuals.level_start_transition/values/disabled"] = "Disabled",
  ["settings/visuals.level_start_transition/values/cross-fade"] = "Cross-fade",
  ["settings/visuals.level_start_transition/values/black"] = "Through black",
})

-- How long a level takes to fade in from black.
local LEVEL_FADE_IN_TICKS = 16

-- The transition in force: none while fade effects are off.
local function transition()
  if not fade.enabled() then
    return "disabled"
  end
  return trx.config.get(TRANSITION)
end

-- What is on screen as a level comes in: the fade that a loading picture
-- handed over and how long it takes to go, the fade that is going, and the
-- wait for the level to run for the first time.
---@type { held?: common.ui.fade.Across, held_ticks: integer, going?: common.ui.fade.Across, first_run?: trx.events.Listener }
local level = { held_ticks = 0 }

local function drop_level()
  if level.first_run ~= nil then
    level.first_run:detach()
  end
  level = { held_ticks = 0 }
end

-- Ends a loading picture's screen, handing what is on screen over to the
-- level as the player picked.
function M.hand_over(ctx)
  if level.held ~= nil then
    level.held:close()
  end
  drop_level()
  local mode = transition()
  local drops = { "title", "unload", "cutscene" }
  if mode == "cross-fade" then
    level.held = fade.across({ layer = { snapshot = true }, drop_on = drops })
    level.held_ticks = ctx.fade_out_ticks
    ctx:confirm()
  elseif mode == "black" then
    level.held = fade.across({
      from = 0,
      to = 1,
      ticks = ctx.fade_out_ticks,
      drop_on = drops,
      on_done = function()
        if ctx.is_held then
          ctx:confirm()
        end
      end,
    })
    level.held_ticks = LEVEL_FADE_IN_TICKS
  else
    ctx:confirm()
  end
end

-- Fades the level in from black as it first runs, unless the fade from its
-- loading picture is still on screen.
local function on_first_run()
  level.first_run:detach()
  level.first_run = nil
  local going = level.going
  level.going = nil
  if going ~= nil and going:is_open() or transition() == "disabled" then
    return
  end
  fade.across({
    from = 1,
    to = 0,
    ticks = LEVEL_FADE_IN_TICKS,
    drop_on = { "title", "unload" },
  })
end

local function on_game_start()
  local current = trx.game.current_level
  if current ~= nil and current.type == trx.game.LevelType.CUTSCENE then
    drop_level()
    return
  end
  local held = level.held
  level.held = nil
  if held ~= nil and held:is_open() then
    level.going = held:to(0, level.held_ticks)
  end
  if level.first_run ~= nil then
    level.first_run:detach()
  end
  level.first_run = trx.events.before_control(on_first_run)
end

-- Declares the setting that picks how a level comes in.
-- Listens for the level starting. Called once, from a global script, after
-- `fade.setup`.
function M.setup()
  trx.config.declare({
    key = TRANSITION,
    kind = "dynamic_enum",
    values = { "disabled", "cross-fade", "black" },
    default = trx.game.version == 4 and "black" or "cross-fade",
    ui = {
      tab = "graphic_ui",
      after = "visuals.enable_exit_fade_effects",
      is_available = function()
        return trx.config.get("visuals.enable_fade_effects")
      end,
    },
  })

  trx.events.on_game_start(on_game_start)
  trx.events.on_title_start(drop_level)
  trx.events.on_level_unload(drop_level)
end

return M
