---
title: Console
order: 5
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  data/scripting/api/console.lua. Edit it there.
-->

## Console module

Module for interacting with the developer console.

`trx.console.log` writes to the console overlay in-game, where `trx.log` writes only to the terminal and the log file.

### Functions

- [lua]`trx.console.log(message)`  
  Logs a line to the developer console. Calling the group itself logs at `INFO`.

  Parameters:
  - **`message`** (string).

  Example:
  ```lua
  trx.console.log("hello")
  ```

- [lua]`trx.console.log.generic(level, message)`  
  Logs at a level chosen at runtime.

  Parameters:
  - **`level`** (integer). Compare against `trx.log.LogLevel`.
  - **`message`** (string).

- [lua]`trx.console.log.info(message)`  
  Logs an informational message.

  Parameters:
  - **`message`** (string).

- [lua]`trx.console.log.warn(message)`  
  Logs a warning.

  Parameters:
  - **`message`** (string).

- [lua]`trx.console.log.warning(message)`  
  Logs a warning. An alias of `trx.console.log.warn`.

  Parameters:
  - **`message`** (string).

- [lua]`trx.console.log.error(message)`  
  Logs an error.

  Parameters:
  - **`message`** (string).

- [lua]`trx.console.log.debug(message)`  
  Logs a debug message.

  Parameters:
  - **`message`** (string).

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

- [lua]`trx.console.clear()`  
  Clears the console.
