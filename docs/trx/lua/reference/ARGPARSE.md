---
title: Argparse
order: 18
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/argparse.lua. Edit it there.
-->

## Argparse module

A small, declarative argument parser for console commands, in the shape of
Python's argparse.

A parser both reads a command's arguments and offers completions for them,
from one declaration. Every command written with `trx.console.register` has
one; a command shapes it through the `args` function it hands over, and `run`
then receives a table of parsed values. A command that shapes nothing takes no
arguments, and is told so when given one.

Every parser answers `-h` and `--help` on its own, printing what it accepts.

How a positional reads a token is its `matcher`, one of:

- `type` - coerce to `"integer"`, `"number"`, `"string"` or `"boolean"`.
- `choices` - the allowed set: a list of values or a `function(parsed)`
  returning one. The token must match one; the set is shown in errors and
  completes.
- `match` - a `function(token, parsed)` returning `value, ok`, for a shape of its own.

These do not combine on one positional; a value that is a number *or* a name is
two matchers, declared with `any_of`. Separately, `suggest` offers completions
without restricting or being shown in errors - for a free value with a long
list behind it, like a setting name.

A parser has these methods, each returning the parser so calls chain:

- `positional(name, opts)` - a positional with one matcher (`opts.type`,
  `opts.choices` or `opts.match`). `opts.optional` lets it be left out;
  `opts.greedy` reads the rest of the line as one token, so a value with spaces
  in it still arrives whole; `opts.suggest` completes it; `opts.help` describes
  it.
- `any_of(name, alternatives, opts)` - a positional whose value is the first of
  several matchers to take the token. Each alternative is a matcher table,
  `{ type = ... }` or `{ choices = ... }`. Same `opts` as `positional`.
- `rest(name, opts)` - the rest of the line from here on, verbatim as one
  string, or nil when an optional one is absent. Always the last argument;
  `opts.suggest` completes it.
- `flag(name, opts)` - a boolean that may sit anywhere. `opts.short` and
  `opts.long` are the spellings, e.g. `"-f"` and `"--force"`; `opts.help`
  describes it.
- `parse(args)` - reads the argument string, returning a table of values, or
  `nil` and a structured error the console layer turns into localized text. A
  value carried by a `{ key, value }` choice comes back as its `value`;
  `-h`/`--help` comes back as `{ help = true }`.
- `complete(text, caret)` - the candidate completions for the token the caret
  sits in, and the byte offsets `start, end` of the run they replace (reaching
  to the end of the line for a greedy argument).
- `usage()` - a short description of what the command accepts.

A choice is either a bare string, where the key and value are the same, or a
`{ key, value }` pair, where `key` is matched and shown and `value` is what
`parse` gives back. Matching is forgiving, through `trx.strings.fuzzy_match`.

### Functions

- [lua]`trx.argparse.new([spec])`  
  Creates an argument parser. See the module description for the parser's methods.

  Parameters:
  - **`spec`** (table, optional). `prog`: the command word, for messages. `description`: what the command does.

  Returns: table. A parser. Describe its arguments with `positional`, `any_of`, `rest` and `flag`, then read them with `parse` or offer completions with `complete`.

  Example:
  ```lua
  local p = trx.argparse.new({ prog = "weather" })
  p:positional("state", { choices = { "snow", "rain", "none" } })
  local parsed = p:parse("snow")  -- { state = "snow" }
  ```
