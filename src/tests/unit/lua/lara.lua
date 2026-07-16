-- The Lara API as a script actually sees it.
--
-- trx.lara is the first module that stands for a C struct: reading
-- trx.lara.air_bar reaches into a real LARA_INFO through the reflection layer.
-- What is reachable is what lara.Lara declares, and nothing else.

local h = require("harness")
local test, raises = h.test, h.raises

test("a field reads straight off Lara", function()
  assert(trx.lara.air_bar == 1800)
  assert(trx.lara.exposure_bar == 600)
  assert(trx.lara.poison == 0)
  assert(trx.lara.is_burning == false)
  assert(
    trx.lara.hit_direction == -1,
    "a negative field must survive as negative"
  )
end)

test("a writable field writes straight through", function()
  trx.lara.air_bar = 900
  assert(trx.lara.air_bar == 900, "the write did not reach Lara")

  trx.lara.poison = 16
  assert(trx.lara.poison == 16)
end)

test("a field declared read-only cannot be written", function()
  raises(function()
    trx.lara.is_burning = true
  end)
  raises(function()
    trx.lara.water_status = trx.lara.WaterState.UNDERWATER
  end)
  assert(trx.lara.is_burning == false, "the write must not have landed")
end)

test("a member of LARA_INFO nobody declared is not reachable", function()
  -- The C struct has turn_rate, move_angle, the LOT and both arms. None of them
  -- is declared, so none of them exists as far as a script is concerned.
  assert(trx.lara.turn_rate == nil)
  assert(trx.lara.move_angle == nil)
  assert(trx.lara.lot == nil)

  raises(function()
    trx.lara.turn_rate = 100
  end)
end)

test("an enum field reads as its enum", function()
  assert(trx.lara.water_status == trx.lara.WaterState.ABOVE_WATER)
  assert(trx.lara.gun_status == trx.lara.GunState.ARMLESS)
end)

test("the enums come from C", function()
  assert(trx.lara.WaterState.ABOVE_WATER == 0)
  assert(trx.lara.WaterState.UNDERWATER == 1)
  assert(trx.lara.WaterState.CHEAT == 3)

  assert(trx.lara.GunState.ARMLESS == 0)
  assert(trx.lara.GunState.READY == 4)

  assert(trx.lara.Mesh.HIPS == 0)
  assert(trx.lara.Mesh.HEAD == 14)
end)

test("properties that are not struct fields still work", function()
  assert(trx.lara.holsters_visible == true)
  trx.lara.holsters_visible = false
  assert(trx.lara.holsters_visible == false)

  assert(trx.lara.outfit == "default")
  assert(trx.lara.has_pistol_weapon == true)
end)

test("lara.item is her Item handle", function()
  -- She is item 0 in C and item 1 to a script, like every other item.
  assert(trx.lara.item ~= nil, "Lara has no item")
end)

test("equipment names the meshes it was given", function()
  trx.lara.set_extra_equipment(trx.lara.Mesh.HAND_R, trx.lara.ExtraMesh.OAR)
  local calls = fake.calls()
  assert(calls.set_equipment == 1, "the equipment did not reach the engine")
  assert(calls.last_mesh == trx.lara.Mesh.HAND_R)
  assert(calls.last_extra_mesh == trx.lara.ExtraMesh.OAR)

  trx.lara.clear_equipment(trx.lara.Mesh.HAND_R)
  assert(fake.calls().clear_equipment == 1)
end)

test("an undeclared name cannot be written onto the module", function()
  raises(function()
    trx.lara.nonsense = 1
  end)
  assert(trx.lara.nonsense == nil)
end)

test("cure_poison clears the poison outright", function()
  trx.lara.poison = 40
  trx.lara.cure_poison()

  assert(trx.lara.poison == 0)
  assert(
    fake.calls().cure_poison == 1,
    "the engine verb must be the one that runs"
  )
end)

test(
  "extinguish puts Lara out, and stops the electrocution with it",
  function()
    trx.lara.electric = 30
    trx.lara.extinguish()

    assert(trx.lara.is_burning == false)
    assert(
      trx.lara.electric == 0,
      "extinguishing must clear the electrocution too"
    )
    assert(fake.calls().extinguish == 1)
  end
)

return h.report()
