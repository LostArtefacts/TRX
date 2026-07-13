local raw = trxc.camera
local api = trx.api

api.module("camera", {
  order = 16,
  description = "Module for inspecting the active camera state.",
})

api.property("camera.pos", {
  type = "vec3",
  description = "Current camera position.",
  get = raw.get_pos,
})

api.property("camera.room_num", {
  type = "integer",
  description = "1-based number of the room the camera is in, or `nil` if unknown.",
  get = raw.get_room,
})

api.property("camera.room", {
  type = "Room",
  description = "The `trx.rooms.Room` the camera is in, or `nil` if unknown.",
  get = function()
    local room_num = raw.get_room()
    return room_num and trx.rooms[room_num] or nil
  end,
})

api.property("camera.target_pos", {
  type = "vec3",
  description = "Position the camera is looking at.",
  get = raw.get_target_pos,
})

api.property("camera.target_room_num", {
  type = "integer",
  description = "1-based number of the room the camera is looking at, or `nil` if unknown.",
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
