---
title: Camera
order: 12
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/camera.lua. Edit it there.
-->

## Camera module

Module for inspecting the active camera state.

### Properties

- **`trx.camera.pos`** (vec3). Current camera position. *(read-only)*
- **`trx.camera.room_num`** (integer). The room the camera is in, or `nil` if unknown. Counted from 0. *(read-only)*
- **`trx.camera.room`** (Room). The `trx.rooms.Room` the camera is in, or `nil` if unknown. *(read-only)*
- **`trx.camera.target_pos`** (vec3). Position the camera is looking at. *(read-only)*
- **`trx.camera.target_room_num`** (integer). The room the camera is looking at, or `nil` if unknown. Counted from 0. *(read-only)*
- **`trx.camera.is_flyby_active`** (boolean). Whether a flyby camera sequence is playing. *(read-only)*

### Functions

- [lua]`trx.camera.shake(intensity)`  
  Shakes the camera by setting its bounce value. Positive values shake it upward, negative values downward.

  Parameters:
  - **`intensity`** (integer). Bounce value.

  Example:
  ```lua
  trx.camera.shake(200)
  ```

- [lua]`trx.camera.reset()`  
  Resets the camera to Lara's current position.

- [lua]`trx.camera.play_flyby(sequence_num)`  
  Starts a flyby camera sequence. Does nothing if another one is already playing.

  Parameters:
  - **`sequence_num`** (integer). Flyby sequence number, as the level numbers them. Counted from 0.

  Returns: boolean. Whether the sequence took the camera.

  Example:
  ```lua
  trx.camera.play_flyby(1)
  ```

- [lua]`trx.camera.cancel_flyby()`  
  Cancels the flyby camera sequence, if one is playing.
