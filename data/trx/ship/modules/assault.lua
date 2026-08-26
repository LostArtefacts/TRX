-- The assault course readouts: the run timer, the penalties it has taken, and
-- the lap times the quad bike circuit shows when a lap ends.
--
-- The engine draws these at fixed places under the top of the screen. Here they
-- are widgets in the top middle region, so they stack with the rest of the
-- interface instead of sitting over it.
--
-- Every one of them is made once and given signals. The timings are polled,
-- because the engine publishes nothing for them, and only while something is
-- on screen to read them.

local ui = trx.ui
local signal = trx.signal
local assault = trx.assault

local DIGITS = trx.catalog.objects.assault_digits
local COURSE = assault.Track.COURSE
local QUAD = assault.Track.QUAD

-- The readouts belong to the run, so they go while the inventory ring, the
-- pause screen or photo mode holds the level still.
local playing = trx.game.signals.is_playing

-- The palettes the engine draws these in. Each is a color at the top of a
-- character fading to a darker one at the bottom.
local PALETTE = {
  white = { "ffffff", "ffffff" },
  neutral = { "ffffff", "404040" },
  grey = { "808080", "1a1a1a" },
  green = { "59bf33", "1a4000" },
  red = { "e63300", "4d0000" },
  pink = { "ff00ff", "400040" },
}

for _, palette in pairs(PALETTE) do
  palette.top = trx.math.color(palette[1])
  palette.bottom = trx.math.color(palette[2])
end

-- TR2 draws the digits flat white; TR3 gives them the shaded palette.
local plain = trx.game.tr_version < 3 and PALETTE.white or PALETTE.neutral

-- Reads a timing once a tick, and only while it is on screen. A readout that
-- is off costs one signal rather than a poll.
local function timing(shown, read)
  local held = signal.new(0)
  local ticks = nil
  local function follow(on)
    if on then
      held:set(read())
      if ticks == nil then
        ticks = signal.tick:on(function()
          held:set(read())
        end)
      end
    else
      held:set(0)
      if ticks ~= nil then
        ticks:detach()
        ticks = nil
      end
    end
  end
  follow(shown:get())
  local watch = shown:on(follow)
  -- The signals the API derives let go of the level themselves. These two are
  -- attached by hand, so they are let go of by hand.
  trx.events.on_level_unload(function()
    watch:detach()
    follow(false)
  end)
  return held
end

