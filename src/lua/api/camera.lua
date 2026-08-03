local raw = trxc.camera
local api = trx.api

api.module("camera", {
  order = 12,
  description = "Module for inspecting the active camera state.",
})

api.property("camera.pos", {
  type = "math.Vec3",
  description = "Current camera position.",
  get = raw.get_pos,
})

api.property("camera.room_num", {
  type = "rooms.Num",
  description = "The room the camera is in, or `nil` if unknown.",
  get = raw.get_room,
})

api.property("camera.room", {
  type = "rooms.Room",
  description = "The room the camera is in, or `nil` if unknown.",
  get = function()
    local room_num = raw.get_room()
    return room_num and trx.rooms[room_num] or nil
  end,
})

api.property("camera.target_pos", {
  type = "math.Vec3",
  description = "Position the camera is looking at.",
  get = raw.get_target_pos,
})

api.property("camera.target_room_num", {
  type = "rooms.Num",
  description = "The room the camera is looking at, or `nil` if unknown.",
  get = raw.get_target_room,
})

api.define("camera.shake", {
  description = "Shakes the camera by setting its bounce value. Positive values shake it upward, "
    .. "negative values downward.",
  params = {
    { name = "intensity", type = "integer", description = "Bounce value." },
  },
  examples = { [[trx.camera.shake(200)]] },
  impl = raw.shake,
})

api.define("camera.reset", {
  description = "Resets the camera to Lara's current position.",
  impl = raw.reset,
})

api.property("camera.is_flyby_active", {
  type = "boolean",
  description = "Whether a flyby camera sequence is playing.",
  get = raw.is_flyby_active,
})

api.number("camera.SequenceNum", {
  base = 0,
  description = "Flyby sequence number, as the level numbers them.",
})

api.define("camera.play_flyby", {
  description = "Starts a flyby camera sequence. Does nothing if another one is already playing.",
  params = {
    {
      name = "sequence_num",
      type = "camera.SequenceNum",
    },
  },
  returns = {
    type = "boolean",
    description = "Whether the sequence took the camera.",
  },
  examples = { [[trx.camera.play_flyby(1)]] },
  impl = raw.play_flyby,
})

api.define("camera.cancel_flyby", {
  description = "Cancels the flyby camera sequence, if one is playing.",
  impl = raw.cancel_flyby,
})
