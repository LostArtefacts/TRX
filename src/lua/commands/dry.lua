-- Dries Lara off, stopping her dripping after a swim.
--
-- Usages:
--   /dry

trx.console.register({
  name = "dry",
  help = "console/cmd/dry/help",
  run = function(args)
    if args ~= "" then
      return trx.console.Result.BAD_INVOCATION
    end
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end
    if not trx.config.get("visuals.enable_droplets") then
      return trx.console.Result.UNAVAILABLE
    end

    if trx.lara.water_status == trx.lara.WaterState.UNDERWATER then
      return trx.console.Result.FAILURE,
        trx.locale.get("console/cmd/dry/underwater")
    end

    if not trx.lara.is_wet then
      trx.console.log.warning(trx.locale.get("console/cmd/dry/already_dry"))
      return trx.console.Result.OK
    end

    trx.lara.dry()
    return trx.console.Result.OK, trx.locale.get("console/cmd/dry/success")
  end,
})
