---
title: JSON
order: 35
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/json.lua. Edit it there.
-->

## <a id="json" name="json"></a>JSON module

Reading and writing JSON, both as text and as a file on disk. The API dump the reference is generated from goes through this, so what a script writes out is encoded the way the engine's own data is.

### Functions

- <a id="json.encode" name="json.encode"></a>[lua]`trx.json.encode(value)`  
  Writes a value out as JSON, on one line. Keys come out in sorted order, so
  the same value encodes the same way twice and a file that is committed and
  diffed only moves when its JSON changes.

  A table is written as a list when entry 1 is present, or when the table is
  empty, and as an object otherwise. A number an object is keyed by comes out
  as text, and a key of any other kind is left out. A function, a handle and anything else
  with no JSON form is left out of an object, and stands as `null` in a list,
  which keeps the entries after it where they were. A number that is not
  finite, and a table that contains itself, raise.

  Parameters:
  - <a id="json.encode.value" name="json.encode.value"></a>**`value`** (any). What to write out.

  Returns: string. The JSON text.

  Example:
  ```lua
  trx.json.encode({ name = "wolf", ids = { 7, 8 } })
  -- {"ids":[7,8],"name":"wolf"}
  ```

- <a id="json.decode" name="json.decode"></a>[lua]`trx.json.decode(text)`  
  Reads a value out of JSON text, as the game reads its own data files: a
  comment, a trailing comma and an unquoted key are all taken. Text that
  does not parse raises with the line and the column.

  Text nested deeper than 100 levels raises as well.

  An object comes back as a table keyed by name and an array as a table
  keyed from 1, which is what [`trx.json.encode`](#json.encode) writes back out. `null`
  comes back as `nil`, so a key with `null` reads the same as an absent key,
  and an array entry with `null` ends the list there.

  Parameters:
  - <a id="json.decode.text" name="json.decode.text"></a>**`text`** (string). The JSON to read.

  Returns: any or `nil`. The decoded value.

  Example:
  ```lua
  local held = trx.json.decode('{"hp": 6, "seen": ["vilcabamba"]}')
  print(held.hp, held.seen[1])
  ```

- <a id="json.read_file" name="json.read_file"></a>[lua]`trx.json.read_file(path)`  
  Reads a file as JSON, which is
  [`trx.path.Path.read_text`](PATH.md#path.Path.read_text) and [`trx.json.decode`](#json.decode) in one call. A file that
  is not there answers `nil`, and one that does not parse raises with the
  file, the line and the column.

  Every table read from a file carries where it was written, which
  [`trx.json.where`](#json.where) reads back. A path outside the directories a script may reach
  raises rather than being read.

  Parameters:
  - <a id="json.read_file.path" name="json.read_file.path"></a>**`path`** ([trx.path.Path](PATH.md#path.Path) or string). Which file, as a path or as the text of one. [`trx.path.resolve`](PATH.md#path.resolve) finds one the game ships, and [`trx.path.config_dir`](PATH.md#path.config_dir) is for a script's own files.

  Returns: any or `nil`. The decoded value, or `nil` where there is no such file.

  Example:
  ```lua
  local found = trx.path.resolve("common_config", "weapons.json5")
  local weapons = found ~= nil and trx.json.read_file(found) or {}
  for key, spec in pairs(weapons) do
    print(key, spec.damage)
  end
  ```

- <a id="json.write_file" name="json.write_file"></a>[lua]`trx.json.write_file(path, value)`  
  Writes a value as JSON and saves it to the file, which is
  [`trx.json.encode`](#json.encode) and [`trx.path.Path.write_text`](PATH.md#path.Path.write_text) in one call.

  A value [`trx.json.encode`](#json.encode) has no JSON for raises, and nothing is written.
  A path outside the directories a script may reach raises as well, so
  [`trx.path.config_dir`](PATH.md#path.config_dir) is where a script's own file belongs.

  Parameters:
  - <a id="json.write_file.path" name="json.write_file.path"></a>**`path`** ([trx.path.Path](PATH.md#path.Path) or string). Which file, as a path or as the text of one. [`trx.path.resolve`](PATH.md#path.resolve) finds one the game ships, and [`trx.path.config_dir`](PATH.md#path.config_dir) is for a script's own files.
  - <a id="json.write_file.value" name="json.write_file.value"></a>**`value`** (any). What to write out.

  Example:
  ```lua
  local kept = trx.path.config_dir / "mymod" / "state.json"
  trx.json.write_file(kept, { seen = { "vilcabamba" } })
  ```

- <a id="json.where" name="json.where"></a>[lua]`trx.json.where(value)`  
  Says where a table read from a file was written: the file, the line and the
  column. Use it when a script must report the source line for bad file
  data. A table a script built itself, and anything that is not a table,
  answer `nil`.

  Parameters:
  - <a id="json.where.value" name="json.where.value"></a>**`value`** (any). A table [`trx.json.read_file`](#json.read_file) gave back.

  Returns: string or `nil`. Where the table was written, or `nil` without file location data.

  Example:
  ```lua
  local weapons = trx.json.read_file("weapons.json5")
  for key, spec in pairs(weapons) do
    if spec.damage == nil then
      error(("%s: '%s' says no damage"):format(trx.json.where(spec), key))
    end
  end
  ```
