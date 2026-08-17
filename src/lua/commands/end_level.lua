-- Ends the current level, as reaching its exit would.
--
-- Usages:
--   /endlevel
--   /nextlevel

trx.locale.declare({
  ["console/cmd/end_level/help"] = "Ends the current level.",
})

local function run()
  local level = trx.game.current_level
  if level == nil or level.type == trx.game.LevelType.TITLE then
    return trx.console.Result.UNAVAILABLE
  end

  trx.game.end_level()
  return trx.console.Result.OK, trx.locale.get("general/osd/complete_level")
end

trx.console.register({
  name = "endlevel",
  aliases = { "nextlevel", "end-level", "next-level" },
  help = "console/cmd/end_level/help",
  run = run,
})
