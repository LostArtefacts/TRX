---
title: Argparse
order: 24
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/argparse.lua. Edit it there.
-->

## <a id="argparse" name="argparse"></a>Argparse module

A small, declarative argument parser for console commands, in the shape of
Python's argparse.

A parser both reads a command's arguments and offers completions for them,
from one declaration. Every command written with [`trx.console.register`](CONSOLE.md#console.register) has
one; a command shapes it through the [`trx.console.register.spec.args`](CONSOLE.md#console.register.spec.args) function
it hands over, and [`trx.console.register.spec.run`](CONSOLE.md#console.register.spec.run) then receives a table of
parsed values. A command that shapes nothing takes no arguments, and is told so
when given one.

Every parser answers `-h` and `--help` on its own, printing what it accepts.

How a positional reads a token is its matcher: a type to coerce to, a set of
choices, or a function of its own. These do not combine on one positional; a
value that is a number *or* a name is two matchers, declared with
[`trx.argparse.Parser:any_of`](#argparse.Parser.any_of).

Positionals are read in order, and an optional one a token does not fit is
passed over: the token goes to the next positional, and the one skipped stays
nil. That is what lets a command take a leading argument it can also be used
without - a verb before a value, a count before a name. Completion follows the
same path, so a slot offers what every argument reachable from it takes. A
token nothing takes is reported against the first argument that refused it.

A choice is either a bare string, where the key and value are the same, or a
`{ key, value }` pair, where the key is matched and shown and the value is what
[`trx.argparse.Parser:parse`](#argparse.Parser.parse) gives back. Matching is forgiving, through
[`trx.strings.fuzzy_match`](STRINGS.md#strings.fuzzy_match).

### Structures

- <a id="argparse.Parser" name="argparse.Parser"></a>[lua]`trx.argparse.Parser`

    An argument parser, built up a call at a time. Every method hands the parser back, so the calls chain.

    Methods:

    - <a id="argparse.Parser.any_of" name="argparse.Parser.any_of"></a>[lua]`parser:any_of(name, alternatives, [opts])`  
      Adds a positional whose value is the first of several matchers to take the token, for an argument that is a number or a name.

      Parameters:
      - <a id="argparse.Parser.any_of.name" name="argparse.Parser.any_of.name"></a>**`name`** (string). What the parsed value is keyed by.
      - <a id="argparse.Parser.any_of.alternatives" name="argparse.Parser.any_of.alternatives"></a>**`alternatives`** (a list of table). The ways the token may read, tried in order.

        Each entry:
        - <a id="argparse.Parser.any_of.alternatives.type" name="argparse.Parser.any_of.alternatives.type"></a>**`type`** (string, optional). As for [`positional`](#argparse.Parser.positional).
        - <a id="argparse.Parser.any_of.alternatives.choices" name="argparse.Parser.any_of.alternatives.choices"></a>**`choices`** (any, optional). As for [`positional`](#argparse.Parser.positional).
        - <a id="argparse.Parser.any_of.alternatives.match" name="argparse.Parser.any_of.alternatives.match"></a>**`match`** (function, optional). As for [`positional`](#argparse.Parser.positional).
        - <a id="argparse.Parser.any_of.alternatives.metavar" name="argparse.Parser.any_of.alternatives.metavar"></a>**`metavar`** (string, optional). Names this alternative, which earns it a line of its own in the help.
        - <a id="argparse.Parser.any_of.alternatives.help" name="argparse.Parser.any_of.alternatives.help"></a>**`help`** (string, optional). What this alternative is for.
      - <a id="argparse.Parser.any_of.opts" name="argparse.Parser.any_of.opts"></a>**`opts`** (table, optional). How the argument behaves.

        Keys:
        - <a id="argparse.Parser.any_of.opts.optional" name="argparse.Parser.any_of.opts.optional"></a>**`optional`** (boolean, optional). Lets the argument be left out.
        - <a id="argparse.Parser.any_of.opts.greedy" name="argparse.Parser.any_of.opts.greedy"></a>**`greedy`** (boolean, optional). Reads the rest of the line as one token, so a value with spaces in it still arrives whole.
        - <a id="argparse.Parser.any_of.opts.metavar" name="argparse.Parser.any_of.opts.metavar"></a>**`metavar`** (string, optional). What the argument is called in messages and in the synopsis. Its own name by default.
        - <a id="argparse.Parser.any_of.opts.suggest" name="argparse.Parser.any_of.opts.suggest"></a>**`suggest`** (function, optional). Completions to offer, without restricting what is accepted or being shown in errors. For a free value with a long list behind it, like a setting name.
        - <a id="argparse.Parser.any_of.opts.help" name="argparse.Parser.any_of.opts.help"></a>**`help`** (string, optional). What the argument is for, shown in the help.

      Returns: [trx.argparse.Parser](#argparse.Parser). The same parser, so declarations chain.

    - <a id="argparse.Parser.complete" name="argparse.Parser.complete"></a>[lua]`parser:complete([text], [caret])`  
      The candidate completions for the token the caret sits in. Matching is against the text before the caret.

      Parameters:
      - <a id="argparse.Parser.complete.text" name="argparse.Parser.complete.text"></a>**`text`** (string, optional). The line so far.
      - <a id="argparse.Parser.complete.caret" name="argparse.Parser.complete.caret"></a>**`caret`** (integer, optional). Where the caret sits, as a byte offset. The end of the line by default.

      Returns:
      - a list of string. The best match comes first.
      - integer. Where the run they replace starts. The run is the token, or the whole tail a greedy argument swallows; in whitespace it is empty, at the caret.
      - integer. Where that run ends.

    - <a id="argparse.Parser.flag" name="argparse.Parser.flag"></a>[lua]`parser:flag(name, [opts])`  
      Adds a boolean flag, which may sit anywhere in the line.

      Parameters:
      - <a id="argparse.Parser.flag.name" name="argparse.Parser.flag.name"></a>**`name`** (string). What the parsed value is keyed by. `help` is reserved.
      - <a id="argparse.Parser.flag.opts" name="argparse.Parser.flag.opts"></a>**`opts`** (table, optional). How it is spelled and what it is for.

        Keys:
        - <a id="argparse.Parser.flag.opts.short" name="argparse.Parser.flag.opts.short"></a>**`short`** (string, optional). The short spelling, such as `"-f"`.
        - <a id="argparse.Parser.flag.opts.long" name="argparse.Parser.flag.opts.long"></a>**`long`** (string, optional). The long spelling, such as `"--force"`.
        - <a id="argparse.Parser.flag.opts.help" name="argparse.Parser.flag.opts.help"></a>**`help`** (string, optional). What the flag is for, shown in the help.

      Returns: [trx.argparse.Parser](#argparse.Parser). The same parser, so declarations chain.

    - <a id="argparse.Parser.format_error" name="argparse.Parser.format_error"></a>[lua]`parser:format_error(err)`  
      Turns what a refused line reported into a localized line naming what was wrong and what was expected.

      Parameters:
      - <a id="argparse.Parser.format_error.err" name="argparse.Parser.format_error.err"></a>**`err`** (table). What [`parse`](#argparse.Parser.parse) handed back.

      Returns: string. The line, ready to print.

    - <a id="argparse.Parser.parse" name="argparse.Parser.parse"></a>[lua]`parser:parse([args])`  
      Reads an argument line. A value carried by a `{ key, value }` choice comes back as its value, and `-h`/`--help` comes back as `{ help = true }`.

      Parameters:
      - <a id="argparse.Parser.parse.args" name="argparse.Parser.parse.args"></a>**`args`** (string, optional). The line as the player typed it.

      Returns:
      - table or `nil`. The values, keyed by argument name, or `nil` where the line was refused.
      - table or `nil`. What was wrong, for [`format_error`](#argparse.Parser.format_error) to put into words.

    - <a id="argparse.Parser.positional" name="argparse.Parser.positional"></a>[lua]`parser:positional(name, [opts])`  
      Adds a positional argument, read one way.

      Parameters:
      - <a id="argparse.Parser.positional.name" name="argparse.Parser.positional.name"></a>**`name`** (string). What the parsed value is keyed by.
      - <a id="argparse.Parser.positional.opts" name="argparse.Parser.positional.opts"></a>**`opts`** (table, optional). How it reads its token, and how it behaves. It reads a token one way: name at most one of [`opts.type`](#argparse.Parser.positional.opts.type), [`opts.choices`](#argparse.Parser.positional.opts.choices) and [`opts.match`](#argparse.Parser.positional.opts.match), and use [`any_of`](#argparse.Parser.any_of) for several.

        Keys:
        - <a id="argparse.Parser.positional.opts.type" name="argparse.Parser.positional.opts.type"></a>**`type`** (string, optional). Coerce the token: `"integer"`, `"number"`, `"string"` or `"boolean"`.
        - <a id="argparse.Parser.positional.opts.choices" name="argparse.Parser.positional.opts.choices"></a>**`choices`** (any, optional). The allowed set: a list of values, or a function of the values parsed so far returning one. The token must match one; the set is shown in errors and completes.
        - <a id="argparse.Parser.positional.opts.match" name="argparse.Parser.positional.opts.match"></a>**`match`** (function, optional). A function of the token and the values parsed so far, returning the value and whether it took, for a shape of its own.
        - <a id="argparse.Parser.positional.opts.optional" name="argparse.Parser.positional.opts.optional"></a>**`optional`** (boolean, optional). Lets the argument be left out.
        - <a id="argparse.Parser.positional.opts.greedy" name="argparse.Parser.positional.opts.greedy"></a>**`greedy`** (boolean, optional). Reads the rest of the line as one token, so a value with spaces in it still arrives whole.
        - <a id="argparse.Parser.positional.opts.metavar" name="argparse.Parser.positional.opts.metavar"></a>**`metavar`** (string, optional). What the argument is called in messages and in the synopsis. Its own name by default.
        - <a id="argparse.Parser.positional.opts.suggest" name="argparse.Parser.positional.opts.suggest"></a>**`suggest`** (function, optional). Completions to offer, without restricting what is accepted or being shown in errors. For a free value with a long list behind it, like a setting name.
        - <a id="argparse.Parser.positional.opts.help" name="argparse.Parser.positional.opts.help"></a>**`help`** (string, optional). What the argument is for, shown in the help.

      Returns: [trx.argparse.Parser](#argparse.Parser). The same parser, so declarations chain.

    - <a id="argparse.Parser.rest" name="argparse.Parser.rest"></a>[lua]`parser:rest(name, [opts])`  
      Adds an argument taking the rest of the line from here on, verbatim as one string, or `nil` where an optional one is absent. Always the last argument.

      Parameters:
      - <a id="argparse.Parser.rest.name" name="argparse.Parser.rest.name"></a>**`name`** (string). What the parsed value is keyed by.
      - <a id="argparse.Parser.rest.opts" name="argparse.Parser.rest.opts"></a>**`opts`** (table, optional). How the argument behaves.

        Keys:
        - <a id="argparse.Parser.rest.opts.optional" name="argparse.Parser.rest.opts.optional"></a>**`optional`** (boolean, optional). Lets the argument be left out.
        - <a id="argparse.Parser.rest.opts.greedy" name="argparse.Parser.rest.opts.greedy"></a>**`greedy`** (boolean, optional). Reads the rest of the line as one token, so a value with spaces in it still arrives whole.
        - <a id="argparse.Parser.rest.opts.metavar" name="argparse.Parser.rest.opts.metavar"></a>**`metavar`** (string, optional). What the argument is called in messages and in the synopsis. Its own name by default.
        - <a id="argparse.Parser.rest.opts.suggest" name="argparse.Parser.rest.opts.suggest"></a>**`suggest`** (function, optional). Completions to offer, without restricting what is accepted or being shown in errors. For a free value with a long list behind it, like a setting name.
        - <a id="argparse.Parser.rest.opts.help" name="argparse.Parser.rest.opts.help"></a>**`help`** (string, optional). What the argument is for, shown in the help.

      Returns: [trx.argparse.Parser](#argparse.Parser). The same parser, so declarations chain.

    - <a id="argparse.Parser.usage" name="argparse.Parser.usage"></a>[lua]`parser:usage()`  
      A short description of what the command accepts.

      Returns: string. The synopsis, and a line per argument.

### Functions

- <a id="argparse.new" name="argparse.new"></a>[lua]`trx.argparse.new([spec])`  
  Creates an argument parser.

  Parameters:
  - <a id="argparse.new.spec" name="argparse.new.spec"></a>**`spec`** (table, optional). What the parser calls itself in messages.

    Keys:
    - <a id="argparse.new.spec.prog" name="argparse.new.spec.prog"></a>**`prog`** (string, optional). The command word.
    - <a id="argparse.new.spec.description" name="argparse.new.spec.description"></a>**`description`** (string, optional). What the command does.

  Returns: [trx.argparse.Parser](#argparse.Parser). A parser, with no arguments declared on it yet.

  Example:
  ```lua
  local p = trx.argparse.new({ prog = "weather" })
  p:positional("state", { choices = { "snow", "rain", "none" } })
  local parsed = p:parse("snow")  -- { state = "snow" }
  ```
