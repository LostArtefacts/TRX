---
title: Console
order: 5
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

  `run` is called with whatever the player typed after the command word, trimmed. What it gives back is a `trx.console.Result`, and returning nothing means `OK`. It may return a message after that, which is logged to the console - as an error, for any result but `OK`.

  A command lives for the whole run, so it can only be registered from a global script. A level script raises if it calls this: it runs again every time its level is loaded.

  Parameters:
  - **`spec`** (table). `name`: the word the player types. `help`: a game string key for the help text, optional. `run`: the function. `aliases`: other words that reach the same command, optional; they dispatch but stay out of the command listing, and the help for `name` shows them.

  Example:
  ```lua
  trx.console.register({
    name = "greet",
    aliases = { "hello", "hi" },
    run = function(args)
      if args == "" then
        return trx.console.Result.BAD_INVOCATION, "greet who?"
      end
      trx.console.log("hello " .. args)
    end,
  })
  ```

- [lua]`trx.console.clear()`  
  Clears the console.
