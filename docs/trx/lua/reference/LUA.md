---
title: Lua
order: 34
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/lua.lua. Edit it there.
-->

## <a id="lua" name="lua"></a>Lua module

Evaluating Lua at runtime: a string of code, or a file on disk. Both run in the same state as every other script.

### Structures

- <a id="lua.Error" name="lua.Error"></a>[lua]`trx.lua.Error`

    What went wrong while running Lua.

    Properties:
    - <a id="lua.Error.kind" name="lua.Error.kind"></a>**`kind`**: string. Either `"syntax"` or `"runtime"`.
    - <a id="lua.Error.message" name="lua.Error.message"></a>**`message`**: string. The error text.

### Functions

- <a id="lua.eval_expr" name="lua.eval_expr"></a>[lua]`trx.lua.eval_expr(code)`  
  Evaluates a string of Lua code, as the `/lua` console command does.

  Parameters:
  - <a id="lua.eval_expr.code" name="lua.eval_expr.code"></a>**`code`** (string). Any chunk of Lua, not only an expression.

  Returns: [trx.lua.Error](#lua.Error) or `nil`. `nil` when the code ran to completion, and what went wrong otherwise. A failure comes back as a value rather than raising, so the caller decides what it means.

  Example:
  ```lua
  trx.lua.eval_expr("trx.console.log('hello')")
  ```

- <a id="lua.eval_file" name="lua.eval_file"></a>[lua]`trx.lua.eval_file(path)`  
  Runs a Lua file, the way a level script is run. A file that cannot be read reports as a `"runtime"` failure.

  Parameters:
  - <a id="lua.eval_file.path" name="lua.eval_file.path"></a>**`path`** (string). Path of the file.

  Returns: [trx.lua.Error](#lua.Error) or `nil`.

  Example:
  ```lua
  trx.lua.eval_file("data/ship/scripts/extra.lua")
  ```
