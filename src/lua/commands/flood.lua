-- Floods or drains a room.
--
-- Usages:
--   /flood        flood the room Lara is in
--   /flood 12     flood a room by number, as the console counts them, from zero
--   /drain        and the same, taking the water back

-- The console counts rooms from zero, as the engine does; Lua counts from one.
local function room_from_args(args)
  if args == "" then
    local lara = trx.lara.item
    if lara == nil then
      return nil
    end
    return trx.rooms.get(lara.room_num)
  end

  local num = tonumber(args)
  if num == nil or num % 1 ~= 0 then
    return nil, true
  end
  return trx.rooms.get(num + 1)
end

trx.console.register({
  name = "flood",
  help = "console/cmd/flood/help",
  run = function(args)
    local room, bad = room_from_args(args)
    if bad then
      return trx.console.Result.BAD_INVOCATION
    end
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
  run = function(args)
    local room, bad = room_from_args(args)
    if bad then
      return trx.console.Result.BAD_INVOCATION
    end
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
