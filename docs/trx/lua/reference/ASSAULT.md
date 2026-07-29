---
title: Assault course
order: 14
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/assault.lua. Edit it there.
-->

## Assault course module

Module for controlling the Assault Course and Quad Bike timers in gym levels.

### Properties

- **`trx.assault.active_track`** (integer). The track Lara is currently running, or `nil` if none. Compare against `trx.assault.Track`. *(read-only)*

### Enums

- [lua]`trx.assault.Track`

    A timed gym track.

    - `trx.assault.Track.QUAD` = `0`  
        The quad bike circuit.
    - `trx.assault.Track.COURSE` = `1`  
        Lara's assault course.

### Functions

- [lua]`trx.assault.stats`  
  A track's record table, as shown on the stats screen. Each track keeps its own. The records are stored in the player's profile, so writing to them outlives the level, and they can be read outside a gym level.

- [lua]`trx.assault.start([track])`  
  Starts the timer and clears its state. Raises outside a gym level.

  Parameters:
  - **`track`** (integer, optional, default `trx.assault.Track.COURSE`). Compare against `trx.assault.Track`.

- [lua]`trx.assault.stop([track])`  
  Stops the timer, leaving it on screen. Raises outside a gym level.

  Parameters:
  - **`track`** (integer, optional, default `trx.assault.Track.COURSE`). Compare against `trx.assault.Track`.

- [lua]`trx.assault.finish([track])`  
  Stops the timer as completing the track does, rather than as an abort. Raises outside a gym level.

  Parameters:
  - **`track`** (integer, optional, default `trx.assault.Track.COURSE`). Compare against `trx.assault.Track`.

- [lua]`trx.assault.reset([track])`  
  Stops the timer and clears its state. Raises outside a gym level.

  Parameters:
  - **`track`** (integer, optional, default `trx.assault.Track.COURSE`). Compare against `trx.assault.Track`.

- [lua]`trx.assault.is_running([track])`  
  Whether the timer is counting. False outside a gym level.

  Parameters:
  - **`track`** (integer, optional, default `trx.assault.Track.COURSE`). Compare against `trx.assault.Track`.

  Returns: boolean.

- [lua]`trx.assault.is_visible([track])`  
  Whether the timer is shown on screen. It stays visible after `stop`.

  Parameters:
  - **`track`** (integer, optional, default `trx.assault.Track.COURSE`). Compare against `trx.assault.Track`.

  Returns: boolean.

- [lua]`trx.assault.stats.add_record(time, [track])`  
  Files a new record, inserting it in time order and bumping the attempt count.

  Parameters:
  - **`time`** (number). Time in seconds. Must be greater than zero.
  - **`track`** (integer, optional, default `trx.assault.Track.COURSE`). Compare against `trx.assault.Track`.

  Returns: boolean. `false` if the table is full and the time is slower than every record in it.

  Example:
  ```lua
  trx.assault.stats.add_record(30.0)
  ```

- [lua]`trx.assault.stats.remove_record(record_id, [track])`  
  Removes a record, closing the gap behind it.

  Parameters:
  - **`record_id`** (integer). 1-based position in the table.
  - **`track`** (integer, optional, default `trx.assault.Track.COURSE`). Compare against `trx.assault.Track`.

  Returns: boolean. `false` if there is no record at that position.

- [lua]`trx.assault.stats.list_records([track])`  
  The records, fastest first.

  Parameters:
  - **`track`** (integer, optional, default `trx.assault.Track.COURSE`). Compare against `trx.assault.Track`.

  Returns: table. List, counted from one, of `{ time = seconds, attempt_num = which attempt it was }`.

  Example:
  ```lua
  for _, record in ipairs(trx.assault.stats.list_records(trx.assault.Track.QUAD)) do
    trx.log.info(("attempt %d: %.2fs"):format(record.attempt_num, record.time))
  end
  ```
