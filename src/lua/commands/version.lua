-- Reports which version this build is.
--
-- Usages:
--   /version

trx.locale.declare({
  ["console/cmd/version/help"] = "Shows the version this build reports.",
})

local function run()
  trx.console.log(trx.game.trx_version)
  return trx.console.Result.OK
end

trx.console.register({
  name = "version",
  help = "console/cmd/version/help",
  run = run,
})
