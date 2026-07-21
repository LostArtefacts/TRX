-- Exits the game.

trx.console.register({
  name = "exit",
  aliases = { "quit" },
  help = "console/cmd/exit/help",
  run = function(args)
    if args ~= "" then
      return trx.console.Result.BAD_INVOCATION
    end
    trx.game.exit_game()
  end,
})
