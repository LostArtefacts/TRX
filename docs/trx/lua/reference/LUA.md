---
title: Lua
order: 29
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/lua.lua. Edit it there.
-->

## <a id="lua" name="lua"></a>Lua module

Evaluating Lua at runtime: a string of code, or a file on disk. Both run in the same state as every other script.

### Functions

- <a id="lua.eval_expr" name="lua.eval_expr"></a>[lua]`trx.lua.eval_expr(code)`  
  Evaluates a string of Lua code, as the `/lua` console command does.

  Parameters:
  - **`code`** (string). The code.

  Returns: table or `nil`. `nil` when the code ran to completion. Otherwise a table with `kind` (`"syntax"` or `"runtime"`) and `message`, the error text. A failure comes back as a value rather than raising, so the caller decides what it means.

  Example:
  ```lua
  trx.lua.eval_expr("trx.console.log('hello')")
  ```

- <a id="lua.eval_file" name="lua.eval_file"></a>[lua]`trx.lua.eval_file(path)`  
  Runs a Lua file, the way a level script is run. A file that cannot be read reports as a `"runtime"` failure.

  Parameters:
  - **`path`** (string). Path of the file.

  Returns: table or `nil`. As in [`trx.lua.eval_expr`](#lua.eval_expr).

  Example:
  ```lua
  trx.lua.eval_file("data/ship/scripts/extra.lua")
  ```
