---
title: Console
order: 18
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/console.lua. Edit it there.
-->

## Console module

Module for interacting with the developer console.

`trx.console.log` writes to the console overlay in-game, where `trx.log` writes only to the terminal and the log file.

### Enums

- [lua]`trx.console.Result`

    How a console command went. What a command's `run` gives back.

    - `trx.console.Result.OK` = `0`  
        It worked.
    - `trx.console.Result.FAILURE` = `1`  
        It ran and could not do what was asked.
    - `trx.console.Result.UNAVAILABLE` = `2`  
        It cannot run here - no level is loaded, or the game is in a menu.
    - `trx.console.Result.BAD_INVOCATION` = `3`  
        The player typed it wrong.

### Functions

- [lua]`trx.console.log(message)`  
  Logs a line to the developer console. Calling the group itself logs at `INFO`. Takes any value: a table is pretty-printed, anything else coerced to a string.

  Parameters:
  - **`message`** (any). Any value; a table is pretty-printed.

  Example:
  ```lua
  trx.console.log({ hp = 1000, pos = { x = 1 } })
  ```

- [lua]`trx.console.log.generic(level, message)`  
  Logs at a level chosen at runtime.

  Parameters:
  - **`level`** (integer). Compare against `trx.log.LogLevel`.
  - **`message`** (any). Any value; a table is pretty-printed.

- [lua]`trx.console.log.info(message)`  
  Logs an informational message.

  Parameters:
  - **`message`** (any). Any value; a table is pretty-printed.

- [lua]`trx.console.log.warn(message)`  
  Logs a warning.

  Parameters:
  - **`message`** (any). Any value; a table is pretty-printed.

- [lua]`trx.console.log.warning(message)`  
  Logs a warning. An alias of `trx.console.log.warn`.

  Parameters:
  - **`message`** (any). Any value; a table is pretty-printed.

- [lua]`trx.console.log.error(message)`  
  Logs an error.

  Parameters:
  - **`message`** (any). Any value; a table is pretty-printed.

- [lua]`trx.console.log.debug(message)`  
  Logs a debug message.

  Parameters:
  - **`message`** (any). Any value; a table is pretty-printed.

- [lua]`trx.console.eval(command, [opts])`  
  Runs a string as a developer console command. Raises if the command fails.

  Output is silenced by default and appears only in the terminal and the log file. Pass `{ verbose = true }` to show it in the console as a command typed by the player would.

  Parameters:
  - **`command`** (string). Command to run, as the player would type it.
  - **`opts`** (table, optional). `verbose`: show the command's output.

  Example:
  ```lua
  trx.console.eval("play 1", { verbose = true })
  ```

- [lua]`trx.console.register(spec)`  
  Registers a console command written in Lua.

  Every command has a `trx.argparse` parser. `args` is an optional function that shapes it - it receives the parser and declares the arguments the command takes. A command that omits `args` takes none, and reports so when handed one. The console completes the arguments from the parser, and answers `-h`/`--help` from it.

  `run` receives the parsed values, a table keyed by argument name. What it gives back is a `trx.console.Result`, and returning nothing means `OK`. It may return a message after that, which is logged to the console - as an error, for any result but `OK`. A line the parser rejects is reported with what it expected, without reaching `run`.

  A command lives for the whole run, so it can only be registered from a global script. A level script raises if it calls this: it runs again every time its level is loaded.

  Parameters:
  - **`spec`** (table). `name`: the word the player types. `help`: a game string key for the help text, optional. `args`: a function that shapes the parser, optional. `run`: the function, called with the parsed arguments. `aliases`: other words that reach the same command, optional; they dispatch but stay out of the command listing, and the help for `name` shows them.

  Example:
  ```lua
  trx.console.register({
    name = "greet",
    aliases = { "hello", "hi" },
    args = function(parser)
      parser:positional("who", { help = "who to greet" })
    end,
    run = function(args)
      trx.console.log("hello " .. args.who)
    end,
  })
  ```

- [lua]`trx.console.clear()`  
  Clears the console.

- [lua]`trx.console.commands()`  
  Every registered console command, in registration order. Each is `{ name, aliases, help }`: `name` is the word the player types, `aliases` the other words that reach it (a list, or absent), and `help` the text the console shows for `name --help` (absent when the command carries none). The help command is built on this.

  Returns: table. A list of `{ name, aliases, help }`.

- [lua]`trx.console.command(name)`  
  The command a name reaches, by its own name or an alias, matched as the console matches when it dispatches. The same `{ name, aliases, help }` as an entry of `trx.console.commands`, or nil when nothing answers to the name.

  Parameters:
  - **`name`** (string). The word or alias to look up.

  Returns: table. `{ name, aliases, help }`, or nil.
