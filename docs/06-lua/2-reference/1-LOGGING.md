---
title: Logging
---

## Logging module

Lua scripts can log with contextual source info via the global `Log` module.
Each call records the Lua script filename, function name, and line number. The
results are logged to the standard output in the console as well as the
`TR*X.log` file in the installation directory.

### Functions

- [lua]`TRX.Log.Info(message)`  
  Logs an information to the terminal output and the log file.

- [lua]`TRX.Log.Warn(message)`  
  Logs a warning to the terminal output and the log file.

- [lua]`TRX.Log.Error(message)`  
  Logs an error to the terminal output and the log file.

- [lua]`TRX.Log.Debug(message)`  
  Logs a debug message to the terminal output and the log file.

### Examples

```lua
TRX.Log.Info("hello from lua")
```
