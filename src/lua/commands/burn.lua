-- Sets Lara on fire, or puts her out.
--
-- Usages:
--   /burn        toggle
--   /burn on     force it on
--   /burn off    force it off

trx.console.register({
  name = "burn",
  help = "console/cmd/burn/help",
  run = function(args)
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    -- She is untouchable in the fly cheat, so lighting her there does nothing.
    if trx.lara.water_status == trx.lara.WaterState.CHEAT then
      return trx.console.Result.UNAVAILABLE
    end

    local target
    if args == "" then
      target = not trx.lara.is_burning
    else
      target = trx.strings.parse_bool(args)
      if target == nil then
        return trx.console.Result.BAD_INVOCATION
      end
    end

    if trx.lara.is_burning == target then
      if target then
        trx.console.log.warning(trx.locale.get("console/cmd/burn/already_on"))
      else
        trx.console.log.warning(trx.locale.get("console/cmd/burn/already_off"))
      end
      return trx.console.Result.OK
    end

    trx.lara.is_burning = target
    if target then
      return trx.console.Result.OK, trx.locale.get("console/cmd/burn/on")
    end
    return trx.console.Result.OK, trx.locale.get("console/cmd/burn/off")
  end,
})
