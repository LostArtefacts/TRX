-- The `spawn` console command. Ported from src/trx/game/console/cmd/spawn.c.
--
--   /spawn wolf   spawn an object by name, one sector in front of Lara

trx.locale.declare({
  ["console/cmd/spawn/fail"] = "Failed to spawn requested object",
  ["console/cmd/spawn/help"] = "Spawns an object by name in front of Lara.",
  ["console/cmd/spawn/invalid"] = "Unknown object: %s",
  ["console/cmd/spawn/name_help"] = "the object to spawn, by name",
  ["console/cmd/spawn/success"] = "Requested object spawned near Lara",
})

-- Tries straight ahead first, then 45 degrees either side, and returns the
-- first spot that lands in valid room geometry.
---@param lara trx.items.Item
local function find_target_pos(lara)
  for _, side in ipairs({ -1, 0, 1 }) do
    local angle = lara.rot.y + side * trx.math.DEG_45
    local dist = trx.math.WALL_L
    local candidate = {
      x = lara.pos.x + math.floor(trx.math.sin(angle) * dist),
      y = lara.pos.y,
      z = lara.pos.z + math.floor(trx.math.cos(angle) * dist),
    }
    local pos, room_num = trx.rooms.find_valid_pos(candidate, lara.room_num)
    if pos ~= nil then
      return pos, room_num
    end
  end
  return nil
end

trx.console.register({
  name = "spawn",
  help = "console/cmd/spawn/help",
  args = function(parser)
    parser:rest("name", {
      help = "console/cmd/spawn/name_help",
      suggest = function()
        return trx.objects.query:spawnable():names()
      end,
    })
  end,
  run = function(args)
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    local lara = trx.lara.item
    if lara == nil or lara.hit_points <= 0 then
      return trx.console.Result.UNAVAILABLE
    end

    local pos = find_target_pos(lara)
    if pos == nil then
      return trx.console.Result.FAILURE,
        trx.locale.get("console/cmd/spawn/fail")
    end

    -- The strongest matches: one object for a name only it answers to, the
    -- whole family for a group name like "pickup". Spawn one of them at random,
    -- so /spawn pickup varies.
    local ids = trx.objects.query:spawnable():by_name(args.name):best()
    local object_id = #ids > 0 and ids[math.random(#ids)] or nil
    if object_id == nil then
      return trx.console.Result.FAILURE,
        trx.locale.format("console/cmd/spawn/invalid", args.name)
    end

    -- Face the spawned item back towards Lara.
    local angle = trx.math.atan(lara.pos.z - pos.z, lara.pos.x - pos.x)

    local item = trx.items.spawn(object_id, pos, angle, { activate = true })
    if item ~= nil then
      return trx.console.Result.OK, trx.locale.get("console/cmd/spawn/success")
    end

    return trx.console.Result.FAILURE
  end,
})
