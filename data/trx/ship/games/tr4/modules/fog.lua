-- TR4 levels use flip effect 28 to set the fog color. The timer picks one of
-- the colors below. Timer 100 turns fog bulbs off.
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

-- The color the effect last picked, kept so the distance fog goes back to it
-- when fog bulbs are turned on again.
local held_color = nil

-- The original colors the distance fog only while volumetric fog is on: the
-- effect belongs to that feature, so the fog falls back to the level color, or
-- to the player's, where the bulbs are off.
local function apply_fog_color()
  trx.fx.fog_color = trx.config.get(BULBS_KEY) and held_color or nil
end

trx.config.on_change(BULBS_KEY, apply_fog_color)

-- The color belongs to the level that picked it, and a level change clears the
-- override the engine holds.
trx.events.on_level_unload(function()
  held_color = nil
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

  local color = M.colors[timer]
  if color == nil then
    trx.log.warning("flip effect " .. FLIP_EFFECT .. " has no color " .. timer)
    return
  end
  held_color = color
  apply_fog_color()
end)

return M
