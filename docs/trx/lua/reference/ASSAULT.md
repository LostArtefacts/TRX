---
title: Assault course
order: 14
---

## Assault course module

Module for controlling the gym assault course timer.

Supports an optional mode argument: `"course"` (default) or `"quad"`.

### Functions

- [lua]`trx.assault.start([mode])`  
    Starts the timer and resets its state.

- [lua]`trx.assault.stop([mode])`  
    Stops the timer while keeping it visible.

- [lua]`trx.assault.reset([mode])`  
    Stops and clears the timer state.
