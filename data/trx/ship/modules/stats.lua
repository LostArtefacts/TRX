-- The statistics screen: what the player found in a level when it ends, the
-- totals when the game ends, and the same from the compass or the stopwatch
-- in the inventory ring. In the gym, the stopwatch shows the best times on the
-- assault course instead, and a held key clears them.
--
-- Usage:
--   require("common.stats").setup()

local M = {}

-- Fallback text for the screen's own strings. The strings files and their
-- translations take precedence.
trx.locale.declare({
  ["general/stats/basic_fmt"] = "%d",
  ["general/stats/detail_fmt"] = "%d of %d",
  ["general/stats/final_statistics"] = "Final Statistics",
  ["general/stats/bonus_statistics"] = "Bonus Statistics",
  ["general/stats/assault_title"] = "BEST TIMES",
  ["general/stats/assault_no_times_set"] = "No Times Set",
  ["general/stats/assault_finish"] = "Finish",
  ["general/stats/assault_best_time_fmt"] = "%s",
  ["general/stats/assault_other_times_fmt"] = "%s",
  ["general/stats/gym_assault_course"] = "Assault Course",
  ["general/stats/gym_racetrack_course"] = "Race Track Course",
  ["general/stats/assault_reset_times"] = "Reset",
  ["general/stats/level"] = "Level",
  ["general/stats/time_taken"] = "Time Taken",
  ["general/stats/ammo"] = "Ammo Hits/Used",
  ["general/stats/ammo_used"] = "Ammo Used",
  ["general/stats/ammo_hits"] = "Hits",
  ["general/stats/kills"] = "Kills",
  ["general/stats/crystals"] = "Crystals",
  ["general/stats/pickups"] = "Pickups",
  ["general/stats/deaths"] = "Deaths",
  ["general/stats/medipacks_used"] = "Health Packs Used",
  ["general/stats/distance_travelled"] = "Distance Traveled",
  ["general/stats/secrets"] = "Secrets Found",
  ["general/stats/none"] = "None",
})

local Mode = {
  LEVEL = 1,
  FINAL = 2,
  ASSAULT = 3,
}

local FPS = 30
local TEXT_HEIGHT = 15

local BARE_ROW_SPACING = 11.0
local MIN_MARGIN_ROWS = 0.5
local MIN_ROW_SPACING_ROWS = 1.0 / 6.0

local MAX_ASSAULT_ROWS = 7
local ASSAULT_CHROME = 80.0
local ASSAULT_MIN_WIDTH = 290.0
local ASSAULT_ROW_SPACING = 3.0
local MAX_ASSAULT_TIMES = 10

local HINT_HEIGHT = 7
local HINT_SCALE = 0.7

local RESET_TEXT_SCALE = 0.85
local RESET_PAD_X = 6.0
local RESET_PAD_Y = 3.0
local RESET_SPACING = 2.0
local RESET_HOLD_DEBUFF = 10
local RESET_HOLD_MAX = 30

-- TR1 sits its box in the middle of the screen; the later games stand it on
-- the foot of the screen, inside a margin.
local function look()
  if trx.game.tr_version == 1 then
    return {
      window_margin = 0.0,
      window_y = 0.5,
      title_spacing = 4.0,
      min_width = 0.0,
      row_spacing = 30.0,
      full_hours = false,
    }
  end
  return {
    window_margin = 40.0,
    window_y = 1.0,
    title_spacing = 3.0,
    min_width = 290.0,
    row_spacing = 25.0,
    full_hours = true,
  }
end

-------------------------------------------------------------------------------
-- The numbers
-------------------------------------------------------------------------------

local function format_time(state, frames)
  local total = frames // FPS
  local hours = total // 3600
  local minutes = total // 60 % 60
  local seconds = total % 60
  if state.look.full_hours then
    return string.format("%02d:%02d:%02d", hours, minutes, seconds)
  elseif hours ~= 0 then
    return string.format("%d:%02d:%02d", hours, minutes, seconds)
  end
  return string.format("%d:%02d", minutes, seconds)
end

