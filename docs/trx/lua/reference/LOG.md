---
title: Logging
order: 1
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  data/scripting/api/log.lua. Edit it there.
-->

## Logging module

Logs a message to the terminal and to `TRX.log` in the installation directory. Each call records the Lua script's filename, function name and line number.

### Enums

- [lua]`trx.log.LogLevel`

    Severity of a log message. Pass one to `trx.log.generic`.

    - `trx.log.LogLevel.DEBUG` = `0`  
        Diagnostic detail, of interest while writing a script.
    - `trx.log.LogLevel.INFO` = `1`  
        Ordinary progress message.
    - `trx.log.LogLevel.WARNING` = `2`  
        Something is wrong, but the script can carry on.
    - `trx.log.LogLevel.ERROR` = `3`  
        Something failed.

### Functions

- [lua]`trx.log.generic(level, message)`  
  Logs a message at a level chosen at runtime, for when the level is computed rather than written literally.

  Parameters:
  - **`level`** (integer). Compare against `trx.log.LogLevel`.
  - **`message`** (string).

  Example:
  ```lua
  local level = ok and trx.log.LogLevel.INFO or trx.log.LogLevel.ERROR
  trx.log.generic(level, "finished")
  ```

- [lua]`trx.log.info(message)`  
  Logs an informational message.

  Parameters:
  - **`message`** (string).

  Example:
  ```lua
  trx.log.info("hello from lua")
  ```

- [lua]`trx.log.warn(message)`  
  Logs a warning.

  Parameters:
  - **`message`** (string).

- [lua]`trx.log.warning(message)`  
  Logs a warning. An alias of `trx.log.warn`.

  Parameters:
  - **`message`** (string).

- [lua]`trx.log.error(message)`  
  Logs an error.

  Parameters:
  - **`message`** (string).

- [lua]`trx.log.debug(message)`  
  Logs a debug message.

  Parameters:
  - **`message`** (string).
