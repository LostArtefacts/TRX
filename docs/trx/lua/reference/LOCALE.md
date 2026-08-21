---
title: Locale
order: 22
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/locale.lua. Edit it there.
-->

## <a id="locale" name="locale"></a>Locale module

The text the player reads, in the player's own language.

### Functions

- <a id="locale.declare" name="locale.declare"></a>[lua]`trx.locale.declare(strings)`  
  Declares game string keys and the text behind them.

  A key belongs with the script that shows it, so a command carries its own
  wording. What is declared here is a fallback, and it takes only for a key
  nothing else holds: the strings files, their translations, and any earlier
  declaration keep the text they already carry.

  Parameters:
  - <a id="locale.declare.strings" name="locale.declare.strings"></a>**`strings`** (table). Keys to their English text.

  Example:
  ```lua
  trx.locale.declare({
    ["console/cmd/heal/help"] = "Heals Lara back to full health.",
    ["console/cmd/heal/success"] = "Healed Lara back to full health",
  })
  ```

- <a id="locale.get" name="locale.get"></a>[lua]`trx.locale.get(key)`  
  The text behind a game string key.

  Parameters:
  - <a id="locale.get.key" name="locale.get.key"></a>**`key`** (string). The key, e.g. `general/misc/off`.

  Returns: string. The key itself if nothing is behind it, so a typo shows up on screen rather than as a nil further down.

  Example:
  ```lua
  trx.console.log(trx.locale.get("general/misc/off"))
  ```

- <a id="locale.format" name="locale.format"></a>[lua]`trx.locale.format(key, ...)`  
  The text behind a key with its placeholders filled in.

  Parameters:
  - <a id="locale.format.key" name="locale.format.key"></a>**`key`** (string). The key, e.g. `general/misc/off`.
  - <a id="locale.format...." name="locale.format...."></a>**`...`** (any). What to fill the placeholders with.

  Returns: string. The text with the arguments in it. A translation whose placeholders do not line up with the arguments comes back unformatted, with a warning in the log: a player is better served by text they can read than by a script that stops.

  Example:
  ```lua
  trx.console.log(trx.locale.format("general/misc/pagination_nav", 1, 5))
  ```

- <a id="locale.reload" name="locale.reload"></a>[lua]`trx.locale.reload()`  
  Reloads the current language's text from disk.

  Returns: boolean. Whether the reload succeeded.
