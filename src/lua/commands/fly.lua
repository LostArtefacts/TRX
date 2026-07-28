-- Toggles the fly-mode cheat.
--
-- Usages:
--   /fly        toggle
--   /fly on     force it on
--   /fly off    force it off

trx.locale.declare({
  ["console/cmd/fly/already_off"] = "Lara is not flying",
  ["console/cmd/fly/already_on"] = "Lara is already flying",
  ["console/cmd/fly/help"] = "Toggles the fly-mode cheat.",
})

trx.console.register({
  name = "fly",
  help = "console/cmd/fly/help",
  args = function(parser)
    parser:positional("state", { type = "boolean", optional = true })
  end,
  run = function(args)
    -- A flyby is cancellable even from a menu, so it lifts the playable guard.
    local flyby = trx.camera.is_flyby_active
    if not flyby and not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    local target = args.state
    if target == nil then
      target = not trx.lara.is_flying
    end

    if flyby then
      trx.camera.cancel_flyby()
    end

    if trx.lara.is_flying == target then
      if target then
        trx.console.log.warning(trx.locale.get("console/cmd/fly/already_on"))
      else
        trx.console.log.warning(trx.locale.get("console/cmd/fly/already_off"))
      end
      return trx.console.Result.OK
    end

    trx.lara.is_flying = target
    return trx.console.Result.OK
  end,
})
