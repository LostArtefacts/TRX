local raw = trxc.camera
local h = require("trx.internal.helpers")

---@class (partial) trx
---@field camera trx.camera

---Module for inspecting the active camera state.
---@trx.module 13
---@class (exact) trx.camera
---@trx.readonly is_flyby_active, pos, room, room_num, target_pos,
---  target_room_num
---@field pos trx.math.Vec3 Current camera position.
---@field room_num trx.rooms.Num? The room the camera is in, or `nil` if unknown.
---@field room trx.rooms.Room? The room the camera is in, or `nil` if unknown.
---@field target_pos trx.math.Vec3 Position the camera is looking at.
---@field target_room_num trx.rooms.Num? The room the camera is looking at, or `nil` if unknown.
---@field is_flyby_active boolean Whether a flyby camera sequence is playing.
local M = h.module("camera")

h.properties(M, "camera", {
  pos = {
    get = raw.get_pos,
  },
  room_num = {
    get = raw.get_room,
  },
  room = {
    get = function()
      local room_num = raw.get_room()
      return room_num and trx.rooms[room_num] or nil
    end,
  },
  target_pos = {
    get = raw.get_target_pos,
  },
  target_room_num = {
    get = raw.get_target_room,
  },
})

---Shakes the camera by setting its bounce value. Positive values shake it
---upward, negative values downward.
---
---```lua
---trx.camera.shake(200)
---```
---@param intensity integer Bounce value.
---@type fun(intensity: integer)
M.shake = raw.shake

---Resets the camera to Lara's current position.
---@type fun()
M.reset = raw.reset

h.properties(M, "camera", {
  is_flyby_active = {
    get = raw.is_flyby_active,
  },
})

---Flyby sequence number, as the level numbers them.
---@trx.base 0
---@alias trx.camera.SequenceNum integer

---Starts a flyby camera sequence. Does nothing if another one is already
---playing.
---
---```lua
---trx.camera.play_flyby(1)
---```
---@param sequence_num trx.camera.SequenceNum
---@return boolean # Whether the sequence took the camera.
---@type fun(sequence_num: trx.camera.SequenceNum): boolean
M.play_flyby = raw.play_flyby

---Cancels the flyby camera sequence, if one is playing.
---@type fun()
M.cancel_flyby = raw.cancel_flyby
