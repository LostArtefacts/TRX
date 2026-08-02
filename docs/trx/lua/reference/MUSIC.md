---
title: Music
order: 18
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/music.lua. Edit it there.
-->

## Music module

Module for playing and controlling the soundtrack.

### Properties

- <a name="music.streams"></a>**`trx.music.streams`** (table). The soundtrack's streams as [`trx.music.Stream`](#music.Stream) handles: `[1]` is the main stream, `[2]` onwards the overlay slots. A slot that is not playing still answers, with a stale handle. Indexing and iterating reach one handle at a time. *(read-only)*
- <a name="music.tracks"></a>**`trx.music.tracks`** (table). The tracks the current level carries, as [`trx.music.Track`](#music.Track) handles keyed by id: `trx.music.tracks[5]` is track 5, or `nil` if the level has no such track. `#` counts them, iterating walks them, and both reach one handle at a time. *(read-only)*
- <a name="music.current_track"></a>**`trx.music.current_track`** ([trx.music.Track](#music.Track)). The track playing now, or `nil` when nothing plays. *(read-only)*
- <a name="music.looped_track"></a>**`trx.music.looped_track`** ([trx.music.Track](#music.Track)). The ambient track that resumes once the current one-shot finishes, or `nil` when none is set. *(read-only)*

### Enums

- <a name="music.PlayMode"></a>[lua]`trx.music.PlayMode`

    How a track is played. Pass one as `opts.mode` to [`trx.music.play`](#music.play).

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

- <a name="music.Stream"></a>[lua]`trx.music.Stream`

    One of the soundtrack's playing streams: the main stream, or an overlay. Reach them through [`trx.music.streams`](#music.streams). A handle to a slot that is not playing goes stale, so reading a field or calling a method on it raises; check `is_valid()` first.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a name="music.Stream.mode"></a>**`mode`**: [trx.music.PlayMode](#music.PlayMode). How the track is playing. *(read-only)*
    - <a name="music.Stream.timestamp"></a>**`timestamp`**: number. How far into the track the stream is, in seconds. *(read-only)*
    - <a name="music.Stream.track_num"></a>**`track_num`**: integer. The track this stream is playing, in the level's own numbering. *(read-only)*

    Methods:

    - <a name="music.Stream.is_valid"></a>[lua]`stream:is_valid()`  
      Whether the slot is still playing. A stream that has finished, or been stopped, leaves its handle stale.

      Returns: boolean.

    - <a name="music.Stream.pause"></a>[lua]`stream:pause()`  
      Pauses this stream.

    - <a name="music.Stream.seek"></a>[lua]`stream:seek(timestamp)`  
      Seeks this stream to a timestamp.

      Parameters:
      - **`timestamp`** (number). Where to seek to, in seconds.

      Returns: boolean. Whether the seek took.

    - <a name="music.Stream.stop"></a>[lua]`stream:stop()`  
      Stops this stream. Stopping the main stream lets a deferred ambient loop resume; an overlay just ends.

    - <a name="music.Stream.unpause"></a>[lua]`stream:unpause()`  
      Resumes this stream.

- <a name="music.Track"></a>[lua]`trx.music.Track`

    A track the current level carries. Reach them through [`trx.music.tracks`](#music.tracks), or as [`trx.music.current_track`](#music.current_track). A handle to a track the loaded level does not carry goes stale, so `is_valid()` answers whether it is still there.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a name="music.Track.num"></a>**`num`**: integer. The number the level gives this track. *(read-only)*

    Methods:

    - <a name="music.Track.is_valid"></a>[lua]`track:is_valid()`  
      Whether the loaded level still carries this track.

      Returns: boolean.

    - <a name="music.Track.path"></a>[lua]`track:path()`  
      Resolves the track's file path.

      Returns: string or `nil`. `nil` when there is no file, e.g. a CD-audio soundtrack.

    - <a name="music.Track.play"></a>[lua]`track:play([opts])`  
      Plays this track.

      Parameters:
      - **`opts`** (table, optional). `mode`: a [`trx.music.PlayMode`](#music.PlayMode). Defaults to `ONCE`.

      Returns: [trx.music.Stream](#music.Stream) or `nil`. The stream it started, or `nil` if none did.

### Functions

- <a name="music.play"></a>[lua]`trx.music.play(id, [opts])`  
  Plays a track by catalog id, mapping it to the level's own track. A game that does not carry the track plays nothing.

  Parameters:
  - **`id`** ([trx.catalog.music](CATALOG.md#catalog.music)). Track to play. To reach a track by the level's own slot, play it through a handle: `trx.music.tracks[slot]:play()`.
  - **`opts`** (table, optional). `mode`: a [`trx.music.PlayMode`](#music.PlayMode). Defaults to `ONCE`.

  Returns: [trx.music.Stream](#music.Stream) or `nil`. The stream it started, or `nil` if none did.

  Example:
  ```lua
  trx.music.play(trx.catalog.music.SECRET)
  trx.music.play(trx.catalog.music.SECRET, { mode = trx.music.PlayMode.LOOP })
  ```

- <a name="music.pause"></a>[lua]`trx.music.pause()`  
  Pauses the music.

- <a name="music.unpause"></a>[lua]`trx.music.unpause()`  
  Resumes paused music.

- <a name="music.stop"></a>[lua]`trx.music.stop()`  
  Stops all music.
