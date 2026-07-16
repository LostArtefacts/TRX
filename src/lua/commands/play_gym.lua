-- Starts the gym level.
--
-- Usages:
--   /gym

local function run(args)
  if args ~= "" then
    return trx.console.Result.BAD_INVOCATION
  end

  local gym = trx.game.gym
  if gym == nil then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_gym/invalid")
  end

  trx.game.play_gym()
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/play_gym/loading", gym.title)
end

trx.console.register({
  name = "gym",
  aliases = { "home" },
  help = "console/cmd/play_gym/help",
  run = run,
})
