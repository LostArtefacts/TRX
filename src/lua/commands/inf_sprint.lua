-- Toggles infinite sprint.
--
-- Usages:
--   /restless        toggle it
--   /restless on     force it on
--   /restless off    force it off

trx.locale.declare({
  ["console/cmd/inf_sprint/help"] = "Toggles infinite sprint.",
  ["console/cmd/inf_sprint/off"] = "Lara can no longer sprint forever",
  ["console/cmd/inf_sprint/on"] = "Lara can now sprint forever",
})

local KEY = "debug.enable_endless_sprint"

trx.console.register({
  name = "restless",
  help = "console/cmd/inf_sprint/help",
  args = function(parser)
    parser:positional("state", { type = "boolean", optional = true })
  end,
  run = function(args)
    local enable = args.state
    if enable == nil then
      enable = not trx.config.get(KEY)
    end

    trx.config.set(KEY, enable)
    if enable then
      return trx.console.Result.OK, trx.locale.get("console/cmd/inf_sprint/on")
    end
    return trx.console.Result.OK, trx.locale.get("console/cmd/inf_sprint/off")
  end,
})
