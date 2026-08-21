---
title: Sound
order: 20
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/sound.lua. Edit it there.
-->

## <a id="sound" name="sound"></a>Sound module

Module for playing sound effects.

### Indexing

The samples the current level carries. A level does not carry every number, so indexing one it lacks is `nil` and iterating passes it by.

- <a id="sound.samples[]" name="sound.samples[]"></a>**`trx.sound.samples[key]`** (key: [trx.sound.SampleNum](#sound.SampleNum), value: [trx.sound.Sample](#sound.Sample) or `nil`).
- **`#trx.sound.samples`** (integer). How many there are.

The sound effects playing now. A slot that is silent still answers, with a stale handle.

- <a id="sound.streams[]" name="sound.streams[]"></a>**`trx.sound.streams[key]`** (key: [trx.sound.StreamNum](#sound.StreamNum), value: [trx.sound.Stream](#sound.Stream) or `nil`).
- **`#trx.sound.streams`** (integer). How many there are.

### Structures

- <a id="sound.SampleNum" name="sound.SampleNum"></a>[lua]`trx.sound.SampleNum`

    Sample number, in the numbering the loaded level carries. Not a [`trx.catalog.samples`](CATALOG.md#catalog.samples) name, which is the sound bank's own. Counted from 0.

- <a id="sound.StreamNum" name="sound.StreamNum"></a>[lua]`trx.sound.StreamNum`

    Which of the voices playing now, counted in the order the engine holds them. Counted from 1.

- <a id="sound.Sample" name="sound.Sample"></a>[lua]`trx.sound.Sample`

    A sound sample the current level carries. Reach them through [`trx.sound.samples`](#sound.samples). A handle to a sample the loaded level does not carry goes stale, so [`is_valid`](#sound.Sample.is_valid) answers whether it is still there.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="sound.Sample.num" name="sound.Sample.num"></a>**`num`**: [trx.sound.SampleNum](#sound.SampleNum). *(read-only)*
    - <a id="sound.Sample.pitch" name="sound.Sample.pitch"></a>**`pitch`**: integer. The sample's base pitch. *(read-only)*
    - <a id="sound.Sample.randomness" name="sound.Sample.randomness"></a>**`randomness`**: integer. How much the sample's playback is randomized. *(read-only)*
    - <a id="sound.Sample.range" name="sound.Sample.range"></a>**`range`**: integer. How far the sample carries. *(read-only)*
    - <a id="sound.Sample.volume" name="sound.Sample.volume"></a>**`volume`**: integer. The sample's base volume. *(read-only)*

    Methods:

    - <a id="sound.Sample.is_valid" name="sound.Sample.is_valid"></a>[lua]`sample:is_valid()`  
      Whether the loaded level still carries this sample.

      Returns: boolean. False once a level change has replaced the samples.

    - <a id="sound.Sample.play" name="sound.Sample.play"></a>[lua]`sample:play([opts])`  
      Plays this sample.

      Parameters:
      - <a id="sound.Sample.play.opts" name="sound.Sample.play.opts"></a>**`opts`** (table, optional). How to play it.

        Keys:
        - <a id="sound.Sample.play.opts.pos" name="sound.Sample.play.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3), optional). A world position to play from, which applies pan and volume. Omit to play at full volume.

      Returns: [trx.sound.Stream](#sound.Stream) or `nil`. The voice it started, or `nil` if none did.

    - <a id="sound.Sample.stop" name="sound.Sample.stop"></a>[lua]`sample:stop()`  
      Stops every voice playing this sample.

- <a id="sound.Stream" name="sound.Stream"></a>[lua]`trx.sound.Stream`

    One of the sound effects playing now. Reach them through [`trx.sound.streams`](#sound.streams). A handle to a voice that has fallen silent goes stale, so check [`is_valid`](#sound.Stream.is_valid) first.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="sound.Stream.sample_num" name="sound.Stream.sample_num"></a>**`sample_num`**: [trx.sound.SampleNum](#sound.SampleNum). The sample this voice is playing. *(read-only)*

    Methods:

    - <a id="sound.Stream.is_valid" name="sound.Stream.is_valid"></a>[lua]`stream:is_valid()`  
      Whether this voice is still playing.

      Returns: boolean. False once the voice has fallen silent.

    - <a id="sound.Stream.pause" name="sound.Stream.pause"></a>[lua]`stream:pause()`  
      Pauses this voice.

    - <a id="sound.Stream.stop" name="sound.Stream.stop"></a>[lua]`stream:stop()`  
      Stops this voice.

    - <a id="sound.Stream.unpause" name="sound.Stream.unpause"></a>[lua]`stream:unpause()`  
      Resumes this voice.

### Functions

- <a id="sound.play" name="sound.play"></a>[lua]`trx.sound.play(id, [opts])`  
  Plays a sound effect by catalog id, mapping it to the level's own sample. A game that does not carry the sample plays nothing.

  Parameters:
  - <a id="sound.play.id" name="sound.play.id"></a>**`id`** ([trx.catalog.samples](CATALOG.md#catalog.samples)). Sample to play. To reach a sample by the level's own slot, play it through a handle: `trx.sound.samples[slot]:play()`.
  - <a id="sound.play.opts" name="sound.play.opts"></a>**`opts`** (table, optional). How to play it.

    Keys:
    - <a id="sound.play.opts.pos" name="sound.play.opts.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3), optional). A world position to play from, which applies pan and volume. Omit to play at full volume.

  Returns: [trx.sound.Stream](#sound.Stream) or `nil`. The voice it started, or `nil` if none did.

  Example:
  ```lua
  trx.sound.play(trx.catalog.samples.LARA_NO)
  trx.sound.play(trx.catalog.samples.LARA_NO, { pos = { x = 100, y = 200, z = 50 } })
  ```

- <a id="sound.stop" name="sound.stop"></a>[lua]`trx.sound.stop(id)`  
  Stops a sound effect by catalog id.

  Parameters:
  - <a id="sound.stop.id" name="sound.stop.id"></a>**`id`** ([trx.catalog.samples](CATALOG.md#catalog.samples)). Sample to stop. To reach a sample by the level's own slot, stop it through a handle: `trx.sound.samples[slot]:stop()`.

- <a id="sound.stop_all" name="sound.stop_all"></a>[lua]`trx.sound.stop_all()`  
  Stops every sound effect currently playing.
