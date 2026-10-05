local raw = trxc.weather
local h = require("trx.internal.helpers")

---@class trx
---@field weather trx.weather

---The runtime weather effect the current level shows.
---@trx.module 17
---@class (exact) trx.weather
---@trx.readonly current
---@field current trx.weather.Type The active weather.
---@field severity number How heavy the weather falls, as a multiple of the number of particles the original games show. `1` is that number, `0` leaves the sky clear, and `4` is as much as the particle pool holds; a value outside the range is clamped to it.
---
---  A level starts at `1`, and a savegame carries what it was saved with.
local M = h.module("weather")

---The kinds of weather a level can show.
---@enum trx.weather.Type
local Type = {
  ---Clear.
  NONE = h.IntegerConstant,
  ---Rain.
  RAIN = h.IntegerConstant,
  ---Snow.
  SNOW = h.IntegerConstant,
}
M.Type = h.enum("weather.Type", "WEATHER_TYPE", Type)

---Sets the active weather.
---
---```lua
---trx.weather.set(trx.weather.Type.SNOW)
---```
---@param type trx.weather.Type The weather to show.
---@type fun(type: trx.weather.Type)
M.set = raw.set

h.properties(M, "weather", {
  current = {
    get = raw.get,
  },
  severity = {
    get = raw.get_severity,
    set = raw.set_severity,
  },
})
