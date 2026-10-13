-- How a level comes in when it starts or a save is loaded.
--
-- Usage:
--   local level_start = require("common.ui.level_start")
--   level_start.setup()         -- once, from a global script
--   level_start.hand_over(ctx)  -- from a loading picture's screen

local fade = require("common.ui.fade")
local timer = require("common.timer")

local M = {}

local TRANSITION = "visuals.level_start_transition"

-- How long a level takes to fade in from black.
local LEVEL_FADE_IN_TICKS = 16

-- The transition in force: none while fade effects are off.
local function transition()
  if not fade.enabled() then
    return "disabled"
  end
  return trx.config.get(TRANSITION)
end

---@type trx.ui.FadeLayer?
local held = nil
local held_ticks = 0
---@type trx.ui.FadeLayer?
local moving = nil
---@type common.timer.Timer?
local first_control = nil

local function drop_level()
  held = nil
  moving = nil
  if first_control ~= nil then
    first_control:cancel()
    first_control = nil
  end
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
  elseif mode == "black" then
    held = fade.across({
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
    held_ticks = LEVEL_FADE_IN_TICKS
  else
    ctx:confirm()
  end
end

-- Fades the level in from black as it is first played, unless the fade from
-- its loading picture is still on screen.
local function fade_in_level()
  first_control = nil
  local open = moving ~= nil and moving.is_open
  moving = nil
  if open or transition() == "disabled" then
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
    moving = held
  end
  held = nil
  if first_control ~= nil then
    first_control:cancel()
  end
  first_control = timer.after(0, fade_in_level)
end

-- Declares the setting that picks how a level comes in.
-- Listens for the level starting. Called once, from a global script, after
-- `fade.setup`.
function M.setup()
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
