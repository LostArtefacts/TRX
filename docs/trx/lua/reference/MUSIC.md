---
title: Music
order: 6
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  data/scripting/music.lua. Edit it there.
-->

## Music module

Module for playing and controlling the soundtrack.

### Enums

- [lua]`trx.music.PlayMode`

    How a track is played. Pass one as `opts.mode` to `trx.music.play`.

    - `trx.music.PlayMode.ONCE` = `0`  
        Plays the track once. When it finishes, any active looped track resumes from its start.
    - `trx.music.PlayMode.LOOP` = `1`  
        Plays the track continuously. It becomes the ambient track.
    - `trx.music.PlayMode.DELAY` = `2`  
        Marks the track for later playback rather than starting it now.
    - `trx.music.PlayMode.NO_REPEAT` = `3`  
        Plays the track once, but does not retrigger it if it is already playing.
    - `trx.music.PlayMode.OVERLAY` = `4`  
        Plays the track on top of the current one.

### Functions

- [lua]`trx.music.get_track()`  
  The track currently playing.

  Returns: integer or `nil`. `nil` if nothing is playing. Compare against `trx.catalog.music`.

- [lua]`trx.music.play(id, [opts])`  
  Plays a track. Raises if the track or the mode is invalid.

  Parameters:
  - **`id`** (integer). Track to play. Compare against `trx.catalog.music`.
  - **`opts`** (table, optional). `mode`: a `trx.music.PlayMode`. Defaults to `ONCE`.

  Example:
  ```lua
  trx.music.play(1)
  trx.music.play(2, { mode = trx.music.PlayMode.LOOP })
  ```

- [lua]`trx.music.pause()`  
  Pauses the music.

- [lua]`trx.music.unpause()`  
  Resumes paused music.

- [lua]`trx.music.stop()`  
  Stops all music.
