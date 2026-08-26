-- Plays an FMV.
--
-- Usages:
--   /fmv 2       a movie by number

trx.locale.declare({
  ["console/cmd/play_fmv/disabled"] = "FMVs are turned off",
  ["console/cmd/play_fmv/help"] = "Plays the FMV with the given number.",
  ["console/cmd/play_fmv/invalid"] = "Invalid FMV",
  ["console/cmd/play_fmv/loading"] = "Playing FMV %d",
  ["console/cmd/play_fmv/none"] = "This game has no FMVs",
})

local function run(args)
  if not trx.config.get("gameplay.enable_fmv") then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_fmv/disabled")
  end

  local fmvs = trx.game.fmvs
  if #fmvs == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_fmv/none")
  end

  local num = args.num
  if num == nil then
    return trx.console.Result.BAD_INVOCATION
  end

  if num < 1 or num > #fmvs then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_fmv/invalid")
  end

  trx.game.play_fmv(num)
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/play_fmv/loading", fmvs[num].num)
end

trx.console.register({
  name = "fmv",
  help = "console/cmd/play_fmv/help",
  args = function(parser)
    parser:positional("num", { type = "integer", optional = true })
  end,
  run = run,
})
