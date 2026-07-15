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

local function run(args, underwater)
  local room, bad = room_from_args(args)
  if bad then
    return trx.console.Result.BAD_INVOCATION
  end
  if room == nil then
    return trx.console.Result.UNAVAILABLE
  end

  room.underwater = underwater
  return trx.console.Result.OK
end

trx.console.register({
  name = "flood",
  help = "console/cmd/flood/help",
  run = function(args)
    return run(args, true)
  end,
})

trx.console.register({
  name = "drain",
  help = "console/cmd/drain/help",
  run = function(args)
    return run(args, false)
  end,
})
