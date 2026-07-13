-- Heals Lara back to full health, and removes poison and fire.
--
-- Usages:
--   /heal

trx.console.register({
  name = "heal",
  help = "console/cmd/heal/help",
  run = function(args)
    if args ~= "" then
      return trx.console.Result.BAD_INVOCATION
    end
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    local lara = trx.lara.item
    local was_full = lara.hit_points == lara.max_hit_points

    lara.hit_points = lara.max_hit_points
    trx.lara.cure_poison()
    trx.lara.extinguish()

    -- An unhurt Lara is still cured and extinguished: the command worked, and
    -- only has nothing to say about her hit points.
    if was_full then
      trx.console.log.warning(trx.locale.get("console/cmd/heal/already_full_hp"))
      return trx.console.Result.OK
    end
    return trx.console.Result.OK, trx.locale.get("console/cmd/heal/success")
  end,
})
