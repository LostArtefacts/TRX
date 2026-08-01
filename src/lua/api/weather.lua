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
