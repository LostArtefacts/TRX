-- Clears the console.
--
-- Usages:
--   /cls

trx.locale.declare({
  ["console/cmd/clear/help"] = "Clears visible console logs.",
})

local function run()
  trx.console.clear()
  return trx.console.Result.OK
end

trx.console.register({
  name = "clear",
  aliases = { "cls" },
  help = "console/cmd/clear/help",
  run = run,
})
