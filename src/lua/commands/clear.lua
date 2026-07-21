-- Clears the console.
--
-- Usages:
--   /cls

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
