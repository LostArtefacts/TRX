---
title: Assault course
order: 18
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

### Structures

- <a id="assault.RecordNum" name="assault.RecordNum"></a>[lua]`trx.assault.RecordNum`

    Where a time sits in the table of best times, fastest first. Counted from 1.

- <a id="assault.AttemptNum" name="assault.AttemptNum"></a>[lua]`trx.assault.AttemptNum`

    Which attempt at a track it was, counted in the order they were made. Counted from 1.

- <a id="assault.Record" name="assault.Record"></a>[lua]`trx.assault.Record`

    One of a track's best times.

    Properties:
    - <a id="assault.Record.attempt_num" name="assault.Record.attempt_num"></a>**`attempt_num`**: [trx.assault.AttemptNum](#assault.AttemptNum).
    - <a id="assault.Record.time" name="assault.Record.time"></a>**`time`**: [trx.game.Seconds](GAME.md#game.Seconds). The time it took.

### Functions

- <a id="assault.stats" name="assault.stats"></a>[lua]`trx.assault.stats`  
  A track's record table, as shown on the stats screen. Each track keeps its own. The records are stored in the player's profile, so writing to them outlives the level, and they can be read outside a gym level.

- <a id="assault.start" name="assault.start"></a>[lua]`trx.assault.start([track])`  
  Starts the timer and clears its state. Raises outside a gym level.

  Parameters:
  - <a id="assault.start.track" name="assault.start.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

- <a id="assault.stop" name="assault.stop"></a>[lua]`trx.assault.stop([track])`  
  Stops the timer, leaving it on screen. Raises outside a gym level.

  Parameters:
  - <a id="assault.stop.track" name="assault.stop.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

- <a id="assault.finish" name="assault.finish"></a>[lua]`trx.assault.finish([track])`  
  Stops the timer as completing the track does, rather than as an abort. Raises outside a gym level.

  Parameters:
  - <a id="assault.finish.track" name="assault.finish.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

- <a id="assault.reset" name="assault.reset"></a>[lua]`trx.assault.reset([track])`  
  Stops the timer and clears its state. Raises outside a gym level.

  Parameters:
  - <a id="assault.reset.track" name="assault.reset.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

- <a id="assault.is_running" name="assault.is_running"></a>[lua]`trx.assault.is_running([track])`  
  Whether the timer is counting. False outside a gym level.

  Parameters:
  - <a id="assault.is_running.track" name="assault.is_running.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: boolean. True from the start of a run until it is finished or stopped.

- <a id="assault.is_visible" name="assault.is_visible"></a>[lua]`trx.assault.is_visible([track])`  
  Whether the timer is shown on screen. It stays visible after [`trx.assault.stop`](#assault.stop).

  Parameters:
  - <a id="assault.is_visible.track" name="assault.is_visible.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: boolean. True while the timer is drawn, counting or not.

- <a id="assault.get_time" name="assault.get_time"></a>[lua]`trx.assault.get_time()`  
  How long the current run has taken.

  This is the level clock, which is what a gym level times its tracks with, so it
  takes no track.

  Returns: [trx.game.Frames](GAME.md#game.Frames). The time on the clock, counting up while the timer runs.

- <a id="assault.get_best_time" name="assault.get_best_time"></a>[lua]`trx.assault.get_best_time([track])`  
  The fastest time the track has on record.

  Parameters:
  - <a id="assault.get_best_time.track" name="assault.get_best_time.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: [trx.game.Frames](GAME.md#game.Frames). The best time, or 0 where the track has none.

- <a id="assault.get_penalty" name="assault.get_penalty"></a>[lua]`trx.assault.get_penalty([track])`  
  The penalty the run has taken for missed pads.

  Parameters:
  - <a id="assault.get_penalty.track" name="assault.get_penalty.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: [trx.game.Frames](GAME.md#game.Frames). The penalty, added to the time when the run is filed.

- <a id="assault.get_target_penalty" name="assault.get_target_penalty"></a>[lua]`trx.assault.get_target_penalty([track])`  
  The penalty the run has taken for missed targets.

  Parameters:
  - <a id="assault.get_target_penalty.track" name="assault.get_target_penalty.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: [trx.game.Frames](GAME.md#game.Frames). The penalty, added to the time when the run is filed.

- <a id="assault.get_penalty_timer" name="assault.get_penalty_timer"></a>[lua]`trx.assault.get_penalty_timer([track])`  
  How much longer a penalty stays on screen.

  Parameters:
  - <a id="assault.get_penalty_timer.track" name="assault.get_penalty_timer.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: [trx.game.Frames](GAME.md#game.Frames). The time left, and 0 where no penalty is shown.

- <a id="assault.get_lap_time" name="assault.get_lap_time"></a>[lua]`trx.assault.get_lap_time([track])`  
  How long the last lap took.

  Parameters:
  - <a id="assault.get_lap_time.track" name="assault.get_lap_time.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: [trx.game.Frames](GAME.md#game.Frames). The lap time, and 0 before a lap is finished.

- <a id="assault.get_lap_timer" name="assault.get_lap_timer"></a>[lua]`trx.assault.get_lap_timer([track])`  
  How much longer the lap times stay on screen.

  Parameters:
  - <a id="assault.get_lap_timer.track" name="assault.get_lap_timer.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: [trx.game.Frames](GAME.md#game.Frames). The time left, and 0 where no lap time is shown.

- <a id="assault.stats.add_record" name="assault.stats.add_record"></a>[lua]`trx.assault.stats.add_record(time, [track])`  
  Files a new record, inserting it in time order and bumping the attempt count.

  Parameters:
  - <a id="assault.stats.add_record.time" name="assault.stats.add_record.time"></a>**`time`** ([trx.game.Seconds](GAME.md#game.Seconds)). Must be greater than zero.
  - <a id="assault.stats.add_record.track" name="assault.stats.add_record.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: boolean. `false` if the table is full and the time is slower than every record in it.

  Example:
  ```lua
  trx.assault.stats.add_record(30.0)
  ```

- <a id="assault.stats.remove_record" name="assault.stats.remove_record"></a>[lua]`trx.assault.stats.remove_record(record_num, [track])`  
  Removes a record, closing the gap behind it.

  Parameters:
  - <a id="assault.stats.remove_record.record_num" name="assault.stats.remove_record.record_num"></a>**`record_num`** ([trx.assault.RecordNum](#assault.RecordNum)).
  - <a id="assault.stats.remove_record.track" name="assault.stats.remove_record.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: boolean. `false` if there is no record at that position.

- <a id="assault.stats.list_records" name="assault.stats.list_records"></a>[lua]`trx.assault.stats.list_records([track])`  
  The records, fastest first.

  Parameters:
  - <a id="assault.stats.list_records.track" name="assault.stats.list_records.track"></a>**`track`** ([trx.assault.Track](#assault.Track), optional, default [`trx.assault.Track.COURSE`](#assault.Track)).

  Returns: a list of [trx.assault.Record](#assault.Record).

  Example:
  ```lua
  for _, record in ipairs(trx.assault.stats.list_records(trx.assault.Track.QUAD)) do
    trx.log.info(("attempt %d: %.2fs"):format(record.attempt_num, record.time))
  end
  ```
