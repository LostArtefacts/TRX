-- Casts Lara in gold, or takes the gold back off.
--
-- Usages:
--   /golden       toggle
--   /golden on
--   /golden off

local OPTION = "visuals.golden_lara"

trx.locale.declare({
  ["console/cmd/golden/help"] = "Casts Lara in gold, whichever outfit she is wearing.",
  ["console/cmd/golden/off"] = "Lara is flesh and blood again",
  ["console/cmd/golden/on"] = "Lara is cast in gold",
})

trx.console.register({
  name = "golden",
  help = "console/cmd/golden/help",
  args = function(parser)
    parser:positional("state", { type = "boolean", optional = true })
  end,
  run = function(args)
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    local target = args.state
    if target == nil then
      target = not trx.config.get(OPTION)
    end
    trx.config.set(OPTION, target)

    local key = target and "console/cmd/golden/on" or "console/cmd/golden/off"
    return trx.console.Result.OK, trx.locale.get(key)
  end,
})
