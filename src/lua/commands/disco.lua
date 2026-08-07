-- Toggles a light show around Lara.
--
-- Usages:
--   /disco        toggle
--   /disco on     force it on
--   /disco off    force it off

trx.locale.declare({
  ["console/cmd/disco/already_off"] = "The lights are already out",
  ["console/cmd/disco/already_on"] = "The lights are already on",
  ["console/cmd/disco/help"] = "Toggles a light show around Lara.",
  ["console/cmd/disco/off"] = "The lights go out",
  ["console/cmd/disco/on"] = "The lights come on",
})

local TURN = 4 * trx.math.DEG_90

-- The lights ride a circle around Lara, evenly spaced along it.
local LIGHT_COUNT = 3
local ORBIT = trx.math.WALL_L
local ORBIT_HEIGHT = trx.math.WALL_L // 2
-- Angle units a light travels each frame: a lap takes about three seconds.
local SPIN = 720
-- Frames a color is held for before the next one takes its place, and how many
-- of them it spends crossing into it. A color that cut straight to the next one
-- read as a flicker rather than as a change of light.
local BEAT = 10
local BLEND = 6
-- How thick the haze sits, and how far the pulse carries it either way.
local HAZE_DENSITY = 100
local HAZE_SWING = 64
-- Frames the show takes to come up and to go back down. The game logic runs at
-- 30 frames a second, so this is half a second either way.
local FADE = 15

local COLORS = {
  { r = 255, g = 32, b = 96 },
  { r = 255, g = 160, b = 0 },
  { r = 64, g = 255, b = 64 },
  { r = 0, g = 208, b = 255 },
  { r = 160, g = 64, b = 255 },
}

local listener = nil
local frame = 0
-- How far the show has come up, from 0 for out to 1 for full, and where it is
-- heading. Everything emitted is scaled by it.
local level = 0
local target_level = 0

-- The crossing into the next color eases in and out, so the light settles on a
-- color rather than sliding through it at an even rate.
local function beat_color(beat)
  local from = COLORS[beat % #COLORS + 1]
  local to = COLORS[(beat + 1) % #COLORS + 1]
  local t = ((frame % BEAT) - (BEAT - BLEND)) / BLEND
  t = math.min(1, math.max(0, t))
  t = t * t * (3 - 2 * t)
  return {
    r = math.floor(from.r + (to.r - from.r) * t),
    g = math.floor(from.g + (to.g - from.g) * t),
    b = math.floor(from.b + (to.b - from.b) * t),
  }
end

local function dim(color)
  return {
    r = math.floor(color.r * level),
    g = math.floor(color.g * level),
    b = math.floor(color.b * level),
  }
end

local function light_up()
  if level < target_level then
    level = math.min(1, level + 1 / FADE)
  elseif level > target_level then
    level = math.max(0, level - 1 / FADE)
    -- Nothing left to draw, so the show stops costing a frame.
    if level == 0 then
      listener:detach()
      listener = nil
      return
    end
  end

  local lara = trx.lara.item
  if lara == nil then
    return
  end

  frame = frame + 1
  local beat = frame // BEAT
  local color = beat_color(beat)

  for i = 1, LIGHT_COUNT do
    local angle = (frame * SPIN + (i - 1) * (TURN // LIGHT_COUNT)) % TURN
    trx.fx.emit_light({
      pos = {
        x = math.floor(lara.pos.x + ORBIT * trx.math.sin(angle)),
        -- The lights rise and fall twice a lap, so they sweep the walls as
        -- well as the floor.
        y = math.floor(
          lara.pos.y
            - ORBIT_HEIGHT
            + ORBIT_HEIGHT * trx.math.sin((2 * angle) % TURN)
        ),
        z = math.floor(lara.pos.z + ORBIT * trx.math.cos(angle)),
      },
      radius = 2 * trx.math.WALL_L,
      color = dim(beat_color(beat + i)),
    })
  end

  trx.fx.emit_fog({
    pos = {
      x = lara.pos.x,
      y = lara.pos.y - ORBIT_HEIGHT,
      z = lara.pos.z,
    },
    radius = 2 * trx.math.WALL_L,
    density = math.floor(
      (HAZE_DENSITY + HAZE_SWING * trx.math.sin((frame * SPIN * 2) % TURN))
        * level
    ),
    color = color,
  })
end

-- The show belongs to the level it was asked for, so it ends outright with
-- that level rather than fading.
trx.events.on_level_unload(function()
  level = 0
  target_level = 0
  if listener ~= nil then
    listener:detach()
    listener = nil
  end
end)

trx.console.register({
  name = "disco",
  help = "console/cmd/disco/help",
  args = function(parser)
    parser:positional("state", { type = "boolean", optional = true })
  end,
  run = function(args)
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    local running = target_level == 1
    local target = args.state
    if target == nil then
      target = not running
    end

    if target == running then
      if target then
        trx.console.log.warning(trx.locale.get("console/cmd/disco/already_on"))
      else
        trx.console.log.warning(
          trx.locale.get("console/cmd/disco/already_off")
        )
      end
      return trx.console.Result.OK
    end

    target_level = target and 1 or 0
    -- Turning it back on while it is still on its way down keeps the listener
    -- and the level it had reached, so the show comes up from where it was.
    if target and listener == nil then
      frame = 0
      listener = trx.events.after_control(light_up)
    end

    if target then
      return trx.console.Result.OK, trx.locale.get("console/cmd/disco/on")
    end
    return trx.console.Result.OK, trx.locale.get("console/cmd/disco/off")
  end,
})
