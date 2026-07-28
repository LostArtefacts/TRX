-- Plays a cutscene by number.
--
-- Usages:
--   /cut 1

trx.locale.declare({
  ["console/cmd/play_cutscene/help"] = "Plays a cutscene with the given number.",
  ["console/cmd/play_cutscene/invalid"] = "Invalid cutscene",
  ["console/cmd/play_cutscene/loading"] = "Loading cutscene %d",
  ["console/cmd/play_cutscene/none"] = "This game has no cutscenes",
})

local function run(args)
  local cutscenes = trx.game.cutscenes
  if #cutscenes == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_cutscene/none")
  end

  local num = args.num
  if num == nil then
    return trx.console.Result.BAD_INVOCATION
  end

  if num < 1 or num > #cutscenes then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_cutscene/invalid")
  end

  trx.game.play_cutscene(num)
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/play_cutscene/loading", cutscenes[num].num)
end

trx.console.register({
  name = "cut",
  aliases = { "cutscene" },
  help = "console/cmd/play_cutscene/help",
  args = function(parser)
    parser:positional("num", { type = "integer", optional = true })
  end,
  run = run,
})
