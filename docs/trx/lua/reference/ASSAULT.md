---
title: Assault course
order: 14
---

## Assault course module

Module for controlling the Assault Course and Quad Bike timers in gym levels.

### Enums

- [lua]`trx.assault.Track`
    Values: `COURSE`, `QUAD`.

### Properties

- [lua]`trx.assault.stats`
    Table for controlling Assault Course records.

### Functions

- [lua]`trx.assault.start([track])`  
    Starts the given timer and resets its state. Defaults to `trx.assault.Track.COURSE`.

- [lua]`trx.assault.stop([track])`  
    Stops the given timer while keeping it visible. Defaults to `trx.assault.Track.COURSE`.

- [lua]`trx.assault.reset([track])`  
    Stops the given timer and clears its state. Defaults to `trx.assault.Track.COURSE`.

## Assault course stats

### Functions

- [lua]`trx.assault.stats.add_record(time)`  
    Adds a new record with the given time in seconds. Increments the internal attempt number.

- [lua]`trx.assault.stats.remove_record(record_id)`  
    Removes a record at the given position, with ids starting from 1.

- [lua]`trx.assault.stats.list_records()`  
    Returns a list of record times.
    Structure:
    - `time`: time in seconds.
    - `attempt_num`: which attempt this was.
