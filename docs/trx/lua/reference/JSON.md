---
title: JSON
order: 33
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/json.lua. Edit it there.
-->

## <a id="json" name="json"></a>JSON module

Writing Lua values out as JSON. The API dump the reference is generated from goes through this, so what a script writes out is encoded the way the engine's own data is.

### Functions

- <a id="json.encode" name="json.encode"></a>[lua]`trx.json.encode(value)`  
  Writes a value out as JSON, on one line. Keys come out in sorted order, so
  the same value encodes the same way twice and a file that is committed and
  diffed only moves when what it holds does.

  A table is written as a list where it holds entry 1, or holds nothing at
  all, and as an object otherwise. A function, a handle and anything else
  with no JSON of its own is left out.

  Parameters:
  - <a id="json.encode.value" name="json.encode.value"></a>**`value`** (any). What to write out.

  Returns: string. The JSON text.

  Example:
  ```lua
  trx.json.encode({ name = "wolf", ids = { 7, 8 } })
  -- {"ids":[7,8],"name":"wolf"}
  ```
