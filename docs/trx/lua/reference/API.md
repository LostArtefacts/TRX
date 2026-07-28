---
title: API registry
order: 26
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/api.lua. Edit it there.
-->

## API registry module

Argument checking for the whole of `trx`.

### Functions

- [lua]`trx.api.strict(enabled)`  
  Turns argument checking on or off for every function in `trx`, and for the methods on its handles. Off by default: checking costs about 100ns a call, which a per-frame handler notices. Turn it on while writing a level, and leave it off in play.

  Parameters:
  - **`enabled`** (boolean).

  Example:
  ```lua
  trx.api.strict(true)
  ```

- [lua]`trx.api.is_strict()`  
  Whether argument checking is on.

  Returns: boolean.
