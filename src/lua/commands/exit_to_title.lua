-- Returns to the title screen.

trx.console.register({
  name = "title",
  help = "console/cmd/title/help",
  run = function()
    trx.game.exit_to_title()
  end,
})
