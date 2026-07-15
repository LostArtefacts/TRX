-- The object API as a script actually sees it.
--
-- An object is what an item is cut from. trx.objects[id] is a real handle over
-- the C OBJECT struct, so what is reachable is what objects.Object declares.

local h = require("harness")
local test, raises = h.test, h.raises

test("an object is reached by id", function()
  local wolf = trx.objects[fake.WOLF]
  assert(wolf ~= nil, "no wolf")
  assert(wolf.loaded == true)
  assert(wolf.is_intelligent == true, "a wolf thinks")
  assert(wolf.radius == 341)
  assert(wolf.mesh_count == 6)
end)

test("get() is the same thing as indexing", function()
  assert(trx.objects.get(fake.WOLF).radius == trx.objects[fake.WOLF].radius)
end)

test("an object the level never loaded still has a definition", function()
  local obj = trx.objects[fake.UNLOADED]
  assert(obj ~= nil, "an unloaded object must still be reachable")
  assert(obj.loaded == false, "and it must say so")
end)

test("an object that does not exist is nil", function()
  assert(trx.objects[9999] == nil)
  assert(trx.objects.get(9999) == nil)
end)

test("a vase does not think", function()
  assert(trx.objects[fake.VASE].is_intelligent == false)
end)

test("a writable field writes through", function()
  local wolf = trx.objects[fake.WOLF]
  wolf.radius = 500
  assert(
    trx.objects[fake.WOLF].radius == 500,
    "the write did not reach the object"
  )

  wolf.shadow_size = 64
  assert(trx.objects[fake.WOLF].shadow_size == 64)
end)

test("a field declared read-only cannot be written", function()
  raises(function()
    trx.objects[fake.WOLF].loaded = false
  end)
  raises(function()
    trx.objects[fake.WOLF].is_intelligent = false
  end)
end)

test("a member of OBJECT nobody declared is not reachable", function()
  -- The struct has mesh_idx, anim_idx, frame_base and a control function.
  -- None is declared, so none exists as far as a script is concerned.
  local wolf = trx.objects[fake.WOLF]
  assert(wolf.mesh_idx == nil)
  assert(wolf.anim_idx == nil)
  assert(wolf.control_func == nil)
end)

test(
  "properties are the object's own, and every item inherits them",
  function()
    local wolf = trx.objects[fake.WOLF]
    assert(wolf.properties.max_hit_points == 20, "the object's default")

    wolf.properties.max_hit_points = 30
    assert(
      trx.objects[fake.WOLF].properties.max_hit_points == 30,
      "the default did not move"
    )
  end
)

test("properties iterate", function()
  local seen = {}
  for name, value in pairs(trx.objects[fake.WOLF].properties) do
    seen[name] = value
  end
  assert(seen.max_hit_points == 20, "pairs() did not yield the property")
end)

test("a property nobody declared reads nil and cannot be written", function()
  local wolf = trx.objects[fake.WOLF]
  assert(wolf.properties.nonsense == nil)
  raises(function()
    wolf.properties.nonsense = 1
  end)
end)

test("swap_mesh reaches the engine", function()
  trx.objects.swap_mesh(fake.WOLF, fake.VASE)
  assert(fake.calls().swap_mesh == 1, "the whole-object swap did not land")

  trx.objects.swap_mesh(fake.WOLF, fake.VASE, 1, 2)
  assert(fake.calls().swap_mesh == 2, "the single-mesh swap did not land")
end)

test("swap_mesh will not reach past an object's meshes", function()
  raises(function()
    trx.objects.swap_mesh(fake.WOLF, fake.VASE, 0, 1000000)
  end)
  raises(function()
    trx.objects.swap_mesh(fake.WOLF, fake.VASE, -1, 0)
  end)
  assert(
    fake.calls().swap_mesh == 0,
    "a rejected swap must not reach the engine"
  )
end)

test("swap_mesh takes both mesh numbers or neither", function()
  raises(function()
    trx.objects.swap_mesh(fake.WOLF, fake.VASE, 1)
  end)
end)

test("an object id outside the table is refused", function()
  raises(function()
    trx.objects.swap_mesh(999999, fake.VASE)
  end)
end)

return h.report()
