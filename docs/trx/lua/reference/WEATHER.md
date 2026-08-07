---
title: Weather
order: 16
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/weather.lua. Edit it there.
-->

## <a id="weather" name="weather"></a>Weather module

The runtime weather effect the current level shows.

### Properties

- <a id="weather.current" name="weather.current"></a>**`trx.weather.current`** ([trx.weather.Type](#weather.Type)). The active weather. *(read-only)*
- <a id="weather.severity" name="weather.severity"></a>**`trx.weather.severity`** (number). How heavy the weather falls, as a multiple of the number of particles the
  original games show. `1` is that number, `0` leaves the sky clear, and `4` is
  as much as the particle pool holds; a value outside the range is clamped to it.

  A level starts at `1`, and a savegame carries what it was saved with.

### Enums

- <a id="weather.Type" name="weather.Type"></a>[lua]`trx.weather.Type`

    The kinds of weather a level can show.

    - `trx.weather.Type.NONE` = `0`  
        Clear.
    - `trx.weather.Type.RAIN` = `1`  
        Rain.
    - `trx.weather.Type.SNOW` = `2`  
        Snow.

### Functions

- <a id="weather.set" name="weather.set"></a>[lua]`trx.weather.set(type)`  
  Sets the active weather.

  Parameters:
  - <a id="weather.set.type" name="weather.set.type"></a>**`type`** ([trx.weather.Type](#weather.Type)). The weather to show.

  Example:
  ```lua
  trx.weather.set(trx.weather.Type.SNOW)
  ```
