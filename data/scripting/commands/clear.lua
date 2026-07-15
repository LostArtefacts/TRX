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

for _, name in ipairs({ "cls", "clear" }) do
  trx.console.register({
    name = name,
    help = "console/cmd/clear/help",
    run = run,
  })
end
