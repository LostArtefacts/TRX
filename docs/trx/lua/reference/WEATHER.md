---
title: Weather
order: 21
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

- **`trx.weather.current`** (integer). The active weather. Compare against `trx.weather.Type`. *(read-only)*

### Enums

- [lua]`trx.weather.Type`

    The kinds of weather a level can show.

    - `trx.weather.Type.NONE` = `0`  
        Clear.
    - `trx.weather.Type.RAIN` = `1`  
        Rain.
    - `trx.weather.Type.SNOW` = `2`  
        Snow.

### Functions

- [lua]`trx.weather.set(type)`  
  Sets the active weather.

  Parameters:
  - **`type`** (integer). The weather to show. Compare against `trx.weather.Type`.

  Example:
  ```lua
  trx.weather.set(trx.weather.Type.SNOW)
  ```
