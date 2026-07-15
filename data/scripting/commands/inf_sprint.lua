-- Toggles infinite sprint.
--
-- Usages:
--   /restless        toggle it
--   /restless on     force it on
--   /restless off    force it off

local KEY = "debug.enable_endless_sprint"

trx.console.register({
  name = "restless",
  help = "console/cmd/inf_sprint/help",
  run = function(args)
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
      return trx.console.Result.OK, trx.locale.get("console/cmd/inf_sprint/on")
    end
    return trx.console.Result.OK, trx.locale.get("console/cmd/inf_sprint/off")
  end,
})
