-- Saves a screenshot.
--
-- Usages:
--   /screenshot          save one to the screenshots folder
--   /screenshot foo.png  save to a path

trx.console.register({
  name = "screenshot",
  help = "console/cmd/screenshot/help",
  run = function(args)
    if args == "" then
      trx.game.screenshot()
    else
      trx.game.screenshot(args)
    end
    return trx.console.Result.OK
  end,
})