-- The clock as the game writes it: minutes, seconds, and a tenth. A run that
-- has not started yet shows dashes where a lap time is asked for.
local function format_time(frames, placeholder)
  if placeholder and frames <= 0 then
    return "--:--.-"
  end
  local seconds = frames // trx.game.LOGIC_FPS
  local tenths = (frames % trx.game.LOGIC_FPS) * 10 // trx.game.LOGIC_FPS
  return ("%d:%02d.%d"):format(seconds // 60, seconds % 60, tenths)
end

-- A penalty is written in whole seconds, and carries the unit after it.
local function format_penalty(frames, is_target)
  local seconds = frames // trx.game.LOGIC_FPS
  local fmt = is_target and "T %d:%02d s" or "%d:%02d s"
  return fmt:format(seconds // 60, seconds % 60)
end

-- The lap times take the top middle to themselves while they are up, which is
-- what the engine does with them. The run timer stands down for the whole of
-- that, even where the lap itself has no time to show.
local lap_up = signal.polled(function()
  return assault.get_lap_timer(QUAD) > 0
end) & playing

local lap_shown = lap_up
  & signal.polled(function()
    return assault.get_lap_time(QUAD) > 0
  end)

-------------------------------------------------------------------------------
-- the run timer, with the penalties standing to the left of it
-------------------------------------------------------------------------------

-- What the engine leaves between the penalties and the clock.
local PENALTY_GAP = 40

do
  local on_screen = signal.polled(function()
    local track = assault.active_track
    return track ~= nil and assault.is_visible(track)
  end) & playing & ~lap_up

  local time = timing(on_screen, function()
    return assault.get_time()
  end)

  local clock = ui.widgets.Digits({
    object = DIGITS,
    text = time:map(function(frames)
      return format_time(frames, false)
    end),
    color = plain.top,
    color_bottom = plain.bottom,
  })

  -- The penalties, which stand for a moment after a pad or a target is
  -- missed. Each is read once, and drawn twice: once beside the clock, and
  -- once more on the far side to hold the same room open.
  local penalties = {}
  for _, is_target in ipairs({ false, true }) do
    local read = is_target and assault.get_target_penalty
      or assault.get_penalty

    local shown = on_screen
      & signal.polled(function()
        local track = assault.active_track
        return track ~= nil
          and assault.get_penalty_timer(track) > 0
          and read(track) > 0
      end)

    local frames = timing(shown, function()
      return read(assault.active_track or COURSE)
    end)

    penalties[#penalties + 1] = {
      shown = shown,
      text = frames:map(function(value)
        return format_penalty(value, is_target)
      end),
    }
  end

  -- Both penalties end in the unit, so aligning them to the right lines the
  -- digits up and leaves the target mark standing out to the left, which is
  -- how the engine writes them.
  local function penalty_column(hidden)
    local children = {}
    for _, penalty in ipairs(penalties) do
      children[#children + 1] = ui.widgets.Digits({
        object = DIGITS,
        text = penalty.text,
        color = PALETTE.pink.top,
        color_bottom = PALETTE.pink.bottom,
        mark_color = plain.top,
        mark_color_bottom = plain.bottom,
        shown = penalty.shown,
      })
    end
    local column = ui.widgets.Stack({
      spacing = 3,
      align = ui.HAlign.RIGHT,
      children = children,
    })
    column.hidden = hidden
    return column
  end

  -- The penalties take the left, and as much room again is held open on the
  -- right, so that the clock stands in the middle whether a run has taken a
  -- penalty or not.
  ui.regions.place(
    ui.Region.TOP_CENTER,
    ui.widgets.Stack({
      orientation = ui.Orientation.HORIZONTAL,
      spacing = PENALTY_GAP,
      shown = on_screen,
      children = { penalty_column(false), clock, penalty_column(true) },
    })
  )
end

-------------------------------------------------------------------------------
-- the lap times, the last beside the best the circuit has on record
-------------------------------------------------------------------------------

do
  local last = timing(lap_shown, function()
    return assault.get_lap_time(QUAD)
  end)
  local best = timing(lap_shown, function()
    return assault.get_best_time(QUAD)
  end)

  -- A lap that matches the record is drawn green on both sides. Otherwise the
  -- lap is red against a record and neutral without one.
  local is_best = signal.combine(last, best, function(last_time, best_time)
    return best_time > 0 and last_time == best_time
  end)

  -- A circuit with no record on file shows the lap alone, as the engine does.
  local has_best = best:map(function(best_time)
    return best_time > 0
  end)

  local function lap_digits(time, shown, color_of)
    local color = signal.combine(is_best, best, color_of)
    return ui.widgets.Digits({
      object = DIGITS,
      text = time:map(function(frames)
        return format_time(frames, true)
      end),
      color = color:map(function(palette)
        return palette.top
      end),
      color_bottom = color:map(function(palette)
        return palette.bottom
      end),
      shown = shown,
    })
  end

  ui.regions.place(
    ui.Region.TOP_CENTER,
    ui.widgets.Stack({
      orientation = ui.Orientation.HORIZONTAL,
      spacing = 20,
      shown = lap_shown,
      children = {
        lap_digits(last, true, function(best_lap, best_time)
          if best_lap then
            return PALETTE.green
          end
          return best_time > 0 and PALETTE.red or PALETTE.neutral
        end),
        lap_digits(best, has_best, function(best_lap)
          return best_lap and PALETTE.green or PALETTE.grey
        end),
      },
    })
  )
end
