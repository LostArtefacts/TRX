-- What a weapon is, as a script reads and writes it. The definitions are the
-- engine's own table, so every write here is a write the game would fire with.

local h = require("harness")
local test, raises = h.test, h.raises

local WEAPONS = trx.catalog.weapons

test("a weapon is reached by name, by id, and by neither", function()
  local uzis = trx.weapons.uzis
  assert(uzis ~= nil, "the catalog name reaches a weapon")
  assert(uzis == trx.weapons[WEAPONS.UZIS], "so does the id")
  assert(uzis == trx.weapons.get(WEAPONS.UZIS))
  assert(uzis ~= trx.weapons.pistols, "and two weapons are not one")

  assert(trx.weapons.nonsense == nil)
  assert(trx.weapons[WEAPONS.UNARMED] == nil, "unarmed is not a weapon")
end)

test("every weapon is listed once, without unarmed", function()
  local all = trx.weapons.all
  assert(#all > 1)

  -- A handle is a fresh value every time, so what is counted is what each one
  -- names: two handles to one weapon are equal, and no weapon appears twice.
  local function count(weapon)
    local found = 0
    for _, listed in ipairs(all) do
      if listed == weapon then
        found = found + 1
      end
    end
    return found
  end
  assert(count(trx.weapons.uzis) == 1)
  assert(count(trx.weapons.shotgun) == 1)
  assert(count(trx.weapons[trx.catalog.weapons.UNARMED]) == 0)
end)

test("the numbers a weapon fires by are writable", function()
  local uzis = trx.weapons.uzis
  uzis.damage = 5
  uzis.target_dist = 12 * trx.math.WALL_L
  uzis.shot_accuracy = 4 * trx.math.DEG_1
  uzis.smoke_count = 20
  uzis.is_available = false

  assert(uzis.damage == 5)
  assert(uzis.target_dist == 12 * trx.math.WALL_L)
  assert(uzis.shot_accuracy == 4 * trx.math.DEG_1)
  assert(uzis.smoke_count == 20)
  assert(uzis.is_available == false)
  assert(
    trx.weapons.is_available(WEAPONS.UZIS) == false,
    "the field and the function are the same weapon"
  )
  uzis.is_available = true
end)

test("the kind is one of the four the engine has", function()
  local uzis = trx.weapons.uzis
  uzis.kind = trx.weapons.Kind.RIFLE
  assert(uzis.kind == trx.weapons.Kind.RIFLE)

  raises(function()
    uzis.kind = 99
  end)
  raises(function()
    uzis.kind = -1
  end)
  assert(
    uzis.kind == trx.weapons.Kind.RIFLE,
    "a refused write changed nothing"
  )
  uzis.kind = trx.weapons.Kind.DUAL_PISTOLS
end)

test("a group is written through, not copied", function()
  local shotgun = trx.weapons.shotgun
  shotgun.ammo.box_shots = 12
  shotgun.ammo.infinite = true
  assert(shotgun.ammo.box_shots == 12, "the write reached the weapon")
  assert(shotgun.ammo.infinite)

  -- Two handles to the same group are the same group; the group of another
  -- weapon is not.
  assert(shotgun.ammo == shotgun.ammo)
  assert(shotgun.ammo ~= trx.weapons.uzis.ammo)
  shotgun.ammo.infinite = false
end)

test("the aim limits are three groups, each its own", function()
  local uzis = trx.weapons.uzis
  uzis.lock.max_yaw = 60 * trx.math.DEG_1
  uzis.left_arm.max_yaw = 80 * trx.math.DEG_1
  uzis.right_arm.min_pitch = -70 * trx.math.DEG_1

  assert(uzis.lock.max_yaw == 60 * trx.math.DEG_1)
  assert(uzis.left_arm.max_yaw == 80 * trx.math.DEG_1)
  assert(uzis.right_arm.min_pitch == -70 * trx.math.DEG_1)
  assert(uzis.lock ~= uzis.left_arm, "the groups are told apart")

  -- An angle counts in cycles, so one past the end of a turn comes back round.
  uzis.lock.min_yaw = -60 * trx.math.DEG_1 + 4 * 0x10000
  assert(uzis.lock.min_yaw == -60 * trx.math.DEG_1)
end)

test("the hand offsets are written per hand", function()
  local uzis = trx.weapons.uzis
  uzis.muzzle_pos.right = { x = -16, y = 128, z = 40 }
  uzis.muzzle_pos.left = { x = 16, y = 128, z = 40 }
  uzis.flash.pos.right = { x = 0, y = 185, z = 40 }

  assert(uzis.muzzle_pos.right.x == -16)
  assert(uzis.muzzle_pos.left.x == 16)
  assert(uzis.flash.pos.right.y == 185)
  assert(uzis.shell_pos.right.x == 0, "a weapon starts with no shell offset")
end)

test("a color is written whole or a channel at a time", function()
  local flare = trx.weapons.flare
  flare.glow.color = "33e5ff"
  assert(flare.glow.color.hex == "33e5ff")
  -- The weapon keeps its colors as fractions, so a channel comes back as close
  -- to the byte as a float carries rather than exactly on it.
  assert(math.abs(flare.glow.color.r - 51) < 0.01)

  -- The channel write goes back to the weapon rather than to a copy.
  flare.glow.color.r = 255
  assert(flare.glow.color.hex == "ffe5ff")

  flare.glow.color = { r = 0, g = 128, b = 192 }
  assert(flare.glow.color.hex == "0080c0")
  flare.glow.color.hex = "ffbf20"
  assert(flare.glow.color.hex == "ffbf20")

  raises(function()
    flare.glow.color = "not a color"
  end)
end)

test("the rest of the glow is writable too", function()
  local flare = trx.weapons.flare
  flare.glow.scale = 2.5
  flare.glow.flicker = true
  flare.glow.pos = { x = 0, y = -32, z = 0 }
  assert(flare.glow.scale == 2.5)
  assert(flare.glow.flicker)
  assert(flare.glow.pos.y == -32)
end)

test("an animation number has to be one the weapon has", function()
  local shotgun = trx.weapons.shotgun
  shotgun.anim.draw_frame = 10
  shotgun.anim.undraw_frame = 20
  shotgun.anim.recoil_frame = 9
  assert(shotgun.anim.draw_frame == 10)
  assert(shotgun.anim.undraw_frame == 20)
  assert(shotgun.anim.recoil_frame == 9)

  raises(function()
    shotgun.anim.equip_anim = -1
  end)
end)

test("the sample a shot plays is a catalog sample", function()
  local uzis = trx.weapons.uzis
  uzis.fire_sample = trx.catalog.samples.LARA_MAGNUMS
  assert(uzis.fire_sample == trx.catalog.samples.LARA_MAGNUMS)

  raises(function()
    uzis.fire_sample = -5
  end)
end)

test("a member nobody declared is not reachable", function()
  local uzis = trx.weapons.uzis
  assert(uzis.sample_num == nil, "the C name is not the API name")
  raises(function()
    uzis.nonsense = 1
  end)
end)

test("a weapon says which weapon it is, and what it is carried as", function()
  local shotgun = trx.weapons.shotgun
  assert(shotgun.id == WEAPONS.SHOTGUN, "the id is what the calls take")
  assert(shotgun.object == trx.weapons.object(WEAPONS.SHOTGUN))
  assert(shotgun.ammo_object == trx.weapons.ammo_object(WEAPONS.SHOTGUN))
  assert(shotgun.rounds_per_shot == 6)
  assert(trx.weapons.uzis.rounds_per_shot == 1)

  raises(function()
    shotgun.id = WEAPONS.UZIS
  end)
end)

test("what a weapon takes and what it is worth", function()
  assert(trx.weapons.object(WEAPONS.SHOTGUN) ~= nil)
  assert(trx.weapons.ammo_object(WEAPONS.SHOTGUN) ~= nil)
  assert(
    trx.weapons.rounds_per_shot(WEAPONS.SHOTGUN) == 6,
    "the shotgun spends six rounds a shot"
  )
  assert(trx.weapons.rounds_per_shot(WEAPONS.UZIS) == 1)
  assert(trx.weapons.shots_per_box(WEAPONS.SHOTGUN) == 10)

  raises(function()
    trx.weapons.object(WEAPONS.UNARMED)
  end)
end)

return h.report()
