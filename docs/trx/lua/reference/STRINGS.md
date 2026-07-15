---
title: Strings
order: 19
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  data/scripting/api/strings.lua. Edit it there.
-->

## Strings module

Utilities for working with strings.

Not to be confused with `trx.locale`, which is the text a player reads: this module is about manipulating strings, that one is about which string the player gets.

### Functions

- [lua]`trx.strings.fuzzy_match(input, sources)`  
  Matches what someone typed against a list of candidates, forgivingly: `big medi` finds `large medipack`.

  Candidates are ranked, best first. Each carries a `value` of the caller's choosing, which comes back untouched on the match - hang an id off it and read it back.

  Parameters:
  - **`input`** (string). What the player typed.
  - **`sources`** (table). List of `{ key = <the name>, value = <anything>, weight = <integer> }`. The key is a non-empty string. A heavier candidate wins a tie; weight defaults to 1, and a weight of zero or less drops the candidate.

  Returns: table. The matches, best first: `{ key, value, score, is_full, is_word }`. `is_full` means the whole candidate matched, `is_word` that a whole word did.

  Example:
  ```lua
  local matches = trx.strings.fuzzy_match("wolf", {
    { key = "wolf", value = trx.catalog.objects.WOLF },
    { key = "bear", value = trx.catalog.objects.BEAR },
  })
  local best = matches[1]
  ```

- [lua]`trx.strings.parse_bool(text)`  
  Reads a boolean the way the console does: `1`, `true` or `on` for true, `0`, `false` or `off` for false, in any case. Anything else is not a boolean.

  Parameters:
  - **`text`** (string). The text to read.

  Returns: boolean or `nil`. `nil` when the text does not name a boolean.

  Example:
  ```lua
  local on = trx.strings.parse_bool("on")
  ```

- [lua]`trx.strings.regex_match(subject, pattern)`  
  Whether a subject matches a regular expression. Case-insensitive.

  Parameters:
  - **`subject`** (string).
  - **`pattern`** (string). A PCRE regular expression.

  Returns: boolean.

  Example:
  ```lua
  if trx.strings.regex_match(args, "^\\d+$") then ... end
  ```
