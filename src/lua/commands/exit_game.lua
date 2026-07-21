-- Exits the game.

trx.console.register({
  name = "exit",
  aliases = { "quit" },
  help = "console/cmd/exit/help",
  run = function()
    trx.game.exit_game()
  end,
})
