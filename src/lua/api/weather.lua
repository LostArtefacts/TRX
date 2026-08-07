local raw = trxc.weather
local api = trx.api

api.module("weather", {
  order = 16,
  description = "The runtime weather effect the current level shows.",
})

api.enum("weather.Type", {
  backing = "WEATHER_TYPE",
  description = "The kinds of weather a level can show.",
  values = {
    NONE = "Clear.",
    RAIN = "Rain.",
    SNOW = "Snow.",
  },
})

api.define("weather.set", {
  description = "Sets the active weather.",
  params = {
    {
      name = "type",
      type = "weather.Type",
      description = "The weather to show.",
    },
  },
  examples = { [[trx.weather.set(trx.weather.Type.SNOW)]] },
  impl = raw.set,
})

api.property("weather.current", {
  type = "weather.Type",
  description = "The active weather.",
  get = raw.get,
})

api.property("weather.severity", {
  type = "number",
  description = [[
How heavy the weather falls, as a multiple of the number of particles the
original games show. `1` is that number, `0` leaves the sky clear, and `4` is
as much as the particle pool holds; a value outside the range is clamped to it.

A level starts at `1`, and a savegame carries what it was saved with.
]],
  get = raw.get_severity,
  set = raw.set_severity,
})
