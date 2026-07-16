-- Toggles the flip map.
--
-- Usages:
--   /flip        toggle it
--   /flip on     force it on
--   /flip off    force it off

local function run(args)
  if not trx.game.is_playable then
    return trx.console.Result.UNAVAILABLE
  end

  local target
  if args == "" then
    target = not trx.rooms.flipped
  else
    target = trx.strings.parse_bool(args)
    if target == nil then
      return trx.console.Result.BAD_INVOCATION
    end
  end

  if trx.rooms.flipped == target then
    if target then
      trx.console.log.warning(trx.locale.get("console/cmd/flipmap/already_on"))
    else
      trx.console.log.warning(
        trx.locale.get("console/cmd/flipmap/already_off")
      )
    end
    return trx.console.Result.OK
  end

  trx.rooms.flip()
  if target then
    return trx.console.Result.OK, trx.locale.get("console/cmd/flipmap/on")
  end
  return trx.console.Result.OK, trx.locale.get("console/cmd/flipmap/off")
end

for _, name in ipairs({ "flip", "flipmap" }) do
  trx.console.register({
    name = name,
    help = "console/cmd/flipmap/help",
    run = run,
  })
end
