---
title: Strings
order: 19
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  data/scripting/strings.lua. Edit it there.
-->

## Strings module

Utilities for working with strings.



### Functions

- [lua]`trx.strings.fuzzy_match(input, sources)`  
  Matches what someone typed against a list of candidates, forgivingly: `big medi` finds `large medipack`.

  Candidates are ranked, best first. Each carries a `value` of the caller's choosing, which comes back untouched on the match - hang an id off it and read it back.

  Parameters:
  - **`input`** (string). What the player typed.
  - **`sources`** (table). List of `{ key = <the name>, value = <anything>, weight = <integer> }`. A heavier candidate wins a tie; weight defaults to 1.

  Returns: table. The matches, best first: `{ key, value, score, is_full, is_word }`. `is_full` means the whole candidate matched, `is_word` that a whole word did.

  Example:
  ```lua
  local matches = trx.strings.fuzzy_match("wolf", {
    { key = "wolf", value = trx.catalog.objects.WOLF },
    { key = "bear", value = trx.catalog.objects.BEAR },
  })
  local best = matches[1]
  ```

- [lua]`trx.strings.regex_match(subject, pattern)`  
  Whether a subject matches a regular expression. Case-insensitive.

  Parameters:
  - **`subject`** (string).
  - **`pattern`** (string). A PCRE regular expression.

  Returns: boolean.

  Example:
  ```lua
  if trx.strings.regex_match(args, "^%d+$") then ... end
  ```
