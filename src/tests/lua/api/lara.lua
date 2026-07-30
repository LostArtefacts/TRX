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
    trx.lara.water_status = trx.lara.WaterState.UNDERWATER
  end)
  assert(
    trx.lara.water_status ~= trx.lara.WaterState.UNDERWATER,
    "the write must not have landed"
  )
end)

test("is_burning lights Lara and puts her out", function()
  trx.lara.is_burning = true
  assert(fake.calls().catch_fire.count == 1, "did not reach Lara_CatchFire")
  assert(trx.lara.is_burning == true, "she should be on fire")

  trx.lara.is_burning = false
  assert(fake.calls().extinguish.count == 1, "did not reach Lara_Extinguish")
  assert(trx.lara.is_burning == false, "she should be out")
end)

test("is_flying enters and leaves the fly cheat", function()
  assert(trx.lara.is_flying == false)

  trx.lara.is_flying = true
  assert(trx.lara.is_flying == true, "she should be flying")
  assert(trx.lara.water_status == trx.lara.WaterState.CHEAT)

  trx.lara.is_flying = false
  assert(trx.lara.is_flying == false, "she should have landed")
end)

test("teleport moves Lara, and says when it could not", function()
  assert(trx.lara.teleport({ x = 2048, y = 0, z = 1024 }, 2) == true)
  assert(trx.lara.item.pos.x == 2048, "she was left where she was")
  assert(fake.calls().teleport.room_num == 2)

  -- Without a room, the engine finds one from the position.
  assert(trx.lara.teleport({ x = 1024, y = 0, z = 0 }) == true)
  assert(fake.calls().teleport.room_num == -1)

  -- Nowhere to stand: she stays put.
  assert(trx.lara.teleport({ x = -1, y = 0, z = 0 }) == false)
  assert(trx.lara.item.pos.x == 1024, "she moved somewhere with no floor")

  raises(function()
    trx.lara.teleport({ x = 0, y = 0, z = 0 }, 99)
  end, "unknown room")
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
  -- She is item 0 to the engine and to a script alike, like every other item.
  assert(trx.lara.item ~= nil, "Lara has no item")
end)

test("equipment names the meshes it was given", function()
  trx.lara.set_extra_equipment(trx.lara.Mesh.HAND_R, trx.lara.ExtraMesh.OAR)
  local calls = fake.calls()
  assert(
    calls.set_equipment.count == 1,
    "the equipment did not reach the engine"
  )
  assert(calls.set_equipment.mesh == trx.lara.Mesh.HAND_R)
  assert(calls.set_equipment.extra_mesh == trx.lara.ExtraMesh.OAR)

  trx.lara.clear_equipment(trx.lara.Mesh.HAND_R)
  assert(fake.calls().clear_equipment.count == 1)
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
    fake.calls().cure_poison.count == 1,
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
    assert(fake.calls().extinguish.count == 1)
  end
)

test("dry clears the wetness that is_wet reads", function()
  assert(trx.lara.is_wet == true)
  trx.lara.dry()

  assert(trx.lara.is_wet == false)
  assert(fake.calls().dry.count == 1)
end)

-- The backpack. The fake counts what it is given without mapping a pickup to
-- its icon, so these name one id throughout.
local KEY = 42
local UZIS = trx.catalog.weapons.UZIS

test("add puts things in and count reads them back", function()
  assert(trx.lara.inventory.count(KEY) == 0, "she starts with nothing")
  assert(not trx.lara.inventory.has(KEY))

  assert(trx.lara.inventory.add(KEY) == 1, "one went in")
  assert(trx.lara.inventory.add(KEY, 3) == 3)
  assert(trx.lara.inventory.count(KEY) == 4)
  assert(trx.lara.inventory.has(KEY))
end)

test("remove takes them back out and stops when she runs out", function()
  trx.lara.inventory.add(KEY, 2)

  assert(trx.lara.inventory.remove(KEY) == 1)
  assert(trx.lara.inventory.count(KEY) == 1)
  assert(trx.lara.inventory.remove(KEY, 5) == 1, "only what she had came out")
  assert(trx.lara.inventory.count(KEY) == 0)
  assert(trx.lara.inventory.remove(KEY) == 0, "nothing left to take")
end)

test("a count below one is refused, and neither adds nor removes", function()
  for _, count in ipairs({ 0, -1 }) do
    raises(function()
      trx.lara.inventory.add(KEY, count)
    end)
    raises(function()
      trx.lara.inventory.remove(KEY, count)
    end)
  end
  assert(trx.lara.inventory.count(KEY) == 0)
end)

-- Inv_AddItem reaches the object table with whatever it is handed, so the
-- bridge turns an id the engine does not know into a raise of its own.
test("an object the engine does not know is refused", function()
  for _, name in ipairs({ "add", "remove", "count", "entry_of", "can_add" }) do
    raises(function()
      trx.lara.inventory[name](999999)
    end)
  end
end)

test("a level that does not carry the icon takes nothing", function()
  fake.set_can_add(false)

  assert(not trx.lara.inventory.can_add(KEY))
  assert(trx.lara.inventory.add(KEY, 3) == 0, "the add did nothing")
  assert(trx.lara.inventory.count(KEY) == 0)

  fake.set_can_add(true)
  assert(trx.lara.inventory.can_add(KEY))
end)

test("a weapon knows its pickup and its ammo", function()
  assert(trx.lara.weapons.object(UZIS) ~= nil)

  assert(trx.lara.weapons.ammo(UZIS) == 0)
  trx.lara.weapons.set_ammo(UZIS, 2000)
  assert(trx.lara.weapons.ammo(UZIS) == 2000)
end)

test("the level says which weapons it allows", function()
  assert(not trx.lara.weapons.is_available(UZIS), "off unless the level says")
  fake.set_weapon_available(UZIS, true)
  assert(trx.lara.weapons.is_available(UZIS))
end)

test("anything that is not a weapon is refused", function()
  for _, value in ipairs({ 0, -1, 999 }) do
    raises(function()
      trx.lara.weapons.ammo(value)
    end)
  end
  raises(function()
    trx.lara.weapons.set_ammo(UZIS, -1)
  end)
end)

return h.report()
