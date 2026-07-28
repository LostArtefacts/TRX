-- Lists the commands, or shows one command's help.
--
-- Usages:
--   /help          list every command that carries help
--   /help fly      the help for one, matched by name or alias

trx.locale.declare({
  ["console/cmd/help/help"] = "Shows help for all commands or detailed help for one.",
  ["console/cmd/help/list"] = "Available commands:",
  ["console/cmd/help/unknown_command"] = "Unknown command: %s",
})

-- The commands that carry help, best listed and completed by name.
local function documented()
  local out = {}
  for _, cmd in ipairs(trx.console.commands()) do
    if cmd.help ~= nil then
      out[#out + 1] = cmd.name
    end
  end
  return out
end

local function run(args)
  if args.command == nil then
    trx.console.log(trx.locale.get("console/cmd/help/list"))
    trx.console.log(table.concat(documented(), ", "))
    return trx.console.Result.OK
  end

  local cmd = trx.console.command(args.command)
  if cmd == nil or cmd.help == nil then
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/help/unknown_command", args.command)
  end
  trx.console.log(cmd.help)
  return trx.console.Result.OK
end

trx.console.register({
  name = "help",
  help = "console/cmd/help/help",
  args = function(parser)
    parser:positional("command", { optional = true, suggest = documented })
  end,
  run = run,
})
