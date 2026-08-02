---
title: Assault course
order: 16
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/assault.lua. Edit it there.
-->

## <a id="assault" name="assault"></a>Assault course module

Module for controlling the Assault Course and Quad Bike timers in gym levels.

### Properties

- <a id="assault.active_track" name="assault.active_track"></a>**`trx.assault.active_track`** ([trx.assault.Track](#assault.Track)). The track Lara is currently running, or `nil` if none. *(read-only)*

### Enums

- <a id="assault.Track" name="assault.Track"></a>[lua]`trx.assault.Track`

    A timed gym track.

    - `trx.assault.Track.QUAD` = `0`  
        The quad bike circuit.
    - `trx.assault.Track.COURSE` = `1`  
        Lara's assault course.

### Functions

- <a id="assault.stats" name="assault.stats"></a>[lua]`trx.assault.stats`  
  A track's record table, as shown on the stats screen. Each track keeps its own. The records are stored in the player's profile, so writing to them outlives the level, and they can be read outside a gym level.

- <a id="assault.start" name="assault.start"></a>[lua]`trx.assault.start([track])`  
  Starts the timer and clears its state. Raises outside a gym level.

  Parameters:
  - **`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

- <a id="assault.stop" name="assault.stop"></a>[lua]`trx.assault.stop([track])`  
  Stops the timer, leaving it on screen. Raises outside a gym level.

  Parameters:
  - **`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

- <a id="assault.finish" name="assault.finish"></a>[lua]`trx.assault.finish([track])`  
  Stops the timer as completing the track does, rather than as an abort. Raises outside a gym level.

  Parameters:
  - **`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

- <a id="assault.reset" name="assault.reset"></a>[lua]`trx.assault.reset([track])`  
  Stops the timer and clears its state. Raises outside a gym level.

  Parameters:
  - **`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

- <a id="assault.is_running" name="assault.is_running"></a>[lua]`trx.assault.is_running([track])`  
  Whether the timer is counting. False outside a gym level.

  Parameters:
  - **`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: boolean.

- <a id="assault.is_visible" name="assault.is_visible"></a>[lua]`trx.assault.is_visible([track])`  
  Whether the timer is shown on screen. It stays visible after [`trx.assault.stop`](#assault.stop).

  Parameters:
  - **`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: boolean.

- <a id="assault.stats.add_record" name="assault.stats.add_record"></a>[lua]`trx.assault.stats.add_record(time, [track])`  
  Files a new record, inserting it in time order and bumping the attempt count.

  Parameters:
  - **`time`** (number). Time in seconds. Must be greater than zero.
  - **`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: boolean. `false` if the table is full and the time is slower than every record in it.

  Example:
  ```lua
  trx.assault.stats.add_record(30.0)
  ```

- <a id="assault.stats.remove_record" name="assault.stats.remove_record"></a>[lua]`trx.assault.stats.remove_record(record_num, [track])`  
  Removes a record, closing the gap behind it.

  Parameters:
  - **`record_num`** (integer). Position in the table. Counted from 1.
  - **`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: boolean. `false` if there is no record at that position.

- <a id="assault.stats.list_records" name="assault.stats.list_records"></a>[lua]`trx.assault.stats.list_records([track])`  
  The records, fastest first.

  Parameters:
  - **`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: table. List of `{ time = seconds, attempt_num = which attempt it was }`.

  Example:
  ```lua
  for _, record in ipairs(trx.assault.stats.list_records(trx.assault.Track.QUAD)) do
    trx.log.info(("attempt %d: %.2fs"):format(record.attempt_num, record.time))
  end
  ```
