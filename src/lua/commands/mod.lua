-- Reports the loaded mod, or switches to another and restarts the game.

trx.console.register({
  name = "mod",
  help = "console/cmd/mod/help",
  args = function(parser)
    parser:rest("name", { optional = true })
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
