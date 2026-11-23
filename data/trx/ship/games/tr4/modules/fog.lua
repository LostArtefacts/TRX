-- TR4 levels use flip effect 28 to set the color of the fog bulbs. The timer
-- picks one of the colors below. Timer 100 turns fog bulbs off.
--
-- The table matches the original engine colors. A mod can change it or use
-- the effect for different behavior.
--
--   local fog = require("tr4.fog")
--   fog.colors[2] = trx.math.color(255, 0, 0)

local M = {}

-- The timer that turns the fog bulbs off rather than choosing a color.
local BULBS_OFF = 100

local FLIP_EFFECT = 28

M.colors = {
  [0] = trx.math.color(0, 0, 0),
  trx.math.color(245, 200, 60),
  trx.math.color(120, 196, 112),
  trx.math.color(202, 204, 230),
  trx.math.color(128, 64, 0),
  trx.math.color(64, 64, 64),
  trx.math.color(243, 232, 236),
  trx.math.color(0, 64, 192),
  trx.math.color(0, 128, 0),
  trx.math.color(150, 172, 157),
  trx.math.color(128, 128, 128),
  trx.math.color(204, 163, 123),
  trx.math.color(177, 162, 140),
  trx.math.color(0, 223, 191),
  trx.math.color(111, 255, 223),
  trx.math.color(244, 216, 152),
  trx.math.color(248, 192, 60),
  trx.math.color(252, 0, 0),
  trx.math.color(198, 95, 87),
  trx.math.color(226, 151, 118),
  trx.math.color(248, 235, 206),
  trx.math.color(0, 30, 16),
  trx.math.color(250, 222, 167),
  trx.math.color(218, 175, 117),
  trx.math.color(225, 191, 78),
  trx.math.color(77, 140, 141),
  trx.math.color(4, 181, 154),
  trx.math.color(255, 174, 0),
}

local BULBS_KEY = "visuals.enable_fog_bulbs"

-- The store is the same table for the life of the session, and a load fills it
-- again, so the timer the effect last picked comes back with a save.
local store = trx.store.level

-- Colors the fog bulbs with the color of a timer. The original changes only
-- the bulbs with this effect: its distance fog stays black, and the two
-- levels that it fogs in another color set that color in the game flow.
local function color_bulbs(timer)
  local color = M.colors[timer]
  if color == nil then
    trx.log.warning("flip effect " .. FLIP_EFFECT .. " has no color " .. timer)
    return
  end
  for _, bulb in pairs(trx.fx.fog_bulbs) do
    bulb.color = color
  end
end

trx.events.on_game_start(function()
  if store.fog_color_timer ~= nil then
    color_bulbs(store.fog_color_timer)
  end
end)

-- Turns fog bulbs off, or restores the player setting. The original turns
-- them back on when the effect picks a color.
local function hold_bulbs_off(off)
  if off and not trx.config.is_overridden(BULBS_KEY) then
    trx.config.override(BULBS_KEY, false)
  elseif not off and trx.config.is_overridden(BULBS_KEY) then
    trx.config.restore(BULBS_KEY)
  end
end

trx.events.on_flip_effect(FLIP_EFFECT, function(timer)
  if timer == BULBS_OFF then
    hold_bulbs_off(true)
    return
  end
  hold_bulbs_off(false)
  store.fog_color_timer = timer
  color_bulbs(timer)
end)

return M
