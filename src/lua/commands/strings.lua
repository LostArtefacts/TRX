-- Reloads the current language's text from disk.

trx.locale.declare({
  ["console/cmd/strings/failed"] = "Failed to reload the language files",
  ["console/cmd/strings/help"] = "Reloads the current language files from disk.",
  ["console/cmd/strings/reloaded"] = "Language files reloaded",
})

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
