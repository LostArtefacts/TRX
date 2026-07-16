-- Ends the current level, as reaching its exit would.
--
-- Usages:
--   /endlevel
--   /nextlevel

local function run(args)
  if args ~= "" then
    return trx.console.Result.BAD_INVOCATION
  end

  local level = trx.game.current_level
  if level == nil or level.type == trx.game.LevelType.TITLE then
    return trx.console.Result.UNAVAILABLE
  end

  trx.game.end_level()
  return trx.console.Result.OK
end

trx.console.register({
  name = "endlevel",
  aliases = { "nextlevel", "end-level", "next-level" },
  help = "console/cmd/end_level/help",
  run = run,
})
