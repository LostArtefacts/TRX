-- Plays a cutscene by number.
--
-- Usages:
--   /cut 1

local function run(args)
  local cutscenes = trx.game.cutscenes
  if #cutscenes == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_cutscene/none")
  end

  if args == "" then
    return trx.console.Result.BAD_INVOCATION
  end

  local num = tonumber(args)
  if num == nil or num % 1 ~= 0 then
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

for _, name in ipairs({ "cut", "cutscene" }) do
  trx.console.register({
    name = name,
    help = "console/cmd/play_cutscene/help",
    run = run,
  })
end
