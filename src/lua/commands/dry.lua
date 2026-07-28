-- Dries Lara off, stopping her dripping after a swim.
--
-- Usages:
--   /dry

trx.locale.declare({
  ["console/cmd/dry/already_dry"] = "Lara's already bone dry",
  ["console/cmd/dry/help"] = "Dries Lara off, stopping her dripping after a swim.",
  ["console/cmd/dry/success"] = "Lara's been toweled off - no more dripping",
  ["console/cmd/dry/underwater"] = "Lara's a bit busy being underwater right now",
})

trx.console.register({
  name = "dry",
  help = "console/cmd/dry/help",
  run = function()
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
