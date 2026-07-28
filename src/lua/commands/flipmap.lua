-- Toggles the flip map.
--
-- Usages:
--   /flip        toggle it
--   /flip on     force it on
--   /flip off    force it off

trx.locale.declare({
  ["console/cmd/flipmap/already_off"] = "Flipmap is already OFF",
  ["console/cmd/flipmap/already_on"] = "Flipmap is already ON",
  ["console/cmd/flipmap/help"] = "Toggles the flip map.",
  ["console/cmd/flipmap/off"] = "Flipmap set to OFF",
  ["console/cmd/flipmap/on"] = "Flipmap set to ON",
})

local function run(args)
  if not trx.game.is_playable then
    return trx.console.Result.UNAVAILABLE
  end

  local target = args.state
  if target == nil then
    target = not trx.rooms.flipped
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

trx.console.register({
  name = "flip",
  aliases = { "flipmap" },
  help = "console/cmd/flipmap/help",
  args = function(parser)
    parser:positional("state", { type = "boolean", optional = true })
  end,
  run = run,
})
