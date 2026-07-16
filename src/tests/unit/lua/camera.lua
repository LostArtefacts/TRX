-- The camera API as a script actually sees it.
--
-- This is the first surface driven entirely by api.property: every member below
-- is a metatable getter, not a field, so reading one calls into C.

local h = require("harness")
local test, raises = h.test, h.raises

test("position reads through to the engine", function()
  assert(trx.camera.pos.x == 1024)
  assert(trx.camera.pos.y == 2048)
  assert(trx.camera.pos.z == 3072)

  assert(trx.camera.target_pos.x == 4096)
  assert(trx.camera.target_pos.y == 5120)
  assert(trx.camera.target_pos.z == 6144)
end)

test("rooms are 1-based, as everywhere else in the API", function()
  -- The engine counts rooms from 0. A script never sees that.
  assert(trx.camera.room_num == fake.CAMERA_ROOM)
  assert(trx.camera.target_room_num == fake.CAMERA_TARGET_ROOM)
end)

test("room hands back a Room handle, not a number", function()
  local room = trx.camera.room
  assert(room ~= nil, "no room")
  assert(room.num == fake.CAMERA_ROOM, "wrong room")
  -- It is the real thing: a handle out of trx.rooms, with the room's own
  -- fields.
  assert(room.num == trx.rooms[fake.CAMERA_ROOM].num)
end)

test("a camera nowhere reports nil, not room zero", function()
  fake.set_no_room()
  assert(trx.camera.room_num == nil, "NO_ROOM must not leak as a number")
  assert(trx.camera.room == nil, "NO_ROOM must not resolve to a Room")
  assert(trx.camera.target_room_num == nil)
end)

test("every property is read-only", function()
  -- None of these has a setter, so the registry must refuse the write rather
  -- than quietly adding a field to the module table.
  raises(function()
    trx.camera.pos = { x = 0, y = 0, z = 0 }
  end, "read-only")
  raises(function()
    trx.camera.room_num = 1
  end, "read-only")
  raises(function()
    trx.camera.room = nil
  end, "read-only")

  -- And the read still works afterwards.
  assert(trx.camera.pos.x == 1024)
end)

test("an undeclared field cannot be written onto the module", function()
  raises(function()
    trx.camera.nonsense = 1
  end)
  assert(trx.camera.nonsense == nil)
end)

test("shake sets the camera bounce", function()
  trx.camera.shake(200)
  assert(fake.calls().bounce == 200, "shake did not reach the camera")

  trx.camera.shake(-50)
  assert(fake.calls().bounce == -50, "a negative shake must pass through")
end)

test("reset asks the engine to reset", function()
  assert(fake.calls().reset == 0)
  trx.camera.reset()
  assert(fake.calls().reset == 1, "reset did not reach the engine")
end)

return h.report()
