-- Reports the loaded mod, or switches to another and restarts the game.

trx.locale.declare({
  ["console/cmd/mod/current"] = "Currently loaded mod: %s",
  ["console/cmd/mod/help"] = "Switches to the specified mod and restarts the game.",
  ["console/cmd/mod/invalid"] = "Invalid mod: %s",
})

local function mod_names()
  local out = {}
  for _, mod in ipairs(trx.mod.list) do
    if mod.can_switch then
      out[#out + 1] = mod.name
    end
  end
  return out
end

trx.console.register({
  name = "mod",
  help = "console/cmd/mod/help",
  args = function(parser)
    parser:rest("name", { optional = true, suggest = mod_names })
  end,
  run = function(args)
    if args.name == nil then
      trx.console.log(
        trx.locale.format("console/cmd/mod/current", trx.mod.current.name)
      )
      return
    end

    if not trx.mod.switch(args.name) then
      return trx.console.Result.FAILURE,
        trx.locale.format("console/cmd/mod/invalid", args.name)
    end
  end,
})
