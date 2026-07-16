---
title: Camera
order: 16
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
- **`trx.camera.room_num`** (integer). 1-based number of the room the camera is in, or `nil` if unknown. *(read-only)*
- **`trx.camera.room`** (Room). The `trx.rooms.Room` the camera is in, or `nil` if unknown. *(read-only)*
- **`trx.camera.target_pos`** (vec3). Position the camera is looking at. *(read-only)*
- **`trx.camera.target_room_num`** (integer). 1-based number of the room the camera is looking at, or `nil` if unknown. *(read-only)*

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
