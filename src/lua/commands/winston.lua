-- Summons Winston next to Lara, teleporting him if he is already about.

trx.locale.declare({
  ["console/cmd/winston/dead"] = "Your butler is dead. You monster!",
  ["console/cmd/winston/spawn_failed"] = "Failed to summon Winston",
  ["console/cmd/winston/spawned"] = "Summoned Winston near Lara",
  ["console/cmd/winston/teleported"] = "Summoned Winston near Lara",
})

local STEP_L = trx.math.WALL_L // 4

-- Both butlers: army Winston counts as Winston for every purpose this
-- command has.
local BUTLERS = {
  trx.catalog.objects.winston,
  trx.catalog.objects.winston_army,
}

local function is_butler(item)
  for _, object_id in ipairs(BUTLERS) do
    if item.object_id == object_id then
      return true
    end
  end
  return false
end

-- Winston cannot be hurt by anything the game does, so his death is always the
-- kill cheat's doing, and the cheat leaves nothing behind to read: it removes
-- him outright. Kept per butler, so killing one is not held against the other.
-- He counts as killed when he is removed or stops with no health left; army
-- Winston clearing the ordinary Winston away on arrival leaves him unharmed.
local killed = {}

local function note_death(item)
  if is_butler(item) and item.hit_points <= 0 then
    killed[item.object_id] = true
  end
end

trx.events.on_destroy(note_death)
trx.events.on_leave_sim(note_death)

trx.events.on_game_start(function()
  killed = {}
end)

-- The butler on his feet, whichever of the two it is.
local function running_butler()
  for _, object_id in ipairs(BUTLERS) do
    for _, item in ipairs(trx.items.query:of_object(object_id):matches()) do
      if item.is_simulated then
        return item
      end
    end
  end
  return nil
end

local function bring(item, target, lara)
  item.pos = target
  local rot = item.rot
  rot.y = lara.rot.y
  item.rot = rot
end

trx.console.register({
  name = "teatime",
  run = function()
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    local lara = trx.lara.item
    if lara == nil or lara.hit_points <= 0 then
      return trx.console.Result.UNAVAILABLE
    end

    local target = trx.rooms.find_valid_pos({
      x = lara.pos.x + STEP_L,
      y = lara.pos.y - trx.math.WALL_L,
      z = lara.pos.z + STEP_L,
    }, lara.room_num)
    if target == nil then
      return trx.console.Result.FAILURE,
        trx.locale.get("console/cmd/winston/spawn_failed")
    end

    -- A butler on his feet is the one who answers, whichever of the two he is.
    local running = running_butler()
    if running ~= nil then
      bring(running, target, lara)
      return trx.console.Result.OK,
        trx.locale.get("console/cmd/winston/teleported")
    end

    -- Nobody is up, so it falls to the butler proper: the one the level placed,
    -- else a new one. Army Winston is never started here, because doing so
    -- clears Winston away.
    local winston_id = trx.catalog.objects.winston
    local winston_obj = trx.objects[winston_id]
    if
      winston_obj ~= nil
      and winston_obj.loaded
      and not killed[winston_id]
    then
      local placed = trx.items.query:of_object(winston_id):first()
      if placed == nil then
        local winston =
          trx.items.spawn(winston_id, target, lara.rot.y, { activate = true })
        if winston == nil then
          return trx.console.Result.FAILURE
        end
        return trx.console.Result.OK,
          trx.locale.get("console/cmd/winston/spawned")
      end

      if not placed.is_finished then
        placed:activate()
      end
      bring(placed, target, lara)
      return trx.console.Result.OK,
        trx.locale.get("console/cmd/winston/teleported")
    end

    -- Nobody to summon. If that is Lara's own doing, she hears about it.
    if next(killed) ~= nil then
      trx.music.stop()
      return trx.console.Result.FAILURE,
        trx.locale.get("console/cmd/winston/dead")
    end
    return trx.console.Result.UNAVAILABLE
  end,
})
