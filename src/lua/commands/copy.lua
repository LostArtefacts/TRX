-- Runs another command and copies its output to the clipboard.
--
-- Usages:
--   /copy pos          Lara's position
--   /copy version      the build the game reports

trx.locale.declare({
  ["console/cmd/copy/help"] = "Copies a command's output to the clipboard.",
  ["console/cmd/copy/copied"] = "Copied to the clipboard.",
  ["console/cmd/copy/failed"] = "%s did not run.",
  ["console/cmd/copy/no_output"] = "%s printed nothing to copy.",
})

trx.console.register({
  name = "copy",
  help = "console/cmd/copy/help",
  args = function(parser)
    parser:rest("command", { help = "the command to run" })
  end,
  -- Everything past the command word is a line of its own, so the console
  -- completes the tail the way it completes what the player types.
  complete = trx.console.complete,
  run = function(args)
    local ok, output =
      pcall(trx.console.eval, args.command, { verbose = true, capture = true })
    if not ok then
      return trx.console.Result.FAILURE,
        trx.locale.format("console/cmd/copy/failed", args.command)
    end
    if output == "" then
      return trx.console.Result.FAILURE,
        trx.locale.format("console/cmd/copy/no_output", args.command)
    end
    trx.console.copy(output)
    return trx.console.Result.OK, trx.locale.get("console/cmd/copy/copied")
  end,
})
