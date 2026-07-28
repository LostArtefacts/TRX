-- Sets the weather in the current level.
--
-- Usages:
--   /weather snow
--   /weather rain
--   /weather none

trx.locale.declare({
  ["console/cmd/weather/current"] = "Current weather: %s",
  ["console/cmd/weather/help"] = "Changes the current weather type.",
  ["console/cmd/weather/invalid"] = "Invalid weather: %s (valid: %s)",
  ["console/cmd/weather/set"] = "Weather set to %s",
})

local function type_names()
  local names = {}
  for name in pairs(trx.weather.Type) do
    names[#names + 1] = name:lower()
  end
  table.sort(names)
  return names
end

local function name_of(value)
  for name, v in pairs(trx.weather.Type) do
    if v == value then
      return name:lower()
    end
  end
  return tostring(value)
end

trx.console.register({
  name = "weather",
  help = "console/cmd/weather/help",
  args = function(parser)
    parser:positional("state", { optional = true, choices = type_names })
  end,
  run = function(args)
    if not trx.game.is_loaded then
      return trx.console.Result.UNAVAILABLE
    end

    local level = trx.game.current_level
    if level == nil or level.type == trx.game.LevelType.TITLE then
      return trx.console.Result.UNAVAILABLE
    end

    if args.state == nil then
      return trx.console.Result.OK,
        trx.locale.format(
          "console/cmd/weather/current",
          name_of(trx.weather.current)
        )
    end

    local want = args.state:lower()
    for name, value in pairs(trx.weather.Type) do
      if name:lower() == want then
        trx.weather.set(value)
        return trx.console.Result.OK,
          trx.locale.format("console/cmd/weather/set", args.state)
      end
    end

    return trx.console.Result.FAILURE,
      trx.locale.format(
        "console/cmd/weather/invalid",
        args.state,
        table.concat(type_names(), ", ")
      )
  end,
})
