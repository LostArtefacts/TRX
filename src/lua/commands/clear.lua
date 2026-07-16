-- Clears the console.
--
-- Usages:
--   /cls

local function run(args)
  if args ~= "" then
    return trx.console.Result.BAD_INVOCATION
  end
  trx.console.clear()
  return trx.console.Result.OK
end

trx.console.register({
  name = "clear",
  aliases = { "cls" },
  help = "console/cmd/clear/help",
  run = run,
})
