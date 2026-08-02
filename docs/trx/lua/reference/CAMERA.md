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

## <a id="camera" name="camera"></a>Camera module

Module for inspecting the active camera state.

### Properties

- <a id="camera.pos" name="camera.pos"></a>**`trx.camera.pos`** (vec3). Current camera position. *(read-only)*
- <a id="camera.room_num" name="camera.room_num"></a>**`trx.camera.room_num`** ([trx.rooms.Num](ROOMS.md#rooms.Num)). The room the camera is in, or `nil` if unknown. *(read-only)*
- <a id="camera.room" name="camera.room"></a>**`trx.camera.room`** ([trx.rooms.Room](ROOMS.md#rooms.Room)). The room the camera is in, or `nil` if unknown. *(read-only)*
- <a id="camera.target_pos" name="camera.target_pos"></a>**`trx.camera.target_pos`** (vec3). Position the camera is looking at. *(read-only)*
- <a id="camera.target_room_num" name="camera.target_room_num"></a>**`trx.camera.target_room_num`** ([trx.rooms.Num](ROOMS.md#rooms.Num)). The room the camera is looking at, or `nil` if unknown. *(read-only)*
- <a id="camera.is_flyby_active" name="camera.is_flyby_active"></a>**`trx.camera.is_flyby_active`** (boolean). Whether a flyby camera sequence is playing. *(read-only)*

### Structures

- <a id="camera.SequenceNum" name="camera.SequenceNum"></a>[lua]`trx.camera.SequenceNum`

    Flyby sequence number, as the level numbers them. Counted from 0.

### Functions

- <a id="camera.shake" name="camera.shake"></a>[lua]`trx.camera.shake(intensity)`  
  Shakes the camera by setting its bounce value. Positive values shake it upward, negative values downward.

  Parameters:
  - **`intensity`** (integer). Bounce value.

  Example:
  ```lua
  trx.camera.shake(200)
  ```

- <a id="camera.reset" name="camera.reset"></a>[lua]`trx.camera.reset()`  
  Resets the camera to Lara's current position.

- <a id="camera.play_flyby" name="camera.play_flyby"></a>[lua]`trx.camera.play_flyby(sequence_num)`  
  Starts a flyby camera sequence. Does nothing if another one is already playing.

  Parameters:
  - **`sequence_num`** ([trx.camera.SequenceNum](#camera.SequenceNum)).

  Returns: boolean. Whether the sequence took the camera.

  Example:
  ```lua
  trx.camera.play_flyby(1)
  ```

- <a id="camera.cancel_flyby" name="camera.cancel_flyby"></a>[lua]`trx.camera.cancel_flyby()`  
  Cancels the flyby camera sequence, if one is playing.
