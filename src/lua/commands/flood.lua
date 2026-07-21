-- Floods or drains a room.
--
-- Usages:
--   /flood        flood the room Lara is in
--   /flood 12     flood a room by number, as the console counts them, from zero
--   /drain        and the same, taking the water back

-- The console counts rooms from zero, as the engine does; Lua counts from one.
local function room_for(num)
  if num == nil then
    local lara = trx.lara.item
    if lara == nil then
      return nil
    end
    return trx.rooms.get(lara.room_num)
  end
  return trx.rooms.get(num + 1)
end

local function args(parser)
  parser:positional("room", { type = "integer", optional = true })
end

trx.console.register({
  name = "flood",
  help = "console/cmd/flood/help",
  args = args,
  run = function(args)
    local room = room_for(args.room)
    if room == nil then
      return trx.console.Result.UNAVAILABLE
    end
    if room.underwater then
      trx.console.log.warning(
        trx.locale.format("console/cmd/flood/already", room.num)
      )
      return trx.console.Result.OK
    end
    room.underwater = true
    return trx.console.Result.OK,
      trx.locale.format("console/cmd/flood/done", room.num)
  end,
})

trx.console.register({
  name = "drain",
  help = "console/cmd/drain/help",
  args = args,
  run = function(args)
    local room = room_for(args.room)
    if room == nil then
      return trx.console.Result.UNAVAILABLE
    end
    if not room.underwater then
      trx.console.log.warning(
        trx.locale.format("console/cmd/drain/already", room.num)
      )
      return trx.console.Result.OK
    end
    room.underwater = false
    return trx.console.Result.OK,
      trx.locale.format("console/cmd/drain/done", room.num)
  end,
})
