-- Reloads the current language's text from disk.

trx.console.register({
  name = "strings",
  help = "console/cmd/strings/help",
  run = function()
    if trx.locale.reload() then
      return trx.console.Result.OK,
        trx.locale.get("console/cmd/strings/reloaded")
    end
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/strings/failed")
  end,
})
