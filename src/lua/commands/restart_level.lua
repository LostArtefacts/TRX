-- Restarts the current level from its start.
--
-- Usages:
--   /restartlevel
--   /restart

trx.locale.declare({
  ["console/cmd/restart_level/help"] = "Restarts the current level.",
})

local function run()
  if not trx.game.can_restart_level() then
    return trx.console.Result.UNAVAILABLE
  end
  trx.game.restart_level()
  return trx.console.Result.OK
end

trx.console.register({
  name = "restartlevel",
  aliases = { "restart", "restart-level" },
  help = "console/cmd/restart_level/help",
  run = run,
})
