---
title: Music
order: 6
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  data/scripting/api/music.lua. Edit it there.
-->

## Music module

Module for playing and controlling the soundtrack.

### Properties

- **`trx.music.streams`** (table). The soundtrack's streams as `trx.music.Stream` handles: `[1]` is the main stream, `[2]` onwards the overlay slots. A slot that is not playing still answers, with a stale handle. Indexing and iterating reach one handle at a time. *(read-only)*
- **`trx.music.tracks`** (table). The tracks the current level carries, as `trx.music.Track` handles keyed by id: `trx.music.tracks[5]` is track 5, or `nil` if the level has no such track. `#` counts them, iterating walks them, and both reach one handle at a time. *(read-only)*
- **`trx.music.current_track`** (Track). The track playing now, as a `trx.music.Track`, or `nil` when nothing plays. *(read-only)*
- **`trx.music.looped_track`** (Track). The ambient track that resumes once the current one-shot finishes, as a `trx.music.Track`, or `nil` when none is set. *(read-only)*

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

### Structures

- [lua]`trx.music.Stream`

    One of the soundtrack's playing streams: the main stream, or an overlay. Reach them through `trx.music.streams`. A handle to a slot that is not playing goes stale, so reading a field or calling a method on it raises; check `is_valid()` first.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`mode`**: integer. How the track is playing. Compare against `trx.music.PlayMode`. *(read-only)*
    - **`timestamp`**: number. How far into the track the stream is, in seconds. *(read-only)*
    - **`track_id`**: integer. The track this stream is playing. Compare against `trx.catalog.music`. *(read-only)*

    Methods:

    - [lua]`stream:is_valid()`  
      Whether the slot is still playing. A stream that has finished, or been stopped, leaves its handle stale.

      Returns: boolean.

    - [lua]`stream:pause()`  
      Pauses this stream.

    - [lua]`stream:seek(timestamp)`  
      Seeks this stream to a timestamp.

      Parameters:
      - **`timestamp`** (number). Where to seek to, in seconds.

      Returns: boolean. Whether the seek took.

    - [lua]`stream:stop()`  
      Stops this stream. Stopping the main stream lets a deferred ambient loop resume; an overlay just ends.

    - [lua]`stream:unpause()`  
      Resumes this stream.

- [lua]`trx.music.Track`

    A track the current level carries. Reach them through `trx.music.tracks`, or as `trx.music.current_track`. A handle to a track the loaded level does not carry goes stale, so `is_valid()` answers whether it is still there.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`id`**: integer. The track's id. Compare against `trx.catalog.music`. *(read-only)*

    Methods:

    - [lua]`track:is_valid()`  
      Whether the loaded level still carries this track.

      Returns: boolean.

    - [lua]`track:path()`  
      Resolves the track's file path.

      Returns: string or `nil`. `nil` when there is no file, e.g. a CD-audio soundtrack.

    - [lua]`track:play([opts])`  
      Plays this track.

      Parameters:
      - **`opts`** (table, optional). `mode`: a `trx.music.PlayMode`. Defaults to `ONCE`.

### Functions

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
