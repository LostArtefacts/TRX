-- Reads or sets Lara's poison.
--
-- Usages:
--   /poison         report her current poison and its target
--   /poison 100     set the poison level
--   /poison -t 100  set the target reservoir instead (TR4 only)

trx.console.register({
  name = "poison",
  help = "console/cmd/poison/help",
  args = function(parser)
    -- -t sets the target reservoir instead of the current level (TR4 only).
    parser:flag("target", { short = "-t", long = "--target" })
    parser:positional("value", { type = "integer", optional = true })
  end,
  run = function(args)
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    if not args.target and args.value == nil then
      return trx.console.Result.OK,
        trx.locale.format(
          "console/cmd/poison/get",
          trx.lara.poison,
          trx.lara.poison_target
        )
    end

    local value = args.value
    if value == nil then
      return trx.console.Result.BAD_INVOCATION
    end

    if args.target then
      if trx.game.version ~= 4 then
        return trx.console.Result.UNAVAILABLE
      end
      value = math.max(0, math.min(value, 4096))
      trx.lara.poison_target = value
      return trx.console.Result.OK,
        trx.locale.format("console/cmd/poison/target_set", value)
    end

    value = math.max(0, math.min(value, trx.game.version == 4 and 4096 or 256))
    trx.lara.poison = value
    return trx.console.Result.OK,
      trx.locale.format("console/cmd/poison/set", value)
  end,
})
