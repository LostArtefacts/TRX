---
title: Sound
order: 15
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

- **`trx.sound.samples`** (table). The samples the current level carries, as `trx.sound.Sample` handles keyed by id: `trx.sound.samples[99]` is sample 99, or `nil` if the level has no such sample. `#` counts them, iterating walks them, and both reach one handle at a time. *(read-only)*
- **`trx.sound.streams`** (table). The sound effects playing now, as `trx.sound.Stream` handles. A slot that is silent still answers, with a stale handle. Indexing and iterating reach one handle at a time. *(read-only)*

### Structures

- [lua]`trx.sound.Sample`

    A sound sample the current level carries. Reach them through `trx.sound.samples`. A handle to a sample the loaded level does not carry goes stale, so `is_valid()` answers whether it is still there.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`id`**: integer. The sample's id, in the level's own numbering. *(read-only)*
    - **`pitch`**: integer. The sample's base pitch. *(read-only)*
    - **`randomness`**: integer. How much the sample's playback is randomized. *(read-only)*
    - **`range`**: integer. How far the sample carries. *(read-only)*
    - **`volume`**: integer. The sample's base volume. *(read-only)*

    Methods:

    - [lua]`sample:is_valid()`  
      Whether the loaded level still carries this sample.

      Returns: boolean.

    - [lua]`sample:play([opts])`  
      Plays this sample.

      Parameters:
      - **`opts`** (table, optional). `pos`: a `{ x =, y =, z = }` world position to play from, which applies pan and volume. Omit to play at full volume.

      Returns: Stream or `nil`. The voice it started, or `nil` if none did.

    - [lua]`sample:stop()`  
      Stops every voice playing this sample.

- [lua]`trx.sound.Stream`

    One of the sound effects playing now. Reach them through `trx.sound.streams`. A handle to a voice that has fallen silent goes stale, so check `is_valid()` first.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`sample_id`**: integer. The sample this voice is playing, in the level's own numbering. *(read-only)*

    Methods:

    - [lua]`stream:is_valid()`  
      Whether this voice is still playing.

      Returns: boolean.

    - [lua]`stream:pause()`  
      Pauses this voice.

    - [lua]`stream:stop()`  
      Stops this voice.

    - [lua]`stream:unpause()`  
      Resumes this voice.

### Functions

- [lua]`trx.sound.play(id, [opts])`  
  Plays a sound effect by catalog id, mapping it to the level's own sample. A game that does not carry the sample plays nothing.

  Parameters:
  - **`id`** (integer). Sample to play. To reach a sample by the level's own slot, play it through a handle: `trx.sound.samples[slot]:play()`. Compare against `trx.catalog.samples`.
  - **`opts`** (table, optional). `pos`: a `{ x =, y =, z = }` world position to play from, which applies pan and volume. Omit to play at full volume.

  Returns: Stream or `nil`. The voice it started, or `nil` if none did.

  Example:
  ```lua
  trx.sound.play(trx.catalog.samples.LARA_NO)
  trx.sound.play(trx.catalog.samples.LARA_NO, { pos = { x = 100, y = 200, z = 50 } })
  ```

- [lua]`trx.sound.stop(id)`  
  Stops a sound effect by catalog id.

  Parameters:
  - **`id`** (integer). Sample to stop. To reach a sample by the level's own slot, stop it through a handle: `trx.sound.samples[slot]:stop()`. Compare against `trx.catalog.samples`.

- [lua]`trx.sound.stop_all()`  
  Stops every sound effect currently playing.
