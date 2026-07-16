-- Summons Winston next to Lara, teleporting him if he already exists.

local STEP_L = trx.math.WALL_L // 4

-- Brings an existing Winston to Lara. Returns whether one was found - a dead one
-- counts, and reports itself.
local function summon_existing(object_id, target, lara)
  local items = trx.items.find({ object_id = object_id })
  local winston = items[1]
  if winston == nil then
    return false
  end

  if
    winston.status == trx.items.Status.INVISIBLE
    or winston.status == trx.items.Status.INACTIVE
  then
    winston:activate()
  elseif winston.is_killed then
    trx.music.stop()
    trx.console.log(trx.locale.get("console/cmd/winston/dead"))
    return true
  end

  winston.pos = target
  local rot = winston.rot
  rot.y = lara.rot.y
  winston.rot = rot
  return true
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

    local winston_obj = trx.objects[trx.catalog.objects.winston]
    if winston_obj == nil or not winston_obj.loaded then
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

    if trx.lara.killed_loyal_item then
      trx.music.stop()
      return trx.console.Result.FAILURE,
        trx.locale.get("console/cmd/winston/dead")
    end

    if
      summon_existing(trx.catalog.objects.winston_army, target, lara)
      or summon_existing(trx.catalog.objects.winston, target, lara)
    then
      return trx.console.Result.OK,
        trx.locale.get("console/cmd/winston/teleported")
    end

    local winston = trx.items.spawn(
      trx.catalog.objects.winston,
      target,
      lara.rot.y,
      { activate = true }
    )
    if winston == nil then
      return trx.console.Result.FAILURE
    end
    return trx.console.Result.OK, trx.locale.get("console/cmd/winston/spawned")
  end,
})
