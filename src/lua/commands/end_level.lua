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
  help = "console/cmd/end_level/help",
  run = run,
})
trx.console.register({
  name = "nextlevel",
  help = "console/cmd/end_level/help",
  run = run,
})

-- The dashed spellings are aliases; they stay out of the help listing.
for _, name in ipairs({ "end-level", "next-level" }) do
  trx.console.register({ name = name, run = run })
end
