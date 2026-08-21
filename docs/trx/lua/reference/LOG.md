---
title: Logging
order: 29
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/log.lua. Edit it there.
-->

## <a id="log" name="log"></a>Logging module

Logs a message to the terminal and to `TRX.log` in the installation directory. Each call records the Lua script's filename, function name and line number.

### Enums

- <a id="log.LogLevel" name="log.LogLevel"></a>[lua]`trx.log.LogLevel`

    Severity of a log message. Pass one to [`trx.log.generic`](#log.generic).

    - `trx.log.LogLevel.DEBUG` = `0`  
        Diagnostic detail, of interest while writing a script.
    - `trx.log.LogLevel.INFO` = `1`  
        Ordinary progress message.
    - `trx.log.LogLevel.WARNING` = `2`  
        Something is wrong, but the script can carry on.
    - `trx.log.LogLevel.ERROR` = `3`  
        Something failed.

### Functions

- <a id="log.generic" name="log.generic"></a>[lua]`trx.log.generic(level, message)`  
  Logs a message at a level chosen at runtime, for when the level is computed rather than written literally.

  Parameters:
  - <a id="log.generic.level" name="log.generic.level"></a>**`level`** ([trx.log.LogLevel](#log.LogLevel)).
  - <a id="log.generic.message" name="log.generic.message"></a>**`message`** (string). The line to log.

  Example:
  ```lua
  local level = ok and trx.log.LogLevel.INFO or trx.log.LogLevel.ERROR
  trx.log.generic(level, "finished")
  ```

- <a id="log.info" name="log.info"></a>[lua]`trx.log.info(message)`  
  Logs an informational message.

  Parameters:
  - <a id="log.info.message" name="log.info.message"></a>**`message`** (string). The line to log.

  Example:
  ```lua
  trx.log.info("hello from lua")
  ```

- <a id="log.warn" name="log.warn"></a>[lua]`trx.log.warn(message)`  
  Logs a warning.

  Parameters:
  - <a id="log.warn.message" name="log.warn.message"></a>**`message`** (string). The line to log.

- <a id="log.warning" name="log.warning"></a>[lua]`trx.log.warning(message)`  
  Logs a warning. An alias of [`trx.log.warn`](#log.warn).

  Parameters:
  - <a id="log.warning.message" name="log.warning.message"></a>**`message`** (string). The line to log.

- <a id="log.error" name="log.error"></a>[lua]`trx.log.error(message)`  
  Logs an error.

  Parameters:
  - <a id="log.error.message" name="log.error.message"></a>**`message`** (string). The line to log.

- <a id="log.debug" name="log.debug"></a>[lua]`trx.log.debug(message)`  
  Logs a debug message.

  Parameters:
  - <a id="log.debug.message" name="log.debug.message"></a>**`message`** (string). The line to log.
