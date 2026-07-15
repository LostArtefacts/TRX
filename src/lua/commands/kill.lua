-- Kills hostile creatures, by proximity or by name.
--
-- Usages:
--   /kill        the hostiles within one sector, else the nearest within five
--   /kill all    every hostile in the level
--   /kill wolf   every creature matching an object name

local WALL_L = trx.math.WALL_L

-- What a name may reach: everything that fights, allies included, so
-- /kill winston works the way /kill wolf does.
local function targetable()
  local q = trx.objects.query
  return (q:creature() | q:loyal()):loaded()
end

-- Which objects are allies does not change over a session, so the set is built
-- the once.
local loyal_ids
local function is_loyal(object_id)
  if loyal_ids == nil then
    loyal_ids = {}
    for _, id in ipairs(trx.objects.query:loyal():ids()) do
      loyal_ids[id] = true
    end
  end
  return loyal_ids[object_id] == true
end

-- Skip anything already dead, turn the allies on Lara if this was one of them,
-- then blow it up.
local function cheat_kill(item)
  if item.is_killed then
    return false
  end
  if not item.is_alive and not item.is_in_play then
    return false
  end

  if is_loyal(item.object_id) then
    trx.creatures.hostile_allies = true
  end

  trx.sound.play(trx.catalog.samples.EXPLOSION_1, { pos = item.pos })
  item:die(true)
  return true
end

local function hostiles()
  local result = {}
  for _, item in ipairs(trx.items.query:matches()) do
    if item.is_hostile then
      result[#result + 1] = item
    end
  end
  return result
end

-- The combat end object wakes a boss once everything else in the level is dead,
-- and needs one still standing to do it. So the bosses survive /kill all until
-- the sequence has begun, and a second /kill all finishes the fight.
local function is_protected(item)
  local boss_id = trx.catalog.objects.cult_3
  if item.object_id ~= boss_id then
    return false
  end
  if
    trx.items.query:of_object(trx.catalog.objects.combat_end):count() == 0
  then
    return false
  end
  for _, boss in ipairs(trx.items.query:of_object(boss_id):matches()) do
    if boss.is_simulated or boss.is_finished or boss.is_killed then
      return false
    end
  end
  return true
end

local function kill_all()
  local killed = 0
  for _, item in ipairs(hostiles()) do
    if not is_protected(item) and cheat_kill(item) then
      killed = killed + 1
    end
  end
  if killed == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/kill/all_fail")
  end
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/kill/all", killed)
end

local function kill_nearest()
  local lara_pos = trx.lara.item.pos

  -- One scan: kill everything within a sector, and remember the nearest within
  -- five in case nothing was that close.
  local killed = 0
  local best, best_dist
  for _, item in ipairs(hostiles()) do
    local dist = item:distance_to(lara_pos)
    if dist <= WALL_L then
      if cheat_kill(item) then
        killed = killed + 1
      end
    elseif dist <= 5 * WALL_L and (best_dist == nil or dist < best_dist) then
      best, best_dist = item, dist
    end
  end

  if killed == 0 and best ~= nil and cheat_kill(best) then
    killed = 1
  end

  if killed == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/kill/nearest_fail")
  end
  return trx.console.Result.OK, trx.locale.get("console/cmd/kill/nearest")
end

local function kill_type(name)
  local ids = targetable():by_name(name):best()
  if #ids == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/kill/invalid_object")
  end

  local matched, killed = false, 0
  for _, id in ipairs(ids) do
    for _, item in ipairs(trx.items.query:of_object(id):matches()) do
      matched = true
      if cheat_kill(item) then
        killed = killed + 1
      end
    end
  end

  if not matched then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/kill/invalid_object")
  end
  if killed == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/kill/object_not_found")
  end
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/kill/all", killed)
end

trx.console.register({
  name = "kill",
  help = "console/cmd/kill/help",
  args = function(parser)
    -- "all" or an object name; a bare /kill takes the nearest hostile.
    parser:rest("target", {
      optional = true,
      suggest = function()
        return targetable():names()
      end,
    })
  end,
  run = function(args)
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    local target = args.target
    if target == nil then
      return kill_nearest()
    end

    if target:lower() == "all" then
      return kill_all()
    end

    return kill_type(target)
  end,
})
