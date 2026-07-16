-- Reads or sets Lara's poison.
--
-- Usages:
--   /poison         report her current poison and its target
--   /poison 100     set the poison level
--   /poison -t 100  set the target reservoir instead (TR4 only)

trx.console.register({
  name = "poison",
  help = "console/cmd/poison/help",
  run = function(args)
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    if args == "" then
      return trx.console.Result.OK,
        trx.locale.format(
          "console/cmd/poison/get",
          trx.lara.poison,
          trx.lara.poison_target
        )
    end

    -- -t, as a standalone token, sets the target reservoir instead of the
    -- current level. Anything left over is the value.
    local set_target = false
    local rest = {}
    for tok in args:gmatch("%S+") do
      if tok == "-t" then
        set_target = true
      else
        rest[#rest + 1] = tok
      end
    end

    local value = #rest == 1 and tonumber(rest[1]) or nil
    if value == nil or value % 1 ~= 0 then
      return trx.console.Result.BAD_INVOCATION
    end

    if set_target then
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
