-- Returns to the title screen.

trx.console.register({
  name = "title",
  help = "console/cmd/title/help",
  run = function(args)
    if args ~= "" then
      return trx.console.Result.BAD_INVOCATION
    end
    trx.game.exit_to_title()
  end,
})
