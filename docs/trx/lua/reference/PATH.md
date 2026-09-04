---
title: Paths
order: 34
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/path.lua. Edit it there.
-->

## <a id="path" name="path"></a>Paths module

Filesystem paths for Lua scripts. A path is a value rather than text, so joining one uses `/` and its parts are properties. Scripts can read and write under the game's own directories, and nowhere else.

### Properties

- <a id="path.trx_dir" name="path.trx_dir"></a>**`trx.path.trx_dir`** ([trx.path.Path](#path.Path)). The `%trx_dir%` directory, or `nil` where the game keeps none. *(read-only)*
- <a id="path.config_dir" name="path.config_dir"></a>**`trx.path.config_dir`** ([trx.path.Path](#path.Path)). The `%config_dir%` directory, or `nil` where the game keeps none. *(read-only)*
- <a id="path.cache_dir" name="path.cache_dir"></a>**`trx.path.cache_dir`** ([trx.path.Path](#path.Path)). The `%cache_dir%` directory, or `nil` where the game keeps none. *(read-only)*
- <a id="path.games_dir" name="path.games_dir"></a>**`trx.path.games_dir`** ([trx.path.Path](#path.Path)). The `%games_dir%` directory, or `nil` where the game keeps none. *(read-only)*
- <a id="path.screenshots_dir" name="path.screenshots_dir"></a>**`trx.path.screenshots_dir`** ([trx.path.Path](#path.Path)). The `%screenshots_dir%` directory, or `nil` where the game keeps none. *(read-only)*
- <a id="path.saves_dir" name="path.saves_dir"></a>**`trx.path.saves_dir`** ([trx.path.Path](#path.Path)). The `%saves_dir%` directory, or `nil` where the game keeps none. *(read-only)*
- <a id="path.legacy_saves_dir" name="path.legacy_saves_dir"></a>**`trx.path.legacy_saves_dir`** ([trx.path.Path](#path.Path)). The `%legacy_saves_dir%` directory, or `nil` where the game keeps none. *(read-only)*

### Structures

- <a id="path.Path" name="path.Path"></a>[lua]`trx.path.Path`

    A filesystem path. Joining one with `/` appends a child segment, and its
    parts are available as properties.

    A path only points to a location. It does not say whether a file is
    present until [`exists`](#path.Path.exists) checks it.

    Properties:
    - <a id="path.Path.name" name="path.Path.name"></a>**`name`**: string. The final component of the path, with its extension. *(read-only)*
    - <a id="path.Path.parent" name="path.Path.parent"></a>**`parent`**: [trx.path.Path](#path.Path). The directory the path sits in. *(read-only)*
    - <a id="path.Path.stem" name="path.Path.stem"></a>**`stem`**: string. The final component of the path, without its extension. *(read-only)*
    - <a id="path.Path.suffix" name="path.Path.suffix"></a>**`suffix`**: string. The extension at the end of the final component, leading `.` and all, or the empty string where there is none. *(read-only)*

    Operators:
    - **`path .. path`**. A path joins text as itself, whichever side of the `..` it is on.
    - **`path / path`**. Appends a child segment, as `config_dir / "mymod" / "state.json"`. An absolute path on the right replaces the left side.
    - **`path == path`**. Two paths are equal when their filesystem text is equal.
    - **`tostring(path)`**. The path as the text the engine would open.

    Methods:

    - <a id="path.Path.exists" name="path.Path.exists"></a>[lua]`path:exists()`  
      Whether anything is at the path now. Raises where the path is outside the directories a script may reach.

      Returns: boolean. Whether a file or directory is present.

    - <a id="path.Path.is_reachable" name="path.Path.is_reachable"></a>[lua]`path:is_reachable()`  
      Whether a script may read or write there. Scripts reach the game's own directories and nothing else, so the rest of the player's disk is closed to them.

      Returns: boolean. Whether reading and writing are allowed.

    - <a id="path.Path.read_text" name="path.Path.read_text"></a>[lua]`path:read_text()`  
      Reads the file as text, or returns `nil` where no file is present. Raises where the path is outside the directories a script may reach.

      Returns: string or `nil`. The text, or `nil` for a file that is not there.

    - <a id="path.Path.write_text" name="path.Path.write_text"></a>[lua]`path:write_text(text)`  
      Writes text into the file, making the directories it sits in and writing over an existing file. Raises where the path is outside the directories a script may reach.

      Parameters:
      - <a id="path.Path.write_text.text" name="path.Path.write_text.text"></a>**`text`** (string). What to write.

### Functions

- <a id="path.new" name="path.new"></a>[lua]`trx.path.new(text)`  
  Creates a path from text, which the engine opens as it stands. Every `%token%` in the text is expanded first, so `"%config_dir%/mymod"` says the same thing as `trx.path.config_dir / "mymod"`.

  Parameters:
  - <a id="path.new.text" name="path.new.text"></a>**`text`** (string). The path as text.

  Returns: [trx.path.Path](#path.Path). The path.

  Example:
  ```lua
  local kept = trx.path.new("%config_dir%/mymod/state.json")
  ```

- <a id="path.kinds" name="path.kinds"></a>[lua]`trx.path.kinds()`  
  Every kind of file [`trx.path.resolve`](#path.resolve) may be asked for.

  Returns: table. The file kinds, as a list of strings.

  Example:
  ```lua
  for _, kind in ipairs(trx.path.kinds()) do
    trx.log.info(kind)
  end
  ```

- <a id="path.resolve" name="path.resolve"></a>[lua]`trx.path.resolve(kind, name)`  
  Works out where the engine would find one of its own files, searching in
  the order it searches: a mod's own copy first, then the game the mod sits
  on, then the configuration directory. If no file is found, this returns
  `nil`.

  This is how a script reads a file the game ships without knowing which of
  those directories supplies it. [`trx.path.kinds`](#path.kinds) lists what may be asked for.

  Parameters:
  - <a id="path.resolve.kind" name="path.resolve.kind"></a>**`kind`** (string). Which kind of file, such as `common_config` or `level_file`.
  - <a id="path.resolve.name" name="path.resolve.name"></a>**`name`** (string). The file to look for, such as `weapons.json5`.

  Returns: [trx.path.Path](#path.Path) or `nil`. The file path, or `nil`.

  Example:
  ```lua
  local weapons = trx.path.resolve("common_config", "weapons.json5")
  if weapons ~= nil then
    trx.log.info("weapons come from " .. tostring(weapons))
  end
  ```
