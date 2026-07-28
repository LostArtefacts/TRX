-- Starts the gym level.
--
-- Usages:
--   /gym

trx.locale.declare({
  ["console/cmd/play_gym/help"] = "Plays the Gym level.",
  ["console/cmd/play_gym/invalid"] = "Invalid level",
  ["console/cmd/play_gym/loading"] = "Loading %s",
})

local function run()
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
