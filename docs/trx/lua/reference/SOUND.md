---
title: Sound
order: 7
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  data/scripting/sound.lua. Edit it there.
-->

## Sound module

Module for playing sound effects.

### Functions

- [lua]`trx.sound.is_available(id)`  
  Whether a sound sample exists in the current level.

  Parameters:
  - **`id`** (integer). Sample to test. Compare against `trx.catalog.samples`.

  Returns: boolean.

- [lua]`trx.sound.play(id, [opts])`  
  Plays a sound effect. Raises if the sample is not available.

  Parameters:
  - **`id`** (integer). Sample to play. Compare against `trx.catalog.samples`.
  - **`opts`** (table, optional). `pos`: a `{ x =, y =, z = }` world position to play from, which applies pan and volume. Omit to play at full volume.

  Example:
  ```lua
  trx.sound.play(99)
  trx.sound.play(99, { pos = { x = 100, y = 200, z = 50 } })
  ```

- [lua]`trx.sound.stop(id)`  
  Stops a sound effect.

  Parameters:
  - **`id`** (integer). Sample to stop. Compare against `trx.catalog.samples`.

- [lua]`trx.sound.stop_all()`  
  Stops every sound effect currently playing.
