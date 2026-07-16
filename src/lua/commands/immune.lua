-- Toggles Lara's invulnerability.
--
-- Usages:
--   /immune        toggle it
--   /immune on     force it on
--   /immune off    force it off

local KEY = "debug.enable_invulnerability"

local function run(args)
  local enable
  if args == "" then
    enable = not trx.config.get(KEY)
  else
    enable = trx.strings.parse_bool(args)
    if enable == nil then
      return trx.console.Result.BAD_INVOCATION
    end
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
  run = run,
})
