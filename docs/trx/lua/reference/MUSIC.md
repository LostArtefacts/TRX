---
title: Music
order: 22
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/music.lua. Edit it there.
-->

## <a id="music" name="music"></a>Music module

Module for playing and controlling the soundtrack.

### Indexing

The soundtrack's streams: `[1]` is the main stream, `[2]` onwards the overlay slots. A slot that is not playing still answers, with a stale handle.

- <a id="music.streams[]" name="music.streams[]"></a>**`trx.music.streams[key]`** (key: [trx.music.StreamNum](#music.StreamNum), value: [trx.music.Stream](#music.Stream) or `nil`).
- **`#trx.music.streams`** (integer). How many there are.

The tracks the current level carries. A level does not carry every number, so indexing one it lacks is `nil` and iterating passes it by.

- <a id="music.tracks[]" name="music.tracks[]"></a>**`trx.music.tracks[key]`** (key: [trx.music.TrackNum](#music.TrackNum), value: [trx.music.Track](#music.Track) or `nil`).
- **`#trx.music.tracks`** (integer). How many there are.

### Properties

- <a id="music.current_track" name="music.current_track"></a>**`trx.music.current_track`** ([trx.music.Track](#music.Track)). The track playing now, or `nil` when nothing plays. *(read-only)*
- <a id="music.looped_track" name="music.looped_track"></a>**`trx.music.looped_track`** ([trx.music.Track](#music.Track)). The ambient track that resumes once the current one-shot finishes, or `nil` when none is set. *(read-only)*

### Enums

- <a id="music.PlayMode" name="music.PlayMode"></a>[lua]`trx.music.PlayMode`

    How a track is played. Pass one as [`trx.music.play.opts.mode`](#music.play.opts.mode).

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

- <a id="music.TrackNum" name="music.TrackNum"></a>[lua]`trx.music.TrackNum`

    Track number, in the numbering the loaded level carries. Not a [`trx.catalog.music`](CATALOG.md#catalog.music) name, which is the soundtrack's own. Counted from 0.

- <a id="music.StreamNum" name="music.StreamNum"></a>[lua]`trx.music.StreamNum`

    Which of the soundtrack's slots: 1 is the main stream, 2 onwards the overlays. Counted from 1.

- <a id="music.Stream" name="music.Stream"></a>[lua]`trx.music.Stream`

    One of the soundtrack's playing streams: the main stream, or an overlay. Reach them through [`trx.music.streams`](#music.streams). A handle to a slot that is not playing goes stale, so reading a field or calling a method on it raises; check [`is_valid`](#music.Stream.is_valid) first.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="music.Stream.mode" name="music.Stream.mode"></a>**`mode`**: [trx.music.PlayMode](#music.PlayMode). How the track is playing. *(read-only)*
    - <a id="music.Stream.timestamp" name="music.Stream.timestamp"></a>**`timestamp`**: [trx.game.Seconds](GAME.md#game.Seconds). How far into the track the stream is. *(read-only)*
    - <a id="music.Stream.track_num" name="music.Stream.track_num"></a>**`track_num`**: [trx.music.TrackNum](#music.TrackNum). The track this stream is playing. *(read-only)*

    Methods:

    - <a id="music.Stream.is_valid" name="music.Stream.is_valid"></a>[lua]`stream:is_valid()`  
      Whether the slot is still playing. A stream that has finished, or been stopped, leaves its handle stale.

      Returns: boolean. False once the slot has gone quiet.

    - <a id="music.Stream.pause" name="music.Stream.pause"></a>[lua]`stream:pause()`  
      Pauses this stream.

    - <a id="music.Stream.seek" name="music.Stream.seek"></a>[lua]`stream:seek(timestamp)`  
      Seeks this stream to a timestamp.

      Parameters:
      - <a id="music.Stream.seek.timestamp" name="music.Stream.seek.timestamp"></a>**`timestamp`** ([trx.game.Seconds](GAME.md#game.Seconds)). Where to seek to.

      Returns: boolean. Whether the seek took.

    - <a id="music.Stream.stop" name="music.Stream.stop"></a>[lua]`stream:stop()`  
      Stops this stream. Stopping the main stream lets a deferred ambient loop resume; an overlay just ends.

    - <a id="music.Stream.unpause" name="music.Stream.unpause"></a>[lua]`stream:unpause()`  
      Resumes this stream.

- <a id="music.Track" name="music.Track"></a>[lua]`trx.music.Track`

    A track the current level carries. Reach them through [`trx.music.tracks`](#music.tracks), or as [`trx.music.current_track`](#music.current_track). A handle to a track the loaded level does not carry goes stale, so [`is_valid`](#music.Track.is_valid) answers whether it is still there.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="music.Track.num" name="music.Track.num"></a>**`num`**: [trx.music.TrackNum](#music.TrackNum). *(read-only)*

    Methods:

    - <a id="music.Track.is_valid" name="music.Track.is_valid"></a>[lua]`track:is_valid()`  
      Whether the loaded level still carries this track.

      Returns: boolean. False once a level change has replaced the tracks.

    - <a id="music.Track.path" name="music.Track.path"></a>[lua]`track:path()`  
      Resolves the track's file path.

      Returns: string or `nil`. `nil` when there is no file, e.g. a CD-audio soundtrack.

    - <a id="music.Track.play" name="music.Track.play"></a>[lua]`track:play([opts])`  
      Plays this track.

      Parameters:
      - <a id="music.Track.play.opts" name="music.Track.play.opts"></a>**`opts`** (table, optional). How to play it.

        Keys:
        - <a id="music.Track.play.opts.mode" name="music.Track.play.opts.mode"></a>**`mode`** ([trx.music.PlayMode](#music.PlayMode), optional). Plays once by default.

      Returns: [trx.music.Stream](#music.Stream) or `nil`. The stream it started, or `nil` if none did.

### Functions

- <a id="music.play" name="music.play"></a>[lua]`trx.music.play(id, [opts])`  
  Plays a track by catalog id, mapping it to the level's own track. A game that does not carry the track plays nothing.

  Parameters:
  - <a id="music.play.id" name="music.play.id"></a>**`id`** ([trx.catalog.music](CATALOG.md#catalog.music)). Track to play. To reach a track by the level's own slot, play it through a handle: `trx.music.tracks[slot]:play()`.
  - <a id="music.play.opts" name="music.play.opts"></a>**`opts`** (table, optional). How to play it.

    Keys:
    - <a id="music.play.opts.mode" name="music.play.opts.mode"></a>**`mode`** ([trx.music.PlayMode](#music.PlayMode), optional). Plays once by default.

  Returns: [trx.music.Stream](#music.Stream) or `nil`. The stream it started, or `nil` if none did.

  Example:
  ```lua
  trx.music.play(trx.catalog.music.SECRET)
  trx.music.play(trx.catalog.music.SECRET, { mode = trx.music.PlayMode.LOOP })
  ```

- <a id="music.pause" name="music.pause"></a>[lua]`trx.music.pause()`  
  Pauses the music.

- <a id="music.unpause" name="music.unpause"></a>[lua]`trx.music.unpause()`  
  Resumes paused music.

- <a id="music.stop" name="music.stop"></a>[lua]`trx.music.stop()`  
  Stops all music.
