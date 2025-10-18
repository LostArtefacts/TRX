---
title: Rooms
---

## Rooms module

Module for inspecting all rooms in the current level.

### Structures

- [lua]`trx.rooms.Room`

    Represents a room.

    Properties:
    - **`underwater`**: Whether the room is underwater or not.
    - **`wind`**: Whether the room has breeze enabled or not. (Requires the player to have breeze enabled in the game settings).

    Writable properties:
    - `underwater`
    - `wind`

### Functions

-- Uses Lua length operator on the rooms table:
- [lua]`#trx.rooms`  
  Returns the total number of rooms.

- [lua]`trx.rooms[num]`  
  Retrieves the [lua]`trx.rooms.Room` at the given 1-based index, or `nil` if out of range.

- [lua]`trx.rooms.fn.get(arg)`  
  Alias of `trx.rooms[arg]`.
