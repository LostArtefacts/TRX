-- Saves a screenshot.
--
-- Usages:
--   /screenshot          save one to the screenshots folder
--   /screenshot foo.png  save to a path

trx.locale.declare({
  ["console/cmd/screenshot/help"] = "Saves a screenshot to disk at optional location.",
})

trx.console.register({
  name = "screenshot",
  help = "console/cmd/screenshot/help",
  args = function(parser)
    parser:rest("path", { optional = true })
  end,
  run = function(args)
    if args.path == nil then
      trx.game.screenshot()
    else
      trx.game.screenshot(args.path)
    end
    return trx.console.Result.OK
  end,
})
