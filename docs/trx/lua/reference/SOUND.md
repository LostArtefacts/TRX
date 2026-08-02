---
title: Sound
order: 17
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/sound.lua. Edit it there.
-->

## Sound module

Module for playing sound effects.

### Properties

- <a name="sound.samples"></a>**`trx.sound.samples`** (table). The samples the current level carries, as [`trx.sound.Sample`](#sound.Sample) handles keyed by id: `trx.sound.samples[99]` is sample 99, or `nil` if the level has no such sample. `#` counts them, iterating walks them, and both reach one handle at a time. *(read-only)*
- <a name="sound.streams"></a>**`trx.sound.streams`** (table). The sound effects playing now, as [`trx.sound.Stream`](#sound.Stream) handles. A slot that is silent still answers, with a stale handle. Indexing and iterating reach one handle at a time. *(read-only)*

### Structures

- <a name="sound.Sample"></a>[lua]`trx.sound.Sample`

    A sound sample the current level carries. Reach them through [`trx.sound.samples`](#sound.samples). A handle to a sample the loaded level does not carry goes stale, so `is_valid()` answers whether it is still there.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a name="sound.Sample.num"></a>**`num`**: integer. The number the level gives this sample. *(read-only)*
    - <a name="sound.Sample.pitch"></a>**`pitch`**: integer. The sample's base pitch. *(read-only)*
    - <a name="sound.Sample.randomness"></a>**`randomness`**: integer. How much the sample's playback is randomized. *(read-only)*
    - <a name="sound.Sample.range"></a>**`range`**: integer. How far the sample carries. *(read-only)*
    - <a name="sound.Sample.volume"></a>**`volume`**: integer. The sample's base volume. *(read-only)*

    Methods:

    - <a name="sound.Sample.is_valid"></a>[lua]`sample:is_valid()`  
      Whether the loaded level still carries this sample.

      Returns: boolean.

    - <a name="sound.Sample.play"></a>[lua]`sample:play([opts])`  
      Plays this sample.

      Parameters:
      - **`opts`** (table, optional). `pos`: a `{ x =, y =, z = }` world position to play from, which applies pan and volume. Omit to play at full volume.

      Returns: [trx.sound.Stream](#sound.Stream) or `nil`. The voice it started, or `nil` if none did.

    - <a name="sound.Sample.stop"></a>[lua]`sample:stop()`  
      Stops every voice playing this sample.

- <a name="sound.Stream"></a>[lua]`trx.sound.Stream`

    One of the sound effects playing now. Reach them through [`trx.sound.streams`](#sound.streams). A handle to a voice that has fallen silent goes stale, so check `is_valid()` first.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a name="sound.Stream.sample_num"></a>**`sample_num`**: integer. The sample this voice is playing, in the level's own numbering. *(read-only)*

    Methods:

    - <a name="sound.Stream.is_valid"></a>[lua]`stream:is_valid()`  
      Whether this voice is still playing.

      Returns: boolean.

    - <a name="sound.Stream.pause"></a>[lua]`stream:pause()`  
      Pauses this voice.

    - <a name="sound.Stream.stop"></a>[lua]`stream:stop()`  
      Stops this voice.

    - <a name="sound.Stream.unpause"></a>[lua]`stream:unpause()`  
      Resumes this voice.

### Functions

- <a name="sound.play"></a>[lua]`trx.sound.play(id, [opts])`  
  Plays a sound effect by catalog id, mapping it to the level's own sample. A game that does not carry the sample plays nothing.

  Parameters:
  - **`id`** ([trx.catalog.samples](CATALOG.md#catalog.samples)). Sample to play. To reach a sample by the level's own slot, play it through a handle: `trx.sound.samples[slot]:play()`.
  - **`opts`** (table, optional). `pos`: a `{ x =, y =, z = }` world position to play from, which applies pan and volume. Omit to play at full volume.

  Returns: [trx.sound.Stream](#sound.Stream) or `nil`. The voice it started, or `nil` if none did.

  Example:
  ```lua
  trx.sound.play(trx.catalog.samples.LARA_NO)
  trx.sound.play(trx.catalog.samples.LARA_NO, { pos = { x = 100, y = 200, z = 50 } })
  ```

- <a name="sound.stop"></a>[lua]`trx.sound.stop(id)`  
  Stops a sound effect by catalog id.

  Parameters:
  - **`id`** ([trx.catalog.samples](CATALOG.md#catalog.samples)). Sample to stop. To reach a sample by the level's own slot, stop it through a handle: `trx.sound.samples[slot]:stop()`.

- <a name="sound.stop_all"></a>[lua]`trx.sound.stop_all()`  
  Stops every sound effect currently playing.