local function format_record_time(frames)
  local total = frames // FPS
  local minutes = total // 60 % 60
  local seconds = total % 60
  local rest = frames % FPS
  if trx.game.tr_version >= 3 then
    return string.format("%d:%02d.%02d", minutes, seconds, rest * 100 // FPS)
  end
  return string.format("%02d:%02d.%-2d", minutes, seconds, rest * 10 // FPS)
end

local function format_distance(distance)
  distance = distance // 445
  if distance < 1000 then
    return string.format("%dm", distance)
  end
  return string.format("%d.%02dkm", distance // 1000, distance % 1000 // 10)
end

-- The levels of the game, the gym first where the game has one, in the order
-- the game flow lists them.
local function every_level()
  local levels = {}
  if trx.game.gym ~= nil then
    levels[#levels + 1] = trx.game.gym
  end
  for _, level in ipairs(trx.game.levels) do
    levels[#levels + 1] = level
  end
  return levels
end

local CATEGORIES = { "pickups", "kills", "secrets", "crystals" }

-- One level's counters. The allies only count against the player once she has
-- turned on one, so the kills to find depend on the levels up to this one.
local function level_totals(level)
  local stats = level.stats
  local totals = {
    timer = stats.timer,
    deaths = stats.deaths,
    ammo_used = stats.ammo_used,
    ammo_hits = stats.ammo_hits,
    distance_travelled = stats.distance_travelled,
    medipacks_used = stats.medipacks_used,
    counts = {},
    maxes = {},
  }
  for _, name in ipairs(CATEGORIES) do
    local category = stats[name]
    totals.counts[name] = category ~= nil and category.count or 0
    totals.maxes[name] = category ~= nil and category.max or 0
  end

  local hurt = false
  if level.num > 0 then
    for _, other in ipairs(every_level()) do
      local other_stats = other.stats
      if other_stats ~= nil and other_stats.allies_hurt then
        hurt = true
      end
      if other.num == level.num and other.type == level.type then
        break
      end
    end
  end
  totals.maxes.kills = stats.max_enemy_kills
    + (hurt and stats.max_ally_kills or 0)
  return totals
end

-- The whole game's counters: every level, and the bonus levels too where the
-- game ended on one.
local function final_totals(include_bonus)
  local totals = {
    timer = 0,
    deaths = 0,
    ammo_used = 0,
    ammo_hits = 0,
    distance_travelled = 0,
    medipacks_used = 0.0,
    counts = { pickups = 0, kills = 0, secrets = 0, crystals = 0 },
    maxes = { pickups = 0, kills = 0, secrets = 0, crystals = 0 },
  }
  local ally_kills = 0
  local enemy_kills = 0
  local hurt = false
  for _, level in ipairs(trx.game.levels) do
    local counted = level.type == trx.game.LevelType.NORMAL
      or (level.type == trx.game.LevelType.BONUS and include_bonus)
    local stats = counted and level.stats or nil
    if stats ~= nil then
      totals.timer = totals.timer + stats.timer
      totals.deaths = totals.deaths + stats.deaths
      totals.ammo_used = totals.ammo_used + stats.ammo_used
      totals.ammo_hits = totals.ammo_hits + stats.ammo_hits
      totals.distance_travelled = totals.distance_travelled
        + stats.distance_travelled
      totals.medipacks_used = totals.medipacks_used + stats.medipacks_used
      for _, name in ipairs(CATEGORIES) do
        local category = stats[name]
        if category ~= nil then
          totals.counts[name] = totals.counts[name] + category.count
          totals.maxes[name] = totals.maxes[name] + category.max
        end
      end
      ally_kills = ally_kills + (stats.max_ally_kills or 0)
      enemy_kills = enemy_kills + (stats.max_enemy_kills or 0)
      hurt = hurt or stats.allies_hurt
    end
  end
  totals.maxes.kills = enemy_kills + (hurt and ally_kills or 0)
  return totals
end

local function count_completed()
  local count = 0
  for _, level in ipairs(every_level()) do
    if level.is_completed then
      count = count + 1
    end
  end
  return count
end

-- The secrets as their glyphs, a found one drawn plain and a missed one in
-- italics. A missed secret is left out until a found one comes before it, so
-- the row does not open on a gap.
local function icon_secrets(level)
  local parts = {}
  local found = 0
  for _, secret in ipairs(level.stats:secret_list()) do
    if secret.found or #parts > 0 then
      if secret.icon ~= nil then
        local glyph = "\\{secret " .. secret.icon .. "}"
        parts[#parts + 1] = secret.found and glyph
          or "\\{i}" .. glyph .. "\\{/i}"
      end
      if secret.found then
        found = found + 1
      end
    end
  end
  if found == 0 then
    return trx.locale.get("general/stats/none")
  end
  return table.concat(parts)
end

-- A secret that is a place to reach has no glyph to draw, so a level with one
-- counts its secrets instead of drawing them.
local function has_floor_secrets(level)
  for _, secret in ipairs(level.stats:secret_list()) do
    if secret.icon == nil then
      return true
    end
  end
  return false
end

-------------------------------------------------------------------------------
-- The rows
-------------------------------------------------------------------------------

local function is_bare(state)
  return state.bare
end

-- Pickup crystals exist only to be counted, so their row is not optional.
local function show_crystals()
  return trx.config.get("ui.stats.show_crystals")
    or trx.config.get("gameplay.save_crystal_mode") == "pickup"
end

local function row(_, key, value)
  return { key = key, value = value }
end

local function centered(text)
  return { text = text }
end

local function auto_secrets_row(state)
  local totals = state.totals
  if state.mode == Mode.FINAL or state.floor_secrets then
    return row(
      state,
      trx.locale.get("general/stats/secrets"),
      trx.locale.format(
        "general/stats/detail_fmt",
        totals.counts.secrets,
        totals.maxes.secrets
      )
    )
  end
  return row(
    state,
    trx.locale.get("general/stats/secrets"),
    icon_secrets(state.level)
  )
end

local function count_row(state, key, name)
  local fmt = trx.config.get("ui.stats.show_totals")
      and "general/stats/detail_fmt"
    or "general/stats/basic_fmt"
  return row(
    state,
    trx.locale.get(key),
    trx.locale.format(fmt, state.totals.counts[name], state.totals.maxes[name])
  )
end

-- The rows that the settings ask for, in the order the look puts them.
local function configured_rows(state)
  local totals = state.totals
  local get = trx.config.get
  local rows = {}
  local function add(r)
    rows[#rows + 1] = r
  end

  local function timer()
    add(
      row(
        state,
        trx.locale.get("general/stats/time_taken"),
        format_time(state, totals.timer)
      )
    )
  end

  if is_bare(state) then
    if get("ui.stats.show_kills") then
      add(count_row(state, "general/stats/kills", "kills"))
    end
    if get("ui.stats.show_pickups") then
      add(count_row(state, "general/stats/pickups", "pickups"))
    end
    if show_crystals() and totals.maxes.crystals ~= 0 then
      add(count_row(state, "general/stats/crystals", "crystals"))
    end
    if get("ui.stats.show_secrets") and totals.maxes.secrets ~= 0 then
      add(auto_secrets_row(state))
    end
    if get("ui.stats.show_time_taken") then
      timer()
    end
  else
    if get("ui.stats.show_time_taken") then
      timer()
    end
    if get("ui.stats.show_secrets") and totals.maxes.secrets ~= 0 then
      add(auto_secrets_row(state))
    end
    if show_crystals() and totals.maxes.crystals ~= 0 then
      add(count_row(state, "general/stats/crystals", "crystals"))
    end
    if get("ui.stats.show_pickups") then
      add(count_row(state, "general/stats/pickups", "pickups"))
    end
    if get("ui.stats.show_kills") then
      add(count_row(state, "general/stats/kills", "kills"))
    end
  end

  if get("ui.stats.show_ammo") then
    if is_bare(state) then
      add(
        row(
          state,
          trx.locale.get("general/stats/ammo_used"),
          string.format("%d", totals.ammo_used)
        )
      )
      add(
        row(
          state,
          trx.locale.get("general/stats/ammo_hits"),
          string.format("%d", totals.ammo_hits)
        )
      )
    else
      add(
        row(
          state,
          trx.locale.get("general/stats/ammo"),
          trx.locale.format(
            "general/misc/pagination_nav",
            totals.ammo_hits,
            totals.ammo_used
          )
        )
      )
    end
  end
  if get("ui.stats.show_medipacks_used") then
    add(
      row(
        state,
        trx.locale.get("general/stats/medipacks_used"),
        string.format("%.1f", totals.medipacks_used)
      )
    )
  end
  if get("ui.stats.show_distance_travelled") then
    add(
      row(
        state,
        trx.locale.get("general/stats/distance_travelled"),
        format_distance(totals.distance_travelled)
      )
    )
  end
  -- Deaths stay with the level they happen on, so a player who dies in one
  -- level and reloads an earlier one still sees them counted.
  if get("ui.stats.show_deaths") and totals.deaths >= 0 then
    add(
      row(
        state,
        trx.locale.get("general/stats/deaths"),
        trx.locale.format("general/stats/basic_fmt", totals.deaths)
      )
    )
  end
  return rows
end

local function has_quad()
  return trx.game.tr_version >= 3
end

local function record_limit()
  return trx.game.tr_version >= 3 and 3 or MAX_ASSAULT_TIMES
end

local function tracks()
  local result = { trx.assault.Track.COURSE }
  if has_quad() then
    result[#result + 1] = trx.assault.Track.QUAD
  end
  return result
end

local function records_of(track)
  local records = {}
  for i, record in ipairs(trx.assault.stats.list_records(track)) do
    if i > record_limit() then
      break
    end
    records[i] = {
      frames = math.floor(record.time * FPS + 0.5),
      attempt_num = record.attempt_num,
    }
  end
  return records
end

local function has_any_time()
  for _, track in ipairs(tracks()) do
    if #trx.assault.stats.list_records(track) > 0 then
      return true
    end
  end
  return false
end

-- As many rows as the screen holds below the box's own furniture, within
-- the bounds the originals keep.
local function assault_visible_rows()
  local limit = record_limit()
  local rows = has_quad() and 3 + 2 * limit or limit
  local available = trx.ui.safe_area.height / trx.ui.text_scale
    - ASSAULT_CHROME
  local result = math.min(
    math.min(rows, MAX_ASSAULT_ROWS),
    math.floor(available / TEXT_HEIGHT)
  )
  return math.max(1, math.min(result, rows))
end

local function assault_rows(state)
  local rows = {}
  local function add(r)
    rows[#rows + 1] = r
  end
  local function track_rows(track)
    local records = records_of(track)
    if #records == 0 then
      add(centered(trx.locale.get("general/stats/assault_no_times_set")))
      return
    end
    for i, record in ipairs(records) do
      local time = trx.locale.format(
        i == 1 and "general/stats/assault_best_time_fmt"
          or "general/stats/assault_other_times_fmt",
        format_record_time(record.frames)
      )
      if trx.game.tr_version == 3 then
        add(centered(time))
      else
        add(
          row(
            state,
            string.format(
              "%2d: %s %d",
              i,
              trx.locale.get("general/stats/assault_finish"),
              record.attempt_num
            ),
            time
          )
        )
      end
    end
  end

  if has_quad() then
    add(centered(trx.locale.get("general/stats/gym_assault_course")))
  end
  track_rows(trx.assault.Track.COURSE)
  if has_quad() then
    add(centered(" "))
    add(centered(trx.locale.get("general/stats/gym_racetrack_course")))
    track_rows(trx.assault.Track.QUAD)
  end
  -- The blank rows follow the room the screen leaves now, while the rows the
  -- list scrolls through were counted when it opened.
  local least = assault_visible_rows()
  while #rows < least do
    add(centered(" "))
  end
  return rows
end

local function title_of(state)
  if state.mode == Mode.LEVEL then
    return state.level.title
  elseif state.mode == Mode.FINAL then
    return trx.locale.get(
      state.level.type == trx.game.LevelType.BONUS
          and "general/stats/bonus_statistics"
        or "general/stats/final_statistics"
    )
  end
  return trx.locale.get("general/stats/assault_title")
end

-- Every row the screen shows. A level screen with nothing to show says the
-- level is complete instead.
local function all_rows(state)
  if state.mode == Mode.ASSAULT then
    return assault_rows(state)
  end
  local rows = {}
  if
    state.mode == Mode.LEVEL and trx.config.get("ui.stats.show_level_header")
  then
    rows[1] = row(
      state,
      trx.locale.get("general/stats/level"),
      trx.locale.format(
        "general/stats/detail_fmt",
        count_completed() + 1,
        #trx.game.levels
      )
    )
  end
  local configured = configured_rows(state)
  if #configured == 0 and state.mode == Mode.LEVEL then
    rows[#rows + 1] = centered(trx.locale.get("general/osd/complete_level"))
  end
  for _, r in ipairs(configured) do
    rows[#rows + 1] = r
  end
  return rows
end

-------------------------------------------------------------------------------
-- The box
-------------------------------------------------------------------------------

local function outer_pad()
  return trx.game.tr_version >= 2 and 3.0 or 2.0
end

local function body_pad_x()
  return trx.game.tr_version >= 2 and 4.0 or 8.0
end

local BODY_PAD_Y = 4.0

local function title_pad_y()
  return trx.game.tr_version >= 2 and 1.0 or 2.0
end

local function scrolls(state)
  return state.mode == Mode.ASSAULT and state.visible < #state.rows
end

-- The room the frame takes around the rows, at the default text size.
local function chrome_height(state)
  local height = 2.0 * (outer_pad() + BODY_PAD_Y)
    + TEXT_HEIGHT
    + 2.0 * title_pad_y()
    + state.look.title_spacing
  if scrolls(state) then
    height = height + 2.0 * HINT_HEIGHT
  end
  return height
end

local function count_layout_rows(state)
  if state.mode == Mode.ASSAULT then
    return state.visible + 1
  end
  return #state.rows
end

-- The engine works the room out in single precision, and a gap that lands on
-- the other side of half a pixel moves a row, so the box is worked out the
-- same way.
local function f32(value)
  return (string.unpack("<f", string.pack("<f", value)))
end

local function row_height()
  local _, height = trx.ui.primitive.measure_text("0", 1.0)
  return height / trx.ui.text_scale
end

-- Where the screen leaves too little room, the gaps between the rows shrink
-- first, then the margin, and then the whole box.
local function layout(state)
  local rows = count_layout_rows(state)
  local rh = row_height()
  local bare = is_bare(state)
  local natural_spacing = bare and BARE_ROW_SPACING or 0.0
  local natural_margin = state.look.window_margin
  local gaps = bare and rows + 1 or 0

  local result = {
    margin = natural_margin,
    spacing = natural_spacing,
    scale = 1.0,
    squeezed = false,
  }

  local function text_height()
    return f32(rows * rh + (bare and rh or chrome_height(state)))
  end

  local available = f32(trx.ui.safe_area.height / trx.ui.text_scale)
  local natural = f32(2.0 * natural_margin + gaps * natural_spacing)
  result.squeezed = text_height() + natural > available
  if not result.squeezed then
    return result
  end

  local min_margin = math.min(f32(rh * MIN_MARGIN_ROWS), natural_margin)
  local min_spacing =
    math.min(f32(rh * f32(MIN_ROW_SPACING_ROWS)), natural_spacing)

  local excess = f32(text_height() + natural - available)
  local spare_spacing = f32(gaps * (natural_spacing - min_spacing))
  result.spacing = gaps > 0
      and f32(natural_spacing - math.min(excess, spare_spacing) / gaps)
    or 0.0

  excess = f32(excess - spare_spacing)
  if excess > 0.0 then
    local spare_margin = f32(2.0 * (natural_margin - min_margin))
    result.margin = f32(natural_margin - math.min(excess, spare_margin) / 2.0)
  end

  local content = f32(text_height() + 2.0 * result.margin)
  if bare then
    content = f32(content + (rows + 1) * result.spacing)
  end
  if content > 0.0 and content > available then
    result.scale = f32(available / content)
  end
  return result
end

local function spacer(w, h)
  return trx.ui.widgets.Custom({
    measure = function()
      local scale = trx.ui.text_scale
      return w * scale, h * scale
    end,
    paint = function() end,
  })
end

-- Where the box is shrunk to fit, the engine shrinks the text and the margins
-- it measures as the box is built, but not the gaps it measures later, so a
-- squeezed box is drawn the way the engine draws it only if each part takes
-- the factor the same way. Text, margins and spacers take it; gaps do not.
local function fitted(state, value)
  return value * state.fit
end

local function label(state, text)
  return trx.ui.widgets.Label({ text = text, scale = state.fit })
end

local function centered_widget(state, text)
  return trx.ui.widgets.Stack({
    align = trx.ui.HAlign.CENTER,
    children = { label(state, text) },
  })
end

-- The bare look draws its words in capitals.
local function bare_text(state, text)
  if is_bare(state) then
    return trx.strings.upper(text)
  end
  return text
end

local function row_widget(state, r)
  if r.text ~= nil then
    return centered_widget(state, bare_text(state, r.text))
  end
  if is_bare(state) then
    return trx.ui.widgets.Stack({
      orientation = trx.ui.Orientation.HORIZONTAL,
      children = {
        label(state, trx.strings.upper(r.key)),
        label(state, " "),
        label(state, r.value),
      },
    })
  end
  return trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    spacing = state.look.row_spacing,
    align = trx.ui.HAlign.DISTRIBUTE,
    children = { label(state, r.key), label(state, r.value) },
  })
end

-- An arrow that says the rows go on past the top or the foot of the box. It
-- keeps its room while there is nothing to point at.
local function hint(state, glyph, anchor, lit)
  local scale = fitted(state, HINT_SCALE)
  return trx.ui.widgets.Custom({
    measure = function()
      local gw = lit and trx.ui.primitive.measure_text(glyph, scale) or 0
      return gw, fitted(state, HINT_HEIGHT) * trx.ui.text_scale
    end,
    paint = function(x, y, w)
      if not lit then
        return
      end
      local gw, gh = trx.ui.primitive.measure_text(glyph, scale)
      local slack = fitted(state, HINT_HEIGHT) * trx.ui.text_scale - gh
      trx.ui.primitive.text(
        glyph,
        x + (w - gw) / 2,
        y + slack * anchor,
        scale,
        0
      )
    end,
  })
end

local function reset_label()
  local key = trx.input.key_name(trx.input.Role.UNBIND_KEY)
  if key == nil then
    return nil
  end
  return string.format(
    "%s: %s",
    trx.locale.get("general/stats/assault_reset_times"),
    trx.locale.format("general/misc/hold_fmt", key)
  )
end

local function reset_button(state)
  local text = reset_label()
  if text == nil then
    return spacer(0, 0)
  end
  local progress = (state.hold - RESET_HOLD_DEBUFF) / RESET_HOLD_MAX
  local bar = trx.ui.widgets.SleekBar({ progress = progress })
  bar.hidden = progress < 0
  local button = trx.ui.widgets.Pad({
    x = fitted(state, RESET_PAD_X),
    y = fitted(state, RESET_PAD_Y),
    child = trx.ui.widgets.Stack({
      spacing = RESET_SPACING,
      align = trx.ui.HAlign.SPAN,
      children = {
        trx.ui.widgets.Label({
          text = text,
          scale = fitted(state, RESET_TEXT_SCALE),
        }),
        bar,
      },
    }),
  })
  button.hidden = not has_any_time()
  return trx.ui.widgets.Stack({
    align = trx.ui.HAlign.CENTER,
    children = { button },
  })
end

local function body_rows(state)
  local children = {}
  local function add(w)
    children[#children + 1] = w
  end
  add(spacer(fitted(state, state.look.min_width), 0))
  if state.mode ~= Mode.ASSAULT then
    for _, r in ipairs(state.rows) do
      add(row_widget(state, r))
    end
    return children
  end

  add(spacer(fitted(state, ASSAULT_MIN_WIDTH), 0))
  local shown = {}
  local last = math.min(state.first + state.visible - 1, #state.rows)
  for i = state.first, last do
    shown[#shown + 1] = row_widget(state, state.rows[i])
  end
  add(trx.ui.widgets.Stack({
    spacing = ASSAULT_ROW_SPACING,
    align = trx.ui.HAlign.SPAN,
    children = shown,
  }))
  add(reset_button(state))
  return children
end

local function window(state)
  local body = body_rows(state)
  if scrolls(state) then
    table.insert(body, 1, hint(state, "\\{arrow up}", 1.5, state.first > 1))
    body[#body + 1] = hint(
      state,
      "\\{arrow down}",
      -1.0,
      state.first + state.visible - 1 < #state.rows
    )
  end

  return trx.ui.widgets.Frame({
    style = trx.ui.FrameStyle.DIALOG,
    z = 170,
    child = trx.ui.widgets.Pad({
      x = fitted(state, outer_pad()),
      y = fitted(state, outer_pad()),
      child = trx.ui.widgets.Stack({
        spacing = state.look.title_spacing,
        align = trx.ui.HAlign.SPAN,
        children = {
          trx.ui.widgets.Frame({
            style = trx.ui.FrameStyle.HEADING,
            z = 160,
            child = trx.ui.widgets.Pad({
              x = fitted(state, 10.0),
              y = fitted(state, title_pad_y()),
              child = centered_widget(state, title_of(state)),
            }),
          }),
          trx.ui.widgets.Pad({
            x = fitted(state, body_pad_x()),
            y = fitted(state, BODY_PAD_Y),
            child = trx.ui.widgets.Stack({
              align = trx.ui.HAlign.SPAN,
              children = body,
            }),
          }),
        },
      }),
    }),
  })
end

local function bare_box(state, spacing)
  local children = { label(state, title_of(state)) }
  for _, w in ipairs(body_rows(state)) do
    children[#children + 1] = w
  end
  return trx.ui.widgets.Stack({
    spacing = spacing,
    align = trx.ui.HAlign.CENTER,
    children = children,
  })
end

local function build(state)
  local box = layout(state)
  state.anchor_y = box.squeezed and 0.5 or state.look.window_y
  state.fit = box.scale
  local dialog = is_bare(state) and bare_box(state, box.spacing)
    or window(state)
  return trx.ui.widgets.Pad({
    x = fitted(state, box.margin),
    y = fitted(state, box.margin),
    child = dialog,
  })
end

-- The box stands in the safe area at the look's height, and a box too large
-- for the room starts at its top left rather than off the screen.
local function place(state)
  return function(w, h)
    local safe = trx.ui.safe_area
    local function offset(slack, ratio)
      return slack >= 0 and f32(slack * ratio) or 0
    end
    return f32(safe.x + offset(f32(safe.width - w), 0.5)),
      f32(safe.y + offset(f32(safe.height - h), state.anchor_y))
  end
end

-------------------------------------------------------------------------------
-- The screens
-------------------------------------------------------------------------------

-- A key that changes with what the box looks like, so the box is built again
-- only when the rows, the scroll or the room for it change.
local function shape_of(state)
  local parts = {
    trx.ui.safe_area.width,
    trx.ui.safe_area.height,
    trx.ui.text_scale,
    state.first or 0,
    state.hold or 0,
    #state.rows,
  }
  for _, r in ipairs(state.rows) do
    parts[#parts + 1] = r.text or (r.key .. "\0" .. r.value)
  end
  return table.concat(parts, "\1")
end

-- The counters run on while the screen is up, as the level clock does in the
-- inventory ring, so they are read again every tick.
local function count(state)
  if state.mode == Mode.LEVEL then
    state.totals = level_totals(state.level)
    state.floor_secrets = has_floor_secrets(state.level)
  elseif state.mode == Mode.FINAL then
    state.totals = final_totals(state.level.type == trx.game.LevelType.BONUS)
  end
  state.rows = all_rows(state)
end

-- The box takes what room the screen leaves once the rest of the interface
-- has taken its own, which is only known as the frame is drawn. It is worked
-- out there, and built again only where the room or the rows have changed.
local function root_of(state)
  ---@type trx.ui.Widget?
  local box
  return trx.ui.widgets.Custom({
    shown = state.shown,
    measure = function()
      local shape = shape_of(state)
      if box == nil or shape ~= state.shape then
        state.shape = shape
        if box ~= nil then
          box:release()
        end
        box = build(state)
      end
      return box:measure()
    end,
    paint = function(x, y, w, h)
      if box ~= nil then
        box:paint(x, y, w, h)
      end
    end,
  })
end

local function refresh(state, layer)
  count(state)
  state.root:wake()
end

local function new_state(level, mode, bare)
  local state = {
    level = level,
    mode = mode,
    bare = bare,
    look = look(),
    first = 1,
    hold = 0,
    armed = true,
  }
  if mode == Mode.ASSAULT then
    state.visible = assault_visible_rows()
  end
  count(state)
  return state
end

local function push(ctx, state, on_input)
  state.root = root_of(state)
  return ctx:push({
    root = state.root,
    place = place(state),
    on_input = on_input,
  })
end

-- The screen between levels and at the end of the game. It reads no keys:
-- the player skips it as they skip the engine's own.
local function open_level_end(ctx)
  local mode = ctx.is_final and Mode.FINAL or Mode.LEVEL
  local state = new_state(ctx.level, mode, ctx.is_bare)
  if mode == Mode.FINAL and #state.rows == 0 then
    return nil
  end
  return push(ctx, state, function(layer)
    refresh(state, layer)
  end)
end

local function scroll(state, step)
  local count = #state.rows
  local last_first = count - state.visible + 1
  if last_first < 1 then
    last_first = 1
  end
  local first = state.first + step
  if first < 1 or first > last_first then
    if not trx.config.get("ui.enable_wraparound") then
      return
    end
    first = first < 1 and last_first or 1
  end
  state.first = first
end

-- Counts how long the reset key is held. Once the hold clears the times, the
-- count waits for the key to be let go.
local function control_reset(state, keys)
  if not has_any_time() then
    state.hold = 0
    return
  end
  local held = keys:held_for(trx.input.Role.UNBIND_KEY)
  if held == 0 then
    state.armed = true
  end
  state.hold = state.armed and held or 0
  if state.hold - RESET_HOLD_DEBUFF > RESET_HOLD_MAX then
    state.armed = false
    state.hold = 0
    for _, track in ipairs(tracks()) do
      trx.assault.stats.clear(track)
    end
    state.first = 1
  end
end

local function in_gym()
  local level = trx.game.current_level
  return level ~= nil and level.type == trx.game.LevelType.GYM
end

-- Whether the entry has come to rest open, which is when the box shows.
local function is_open()
  local anim = trx.inventory_ring.selection_anim()
  return anim ~= nil and anim.frame == anim.goal_frame
end

local function close(ctx, object, confirmed)
  local anim = trx.inventory_ring.selection_anim()
  if anim ~= nil then
    trx.inventory_ring.animate_selection(anim.frame_count - 1, 1)
  end
  if object == trx.catalog.objects.STOPWATCH_OPTION then
    trx.sound.stop(trx.catalog.samples.MENU_STOPWATCH)
  end
  if confirmed then
    ctx:confirm()
  else
    ctx:cancel()
  end
end

-- The compass and the stopwatch in the inventory ring. The compass shows the
-- statistics only where the player asks it to, and is otherwise just put
-- away again.
local function open_ring_entry(ctx)
  local object = ctx.object
  local level = trx.game.current_level
  if level == nil then
    return nil
  end
  local shows_stats = object ~= trx.catalog.objects.COMPASS_OPTION
    or trx.config.get("gameplay.enable_compass_stats")

  local mode = Mode.LEVEL
  if in_gym() and trx.game.tr_version >= 2 then
    mode = Mode.ASSAULT
  end
  local state =
    new_state(level, mode, trx.config.get("ui.stats.style") == "bare")
  state.shown = trx.signal.new(false)

  local layer = push(ctx, state, function(layer, keys)
    if not is_open() then
      return
    end
    -- The rows the best times show are counted once the entry rests open,
    -- against the room the ring leaves then.
    if mode == Mode.ASSAULT and not state.counted_rows then
      state.counted_rows = true
      state.visible = assault_visible_rows()
    end
    state.shown:set(shows_stats)
    if mode == Mode.ASSAULT then
      control_reset(state, keys)
      if keys:pressed(trx.input.Role.MENU_DOWN) then
        scroll(state, 1)
      elseif keys:pressed(trx.input.Role.MENU_UP) then
        scroll(state, -1)
      end
    end
    if keys:pressed(trx.input.Role.MENU_CONFIRM) then
      close(ctx, object, true)
      return
    elseif keys:pressed(trx.input.Role.MENU_BACK) then
      close(ctx, object, false)
      return
    elseif object == trx.catalog.objects.STOPWATCH_OPTION then
      trx.sound.play(trx.catalog.samples.MENU_STOPWATCH)
    end
    refresh(state, layer)
  end)
  return layer
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.STATS, open_level_end)
  trx.ui.screens.define(
    trx.ui.Screen.RING_ENTRY,
    open_ring_entry,
    { object = trx.catalog.objects.COMPASS_OPTION }
  )
  trx.ui.screens.define(
    trx.ui.Screen.RING_ENTRY,
    open_ring_entry,
    { object = trx.catalog.objects.STOPWATCH_OPTION }
  )
end

return M
