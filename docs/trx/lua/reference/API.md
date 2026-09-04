---
title: API registry
order: 37
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/api.lua. Edit it there.
-->

## <a id="api" name="api"></a>API registry module

Argument checking for the whole of `trx`.

### Functions

- <a id="api.strict" name="api.strict"></a>[lua]`trx.api.strict(enabled)`  
  Turns argument checking on or off for every function in `trx`, and for the
  methods on its handles. Off by default: checking costs a couple of hundred
  nanoseconds a call, which a per-frame handler notices. Turn it on while
  writing a level, and leave it off in play.

  Parameters:
  - <a id="api.strict.enabled" name="api.strict.enabled"></a>**`enabled`** (boolean). Whether to check.

  Example:
  ```lua
  trx.api.strict(true)
  ```

- <a id="api.is_strict" name="api.is_strict"></a>[lua]`trx.api.is_strict()`  
  Whether argument checking is on.

  Returns: boolean. False as the game starts, and true once something turns it on.
