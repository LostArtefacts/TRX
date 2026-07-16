-- Reads or sets Lara's health.
--
-- Usages:
--   /hp        report her current health
--   /hp 500    set it

trx.console.register({
  name = "hp",
  help = "console/cmd/hp/help",
  run = function(args)
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    local lara = trx.lara.item
    if args == "" then
      return trx.console.Result.OK,
        trx.locale.format("console/cmd/hp/get", lara.hit_points)
    end

    local hp = tonumber(args)
    if hp == nil or hp % 1 ~= 0 then
      return trx.console.Result.BAD_INVOCATION
    end

    hp = math.max(0, math.min(hp, lara.max_hit_points))
    lara.hit_points = hp
    return trx.console.Result.OK, trx.locale.format("console/cmd/hp/set", hp)
  end,
})
