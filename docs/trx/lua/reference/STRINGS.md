---
title: Strings
order: 32
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/strings.lua. Edit it there.
-->

## <a id="strings" name="strings"></a>Strings module

Utilities for working with strings.

Not to be confused with [`trx.locale`](LOCALE.md#locale), which is the text a player reads: this module is about manipulating strings, that one is about which string the player gets.

### Structures

- <a id="strings.Match" name="strings.Match"></a>[lua]`trx.strings.Match`

    A candidate that matched, and how well.

    Properties:
    - <a id="strings.Match.is_full" name="strings.Match.is_full"></a>**`is_full`**: boolean. Whether the whole candidate matched.
    - <a id="strings.Match.is_word" name="strings.Match.is_word"></a>**`is_word`**: boolean. Whether a whole word matched.
    - <a id="strings.Match.key" name="strings.Match.key"></a>**`key`**: string. The candidate that matched.
    - <a id="strings.Match.score" name="strings.Match.score"></a>**`score`**: number. How well it matched.
    - <a id="strings.Match.value" name="strings.Match.value"></a>**`value`**: any, optional. What the candidate carried, where it carried one.

### Functions

- <a id="strings.fuzzy_match" name="strings.fuzzy_match"></a>[lua]`trx.strings.fuzzy_match(input, sources)`  
  Matches what someone typed against a list of candidates, forgivingly: `big medi` finds `large medipack`.

  Candidates are ranked, best first. Each carries a [`sources.value`](#strings.fuzzy_match.sources.value) of the caller's choosing, which comes back untouched on the match - hang an id off it and read it back.

  Parameters:
  - <a id="strings.fuzzy_match.input" name="strings.fuzzy_match.input"></a>**`input`** (string). What the player typed.
  - <a id="strings.fuzzy_match.sources" name="strings.fuzzy_match.sources"></a>**`sources`** (a list of table). The candidates.

    Each entry:
    - <a id="strings.fuzzy_match.sources.key" name="strings.fuzzy_match.sources.key"></a>**`key`** (string). The name to match against. Non-empty.
    - <a id="strings.fuzzy_match.sources.value" name="strings.fuzzy_match.sources.value"></a>**`value`** (any). Anything of the caller's, handed back on the match.
    - <a id="strings.fuzzy_match.sources.weight" name="strings.fuzzy_match.sources.weight"></a>**`weight`** (integer, optional, default `1`). A heavier candidate wins a tie. Zero or less drops it.

  Returns: a list of [trx.strings.Match](#strings.Match). The best match comes first.

  Example:
  ```lua
  local matches = trx.strings.fuzzy_match("wolf", {
    { key = "wolf", value = trx.catalog.objects.WOLF },
    { key = "bear", value = trx.catalog.objects.BEAR },
  })
  local best = matches[1]
  ```

- <a id="strings.parse_bool" name="strings.parse_bool"></a>[lua]`trx.strings.parse_bool(text)`  
  Reads a boolean the way the console does: `1`, `true` or `on` for true, `0`, `false` or `off` for false, in any case. Anything else is not a boolean.

  Parameters:
  - <a id="strings.parse_bool.text" name="strings.parse_bool.text"></a>**`text`** (string). The text to read.

  Returns: boolean or `nil`. `nil` when the text does not name a boolean.

  Example:
  ```lua
  local on = trx.strings.parse_bool("on")
  ```

- <a id="strings.collapse_ranges" name="strings.collapse_ranges"></a>[lua]`trx.strings.collapse_ranges(numbers, [separator])`  
  Writes a list of whole numbers as ranges, so that a long run reads as one: `{ 0, 2, 3, 4, 9 }` becomes `0, 2-4, 9`.

  The list is sorted first, and duplicates survive as they are, so the caller need not tidy up before handing it over.

  Parameters:
  - <a id="strings.collapse_ranges.numbers" name="strings.collapse_ranges.numbers"></a>**`numbers`** (a list of integer). The numbers to write out.
  - <a id="strings.collapse_ranges.separator" name="strings.collapse_ranges.separator"></a>**`separator`** (string, optional). What to put between the parts. Defaults to `", "`.

  Returns: string. Empty when the list is.

  Example:
  ```lua
  trx.strings.collapse_ranges({ 4, 1, 2, 3 }) -- "1-4"
  ```

- <a id="strings.regex_match" name="strings.regex_match"></a>[lua]`trx.strings.regex_match(subject, pattern)`  
  Whether a subject matches a regular expression. Case-insensitive.

  Parameters:
  - <a id="strings.regex_match.subject" name="strings.regex_match.subject"></a>**`subject`** (string). The text to search.
  - <a id="strings.regex_match.pattern" name="strings.regex_match.pattern"></a>**`pattern`** (string). A PCRE regular expression.

  Returns: boolean. True where the pattern matches anywhere in the subject.

  Example:
  ```lua
  if trx.strings.regex_match(args, "^\\d+$") then ... end
  ```

- <a id="strings.dedent" name="strings.dedent"></a>[lua]`trx.strings.dedent(text)`  
  Takes the shared indentation off a block of text, so that a long string
  written inside `[[ ]]` reads as what it says rather than as where it sat in
  the file. Leading and trailing blank lines go too.

  The deepest lines keep the rest of their indentation, since a block may lay
  something out, and four spaces of it is a code block in markdown. Text may
  open on the line the brackets are on, and that line then sets nothing and
  keeps what it has, however the ones under it are written.

  Parameters:
  - <a id="strings.dedent.text" name="strings.dedent.text"></a>**`text`** (string). The text to take in.

  Returns: string. The text at the left margin.

  Example:
  ```lua
  local help = trx.strings.dedent([[
        Usage: /give <what>
          keys   every plot item the level has a place for
      ]])
  ```
