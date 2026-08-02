---
title: Weather
order: 15
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/weather.lua. Edit it there.
-->

## Weather module

The runtime weather effect the current level shows.

### Properties

- <a name="weather.current"></a>**`trx.weather.current`** ([trx.weather.Type](#weather.Type)). The active weather. *(read-only)*

### Enums

- <a name="weather.Type"></a>[lua]`trx.weather.Type`

    The kinds of weather a level can show.

    - `trx.weather.Type.NONE` = `0`  
        Clear.
    - `trx.weather.Type.RAIN` = `1`  
        Rain.
    - `trx.weather.Type.SNOW` = `2`  
        Snow.

### Functions

- <a name="weather.set"></a>[lua]`trx.weather.set(type)`  
  Sets the active weather.

  Parameters:
  - **`type`** ([trx.weather.Type](#weather.Type)). The weather to show.

  Example:
  ```lua
  trx.weather.set(trx.weather.Type.SNOW)
  ```
