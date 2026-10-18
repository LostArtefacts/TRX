-- The speedometer of the vehicle Lara rides, drawn as the original TR4 draws
-- the jeep's: a quarter dial of ticks from the left to the top, red past the
-- top speed, a needle that shakes with the engine, and the gear's letter.

local M = {}

-- The dial's size and its parts, in canvas units, measured from its centre.
-- The original draws it slightly wider than tall.
local RADIUS_X = 1.875
local RADIUS_Y = 2
local TICK_INNER = 28
local TICK_OUTER = 32
local NEEDLE_START = -4
local NEEDLE_END = 30
local TICK_STEP = 0x800
local LINE_WIDTH = 1

-- The box the letter of the gear is drawn in.
local GEAR_X = 23
local GEAR_Y = -17
local GEAR_WIDTH = 16
local GEAR_HEIGHT = 17
local SPRITE_DRIVE = 20
local SPRITE_REVERSE = 21

-- The box the dial takes, and where its centre sits in it.
local WIDTH = 100
local HEIGHT = 68
local CENTER_X = 60
local CENTER_Y = 64

local TICK_COLOR = { r = 255, g = 255, b = 255, a = 224 }
local RED_COLOR = { r = 255, g = 64, b = 32, a = 224 }
local NEEDLE_COLOR = { r = 255, g = 96, b = 48, a = 224 }
local GEAR_COLOR = { r = 255, g = 255, b = 255 }

-- Returns a point of the dial at an angle from the left, as far from the
-- centre as given.
local function at(cx, cy, angle, distance)
  local rad = (angle - trx.math.DEG_90) * math.pi / (2 * trx.math.DEG_90)
  return cx + math.sin(rad) * distance * RADIUS_X,
    cy - math.cos(rad) * distance * RADIUS_Y
end

-- Returns the angle of the needle for a dashboard on a tick. The needle
-- shakes as the original's does, more the faster the vehicle goes.
local function needle_angle(dashboard, tick)
  local angle = math.abs(dashboard.speed) // 2
  if angle ~= 0 then
    local shake = trx.math.sin((tick & 7) << 13)
    angle = angle + math.floor((angle - 4096) // 32 * shake)
    angle = math.max(angle, 0)
  end
  return angle
end

---@param state common.ui.hud.overlay.Shared
function M.setup(state)
  local signal = trx.signal
  local primitive = trx.ui.primitive
  local shown = state.ui_enabled & state.playing

  local tick = 0
  local last_angle = nil
  local reading = signal.polled_while(shown, function()
    local dashboard = trx.lara.vehicle_dashboard
    if dashboard == nil then
      last_angle = nil
      return nil
    end
    tick = tick + 1
    local angle = needle_angle(dashboard, tick)
    local reading = {
      dashboard = dashboard,
      from = last_angle or angle,
      to = angle,
    }
    last_angle = angle
    return reading
  end)

  local function paint(x, y)
    local value = reading:get()
    if value == nil then
      return
    end
    local dashboard = value.dashboard
    local cx = x + CENTER_X
    local cy = y + CENTER_Y

    local top = dashboard.top_speed // 2
    local last = dashboard.turbo_speed // 2
    for i = 0, last, TICK_STEP do
      local x0, y0 = at(cx, cy, i, TICK_INNER)
      local x1, y1 = at(cx, cy, i, TICK_OUTER)
      local color = i > top and RED_COLOR or TICK_COLOR
      primitive.line(x0, y0, x1, y1, 0, LINE_WIDTH, color)
    end

    local rate = trx.ui.interpolation_rate()
    local angle = value.from + (value.to - value.from) * rate
    local x0, y0 = at(cx, cy, angle, NEEDLE_START)
    local x1, y1 = at(cx, cy, angle, NEEDLE_END)
    primitive.line(x0, y0, x1, y1, 0, LINE_WIDTH, NEEDLE_COLOR)

    local objects = trx.catalog.objects
    if trx.objects.get(objects.SPARKS_GFX).loaded then
      primitive.sprite_rect(
        objects.SPARKS_GFX,
        dashboard.reverse and SPRITE_REVERSE or SPRITE_DRIVE,
        cx + GEAR_X,
        cy + GEAR_Y,
        0,
        GEAR_WIDTH,
        GEAR_HEIGHT,
        GEAR_COLOR
      )
    end
  end

  trx.ui.regions.place(
    trx.ui.Region.BOTTOM_RIGHT,
    trx.ui.widgets.Custom({
      measure = function()
        return WIDTH, HEIGHT
      end,
      paint = paint,
      visible = shown & reading:map(function(value)
        return value ~= nil
      end),
    })
  )
end

return M
