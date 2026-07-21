-- Toggles Lara's invulnerability.
--
-- Usages:
--   /immune        toggle it
--   /immune on     force it on
--   /immune off    force it off

local KEY = "debug.enable_invulnerability"

local function run(args)
  local enable = args.state
  if enable == nil then
    enable = not trx.config.get(KEY)
  end

  trx.config.set(KEY, enable)
  if enable then
    return trx.console.Result.OK, trx.locale.get("console/cmd/immune/on")
  end
  return trx.console.Result.OK, trx.locale.get("console/cmd/immune/off")
end

trx.console.register({
  name = "immune",
  aliases = { "immunity" },
  help = "console/cmd/immune/help",
  args = function(parser)
    parser:positional("state", { type = "boolean", optional = true })
  end,
  run = run,
})
