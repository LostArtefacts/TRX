-- Reports which level Lara is in and where she stands.

-- A full turn in the engine's angle units.
local DEG_360 = trx.math.DEG_90 * 4

-- The number the level goes by on screen. Cutscenes and demos count from one;
-- numbered levels already do when the game has a gym at ordinal zero, and need a
-- nudge when it does not.
local function level_line(level)
  if level.type == trx.game.LevelType.CUTSCENE then
    return trx.locale.format(
      "console/cmd/pos/level_fmt_cutscene",
      level.num + 1
    )
  end
  if level.type == trx.game.LevelType.DEMO then
    return trx.locale.format("console/cmd/pos/level_fmt_demo", level.num + 1)
  end
  local reindex = trx.game.gym == nil and 1 or 0
  return trx.locale.format("console/cmd/pos/level_fmt", level.num + reindex)
end

local function lara_line()
  local lara = trx.lara.item
  if lara == nil then
    return trx.locale.get("console/cmd/pos/lara_missing"), "\n"
  end

  -- Report the room the player can actually reach when the map is flipped.
  local room_num = lara.room_num
  local room = trx.rooms[room_num]
  if trx.rooms.flipped and room.flipped_room ~= nil then
    room_num = room.flipped_room.num
  end

  local wall = trx.math.WALL_L
  local details = trx.locale.format(
    "console/cmd/pos/lara_pos_fmt",
    room_num,
    lara.pos.x / wall,
    lara.pos.y / wall,
    lara.pos.z / wall,
    lara.rot.x * 360 / DEG_360,
    lara.rot.y * 360 / DEG_360,
    lara.rot.z * 360 / DEG_360
  )
  return details, "  "
end

trx.console.register({
  name = "pos",
  help = "console/cmd/pos/help",
  run = function(args)
    if not trx.game.is_loaded then
      return trx.console.Result.UNAVAILABLE
    end

    if args ~= "" then
      return trx.console.Result.BAD_INVOCATION
    end

    local level = trx.game.current_level
    if level == nil or level.type == trx.game.LevelType.TITLE then
      return trx.console.Result.UNAVAILABLE
    end

    local title = level_line(level)
    local details, glue = lara_line()

    if level.title ~= nil and level.title ~= title then
      title = title .. " (" .. level.title .. ")"
    end
    trx.console.log(title .. glue .. details)
    return trx.console.Result.OK
  end,
})
