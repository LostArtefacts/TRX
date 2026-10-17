-- The rows that the statistics screen lists, which the settings and the look
-- of each game decide.

local counts = require("common.ui.screens.stats.counts")
local metrics = require("common.ui.metrics")

local M = {}

trx.locale.declare({
  ["settings/ui.stats.show_ammo/title"] = "Ammo hits/used",
  ["settings/ui.stats.show_ammo/description"] = "Shows the ammo row in the level statistics.",
  ["settings/ui.stats.show_crystals/title"] = "Crystals",
  ["settings/ui.stats.show_crystals/description"] = "Shows the crystals row in the level statistics.",
  ["settings/ui.stats.show_deaths/title"] = "Deaths",
  ["settings/ui.stats.show_deaths/description"] = "Shows Lara's deaths in the compass statistics and in the level "
    .. "statistics. Death count is updated in the currently loaded save as "
    .. "soon as Lara dies.",
  ["settings/ui.stats.show_distance_travelled/title"] = "Distance traveled",
  ["settings/ui.stats.show_distance_travelled/description"] = "Shows the distance traveled row in the level statistics.",
  ["settings/ui.stats.show_kills/title"] = "Kills",
  ["settings/ui.stats.show_kills/description"] = "Shows the kills row in the level statistics.",
  ["settings/ui.stats.show_level_header/title"] = "Level counter",
  ["settings/ui.stats.show_level_header/description"] = "Shows the current level number at the top of the level statistics.",
  ["settings/ui.stats.show_medipacks_used/title"] = "Health packs used",
  ["settings/ui.stats.show_medipacks_used/description"] = "Shows the health packs used row in the level statistics.",
  ["settings/ui.stats.show_pickups/title"] = "Pickups",
  ["settings/ui.stats.show_pickups/description"] = "Shows the pickups row in the level statistics.",
  ["settings/ui.stats.show_secrets/title"] = "Secrets found",
  ["settings/ui.stats.show_secrets/description"] = "Shows the secrets found row in the level statistics.",
  ["settings/ui.stats.show_time_taken/title"] = "Time taken",
  ["settings/ui.stats.show_time_taken/description"] = "Shows the time taken row in the level statistics.",
  ["settings/ui.stats.show_totals/title"] = "Show totals",
  ["settings/ui.stats.show_totals/description"] = "Shows totals next to stats when applicable. Secrets remain unaffected "
    .. "by this setting.",
})

trx.config.declare({
  key = "ui.stats.show_ammo",
  kind = "boolean",
  default = true,
})
trx.config.declare({
  key = "ui.stats.show_crystals",
  kind = "boolean",
  default = false,
})
trx.config.declare({
  key = "ui.stats.show_deaths",
  kind = "boolean",
  default = true,
})
trx.config.declare({
  key = "ui.stats.show_distance_travelled",
  kind = "boolean",
  default = true,
})
trx.config.declare({
  key = "ui.stats.show_kills",
  kind = "boolean",
  default = true,
})
trx.config.declare({
  key = "ui.stats.show_level_header",
  kind = "boolean",
  default = false,
})
trx.config.declare({
  key = "ui.stats.show_medipacks_used",
  kind = "boolean",
  default = true,
})
trx.config.declare({
  key = "ui.stats.show_pickups",
  kind = "boolean",
  default = true,
})
trx.config.declare({
  key = "ui.stats.show_secrets",
  kind = "boolean",
  default = true,
})
trx.config.declare({
  key = "ui.stats.show_time_taken",
  kind = "boolean",
  default = true,
})
trx.config.declare({
  key = "ui.stats.show_totals",
  kind = "boolean",
  default = true,
})

M.Mode = {
  LEVEL = "level",
  FINAL = "final",
  ASSAULT = "assault",
}

local Mode = M.Mode

local MAX_ASSAULT_ROWS = 7
local ASSAULT_CHROME = 80.0

-- TR1 sits its box in the middle of the screen; the later games stand it on
-- the foot of the screen, inside a margin.
function M.look()
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

function M.is_bare(state)
  return state.bare
end

-- Pickup crystals exist only to be counted, so their row is not optional.
local function show_crystals()
  return trx.config.get("ui.stats.show_crystals")
    or trx.config.get("gameplay.save_crystal_mode") == "pickup"
