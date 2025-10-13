---
title: Console
---

## Console module

Module for interacting with the developer console.

### Functions

- [lua]`TRX.Console.Log("string1")`  
    Logs a line to the developer console.

- [lua]`TRX.Console.Eval("string"[, opts])`  
   Evaluates a given string as a developer console command.

   By default, output from commands is silenced and only appears in the
   terminal and the log file. To see output from the commands normally, pass `{ verbose = true }` in `opts`.  

   Example:
   > ```lua
   > TRX.Console.Eval("play 1", { verbose = true })
   > ```
   will play the first level and show an according message in the console log.

- [lua]`TRX.Console.Clear()`  
    Clears the console.
