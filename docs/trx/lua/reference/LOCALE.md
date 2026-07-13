---
title: Locale
order: 18
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  data/scripting/locale.lua. Edit it there.
-->

## Locale module

The text the player reads, in the player's own language.

### Functions

- [lua]`trx.locale.get(key)`  
  The text behind a game string key.

  Parameters:
  - **`key`** (string). The key, e.g. `general/misc/off`.

  Returns: string. The key itself if nothing is behind it, so a typo shows up on screen rather than as a nil further down.

  Example:
  ```lua
  trx.console.log(trx.locale.get("general/misc/off"))
  ```

- [lua]`trx.locale.format(key, ...)`  
  The text behind a key with its placeholders filled in.

  Parameters:
  - **`key`** (string). The key.
  - **`...`** (any). What to fill the placeholders with.

  Returns: string. The text with the arguments in it. A translation whose placeholders do not line up with the arguments comes back unformatted, with a warning in the log: a player is better served by text they can read than by a script that stops.

  Example:
  ```lua
  trx.console.log(trx.locale.format("general/misc/pagination_nav", 1, 5))
  ```
