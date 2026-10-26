-- The numbers that the statistics screen shows: the counters of a level and
-- of the whole game, the best times on the assault course, and how each is
-- written.

local M = {}

local FPS = trx.game.LOGIC_FPS

local MAX_ASSAULT_TIMES = 10

function M.format_time(state, frames)
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

function M.format_record_time(frames)
  local total = frames // FPS
  local minutes = total // 60 % 60
  local seconds = total % 60
  local rest = frames % FPS
  if trx.game.tr_version >= 3 then
    return string.format("%d:%02d.%02d", minutes, seconds, rest * 100 // FPS)
  end
  return string.format("%02d:%02d.%-2d", minutes, seconds, rest * 10 // FPS)
end

function M.format_distance(distance)
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
function M.level_totals(level)
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
function M.final_totals(include_bonus)
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
      or level.type == trx.game.LevelType.BONUS and include_bonus
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

function M.count_completed()
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
function M.icon_secrets(level)
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
    return trx.locale.text("general/stats/none")
  end
  return table.concat(parts)
end

-- A secret that is a place to reach has no glyph to draw, so a level with one
-- counts its secrets instead of drawing them.
function M.has_floor_secrets(level)
  for _, secret in ipairs(level.stats:secret_list()) do
    if secret.icon == nil then
      return true
    end
  end
  return false
end

function M.has_quad()
  return trx.game.tr_version >= 3
end

function M.record_limit()
  return trx.game.tr_version >= 3 and 3 or MAX_ASSAULT_TIMES
end

function M.tracks()
  local result = { trx.assault.Track.COURSE }
  if M.has_quad() then
    result[#result + 1] = trx.assault.Track.QUAD
  end
  return result
end

function M.records_of(track)
  local records = {}
  for i, record in ipairs(trx.assault.stats.list_records(track)) do
    if i > M.record_limit() then
      break
    end
    records[i] = {
      frames = math.floor(record.time * FPS + 0.5),
      attempt_num = record.attempt_num,
    }
  end
  return records
end

function M.has_any_time()
  for _, track in ipairs(M.tracks()) do
    if #trx.assault.stats.list_records(track) > 0 then
      return true
    end
  end
  return false
end

return M
