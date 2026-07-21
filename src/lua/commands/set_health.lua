-- Reads or sets Lara's health.
--
-- Usages:
--   /hp        report her current health
--   /hp 500    set it

trx.console.register({
  name = "hp",
  help = "console/cmd/hp/help",
  args = function(parser)
    parser:positional("hp", { type = "integer", optional = true })
  end,
  run = function(args)
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    local lara = trx.lara.item
    if args.hp == nil then
      return trx.console.Result.OK,
        trx.locale.format("console/cmd/hp/get", lara.hit_points)
    end

    local hp = math.max(0, math.min(args.hp, lara.max_hit_points))
    lara.hit_points = hp
    return trx.console.Result.OK, trx.locale.format("console/cmd/hp/set", hp)
  end,
})
