-- Sets the weather in the current level, and how heavy it falls.
--
-- Usages:
--   /weather snow
--   /weather rain
--   /weather none
--   /weather rain 2.5
--   /weather 2.5

trx.locale.declare({
  ["console/cmd/weather/current"] = "Current weather: %s, severity %s",
  ["console/cmd/weather/help"] = "Changes the current weather type.",
  ["console/cmd/weather/invalid"] = "Invalid weather: %s (valid: %s)",
  ["console/cmd/weather/set"] = "Weather set to %s, severity %s",
})

local function type_names()
  local names = {}
  for name in pairs(trx.weather.Type) do
    names[#names + 1] = trx.strings.dash_case(name)
  end
  table.sort(names)
  return names
end

local function name_of(value)
  for name, v in pairs(trx.weather.Type) do
    if v == value then
      return trx.strings.dash_case(name)
    end
  end
  return tostring(value)
end

-- The severity as a player would write it: no trailing zeros, so the default
-- reads as 1 rather than 1.00.
local function severity_text(value)
  return (string.format("%.2f", value):gsub("%.?0+$", ""))
end

trx.console.register({
  name = "weather",
  help = "console/cmd/weather/help",
  args = function(parser)
    parser:positional("state", { optional = true, choices = type_names })
    parser:positional("severity", { optional = true, type = "number" })
  end,
  run = function(args)
    if not trx.game.is_loaded then
      return trx.console.Result.UNAVAILABLE
    end

    local level = trx.game.current_level
    if level == nil or level.type == trx.game.LevelType.TITLE then
      return trx.console.Result.UNAVAILABLE
    end

    if args.state == nil and args.severity == nil then
      return trx.console.Result.OK,
        trx.locale.format(
          "console/cmd/weather/current",
          name_of(trx.weather.current),
          severity_text(trx.weather.severity)
        )
    end

    if args.severity ~= nil then
      -- The property clamps what it is given; read it back for the message.
      trx.weather.severity = args.severity
    end

    if args.state ~= nil then
      local want = trx.strings.dash_case(args.state)
      local found = nil
      for name, value in pairs(trx.weather.Type) do
        if trx.strings.dash_case(name) == want then
          found = value
        end
      end

      if found == nil then
        return trx.console.Result.FAILURE,
          trx.locale.format(
            "console/cmd/weather/invalid",
            args.state,
            table.concat(type_names(), ", ")
          )
      end
      trx.weather.set(found)
    end

    return trx.console.Result.OK,
      trx.locale.format(
        "console/cmd/weather/set",
        name_of(trx.weather.current),
        severity_text(trx.weather.severity)
      )
  end,
})
