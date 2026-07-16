---
title: Miscellaneous
order: 9
---

## Miscellaneous functions

- [lua]`assert(condition)`  
    Logs a detailed error message to the console if something is not true.
- [lua]`print(message)`  
    Logs a basic string to the console without extra decorations like timestamp or the module name.
- [lua]`p(value)`  
    A global shorthand for [`trx.console.log`](CONSOLE.md), for quick debugging from the console. Logs any value at `INFO`; a table is pretty-printed across lines.