end

local function row(key, value)
  return { key = key, value = value }
end

local function centered(text)
  return { text = text }
end

local function auto_secrets_row(state)
  local totals = state.totals
  if state.mode == Mode.FINAL or state.floor_secrets then
    return row(
      trx.locale.get("general/stats/secrets"),
      trx.locale.format(
        "general/stats/detail_fmt",
        totals.counts.secrets,
        totals.maxes.secrets
      )
    )
  end
  return row(
    trx.locale.get("general/stats/secrets"),
    counts.icon_secrets(state.level)
  )
end

local function count_row(state, key, name)
  local fmt = trx.config.get("ui.stats.show_totals")
      and "general/stats/detail_fmt"
    or "general/stats/basic_fmt"
  return row(
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
    add(row(trx.locale.get("general/stats/time_taken"), state.time_text))
  end

  if M.is_bare(state) then
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
    if M.is_bare(state) then
      add(
        row(
          trx.locale.get("general/stats/ammo_used"),
          string.format("%d", totals.ammo_used)
        )
      )
      add(
        row(
          trx.locale.get("general/stats/ammo_hits"),
          string.format("%d", totals.ammo_hits)
        )
      )
    else
      add(
        row(
          trx.locale.get("general/stats/ammo"),
          trx.locale.format(
            "general/stats/ammo_fmt",
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
        trx.locale.get("general/stats/medipacks_used"),
        string.format("%.1f", totals.medipacks_used)
      )
    )
  end
  if get("ui.stats.show_distance_travelled") then
    add(
      row(
        trx.locale.get("general/stats/distance_travelled"),
        counts.format_distance(totals.distance_travelled)
      )
    )
  end
  -- Deaths stay with the level they happen on, so a player who dies in one
  -- level and reloads an earlier one still sees them counted.
  if get("ui.stats.show_deaths") and totals.deaths >= 0 then
    add(
      row(
        trx.locale.get("general/stats/deaths"),
        trx.locale.format("general/stats/basic_fmt", totals.deaths)
      )
    )
  end
  return rows
end

-- As many rows as the screen holds below the box's own furniture, within
-- the bounds the originals keep.
function M.assault_visible_rows()
  local limit = counts.record_limit()
  local rows = counts.has_quad() and 3 + 2 * limit or limit
  local available = trx.ui.safe_area.height / trx.ui.text_scale
    - ASSAULT_CHROME
  local result = math.min(
    math.min(rows, MAX_ASSAULT_ROWS),
    math.floor(available / metrics.TEXT_HEIGHT)
  )
  return math.max(1, math.min(result, rows))
end

local function assault_rows(state)
  local rows = {}
  local function add(r)
    rows[#rows + 1] = r
  end
  local function track_rows(track)
    local records = counts.records_of(track)
    if #records == 0 then
      add(centered(trx.locale.get("general/stats/assault_no_times_set")))
      return
    end
    for i, record in ipairs(records) do
      local time = trx.locale.format(
        i == 1 and "general/stats/assault_best_time_fmt"
          or "general/stats/assault_other_times_fmt",
        counts.format_record_time(record.frames)
      )
      if trx.game.tr_version == 3 then
        add(centered(time))
      else
        add(
          row(
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

  if counts.has_quad() then
    add(centered(trx.locale.get("general/stats/gym_assault_course")))
  end
  track_rows(trx.assault.Track.COURSE)
  if counts.has_quad() then
    add(centered(" "))
    add(centered(trx.locale.get("general/stats/gym_racetrack_course")))
    track_rows(trx.assault.Track.QUAD)
  end
  -- The blank rows follow the room the screen leaves now, while the rows the
  -- list scrolls through were counted when it opened.
  local least = M.assault_visible_rows()
  while #rows < least do
    add(centered(" "))
  end
  return rows
end

function M.title_of(state)
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
function M.all_rows(state)
  if state.mode == Mode.ASSAULT then
    return assault_rows(state)
  end
  local rows = {}
  if
    state.mode == Mode.LEVEL and trx.config.get("ui.stats.show_level_header")
  then
    rows[1] = row(
      trx.locale.get("general/stats/level"),
      trx.locale.format(
        "general/stats/detail_fmt",
        counts.count_completed() + 1,
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

return M
