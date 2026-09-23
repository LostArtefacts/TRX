-- The BFG9000 is defined entirely by this script.
--
-- The script defines the weapon, pickup, ammunition, and projectile before the
-- game loads a level or save.
--
-- The models come from bfg.bin. The script refers to them by name, so it
-- does not depend on object slots. The same file works for every game.

local OBJECTS = trx.catalog.Context.OBJECTS

local WEAPON_KEY = "dash:bfg9000"
local GLOW_TINT = { r = 48, g = 255, b = 96 }
local BALL_KEY = "dash:bfg_ball"
local HELD_KEY = "dash:bfg_held"
local GUN_KEY = "dash:bfg_item"
local AMMO_KEY = "dash:bfg_ammo_item"

local FIRE_KEY = "dash:bfg_fire"
local ARRIVE_KEY = "dash:bfg_arrive"

local MAX_SPEED = 400
local SPLASH_SPEED_MIN = 16
local SPLASH_SPEED_SPAN = 0x1F
local SPLASH_COUNT_MIN = 2
local SPLASH_COUNT_SPAN = 3
local FALLOFF = { [0] = 13, 7, 7, 7, 7 }
local HIT_RANGE = 512
local BLAST_RADIUS = 1024
local BLAST_DAMAGE = 200
local TRACER_RADIUS = 3072
local MAX_LIFE = 300
local FUN_AMMO = 999

local GREEN_FLAME = 254

local SUCCESS_LINES = {
  "console/cmd/bignasty/success_1",
  "console/cmd/bignasty/success_2",
  "console/cmd/bignasty/success_3",
}

local CARRY_LIGHT_RADIUS = 768

local floor = math.floor
local Spark = trx.fx.SparkType
local Draw = trx.fx.DrawType

local fire_sample = trx.catalog.mint(trx.catalog.Context.SAMPLES, FIRE_KEY)
local arrive_sample = trx.catalog.mint(trx.catalog.Context.SAMPLES, ARRIVE_KEY)

local ball = trx.catalog.mint(OBJECTS, BALL_KEY)
trx.catalog.mint(OBJECTS, HELD_KEY)
local gun_item = trx.catalog.mint(OBJECTS, GUN_KEY)
local ammo_item = trx.catalog.mint(OBJECTS, AMMO_KEY)

local state = {}

local pending_splash = 0

local incoming = nil

local weapon_id

local landed = nil

local function shown(n)
  return trx.random.draw:randrange(n)
end

local function ball_state(item)
  local own = state[item.num]
  if own == nil then
    own = { splash = 0, speed = 0, fall_speed = 0, age = 0 }
    state[item.num] = own
  end
  return own
end

local function inside(pos)
  return trx.rooms.floor_height(pos) ~= nil and pos or nil
end

local function drop(item)
  local num = item.num
  item:destroy()
  state[num] = nil
end

local function flame(pos, vel, size, scalar)
  trx.fx.sparks.fire_flame({ pos = pos, variant = GREEN_FLAME })
  if shown(2) == 1 then
    trx.fx.sparks.fire_smoke({
      pos = {
        x = pos.x + shown(64) - 32,
        y = pos.y + shown(64) - 32,
        z = pos.z + shown(64) - 32,
      },
      variant = GREEN_FLAME,
    })
  end
end

local function nearest_hostile(pos, radius)
  local best, best_dist = nil, radius
  for _, target in
    ipairs(trx.items.query:in_sphere(pos, radius):in_play():matches())
  do
    if target.is_alive and target.is_hostile then
      local dist = target:distance_to(pos)
      if dist < best_dist then
        best, best_dist = target, dist
      end
    end
  end
  return best
end

local function homing_target(pos, radius)
  local aimed = trx.lara.target
  if aimed ~= nil and aimed.is_alive and aimed.is_hostile then
    return aimed
  end
  return nearest_hostile(pos, radius)
end

local function burn_green(target)
  for _ = 1, 12 do
    trx.fx.sparks.fire_flame({
      pos = {
        x = floor(target.pos.x + shown(128) - 64),
        y = floor(target.pos.y - shown(512)),
        z = floor(target.pos.z + shown(128) - 64),
      },
      variant = GREEN_FLAME,
    })
  end
end

local function throw(pos, room_num, yaw, pitch, splash)
  -- A ball that burst against a wall sits in the wall, and nothing may be
  -- spawned there, so the burst is nudged back into the room first.
  local valid = trx.rooms.find_valid_pos(pos, room_num) or pos
  -- A spawn asks which room holds the point and refuses one no room does, so
  -- the same question is asked here rather than letting it raise.
  if trx.rooms.floor_height(valid) == nil then
    return nil
  end
  pending_splash = splash
  local item = trx.items.spawn(ball, valid, yaw, { activate = true })
  pending_splash = 0
  if item == nil then
    return nil
  end
  item.rot = { x = pitch, y = yaw, z = 0 }
  local own = ball_state(item)
  own.splash = splash
  own.speed = trx.random.randrange(SPLASH_SPEED_SPAN + 1) + SPLASH_SPEED_MIN
  own.fall_speed = -16 * splash
  return item
end

local function burn_blast(pos, ring)
  local reach = (BLAST_RADIUS * ring) // 5
  for _ = 1, 12 do
    local a = trx.random.draw:angle()
    local at = inside({
      x = floor(pos.x + reach * trx.math.sin(a)),
      y = floor(pos.y + shown(reach) - reach // 2),
      z = floor(pos.z + reach * trx.math.cos(a)),
    })
    if at ~= nil then
      trx.fx.sparks.fire_flame({ pos = at, variant = GREEN_FLAME })
      if shown(2) == 1 then
        trx.fx.sparks.fire_smoke({ pos = at, variant = GREEN_FLAME })
      end
    end
  end
end

local function burst(item, own, from)
  -- A ball that reaches a wall stops with its centre in the wall, and the
  -- blast needs a point a room holds, so it is nudged back into the room.
  local centre = inside(item.pos)
    or inside(from)
    or trx.rooms.find_valid_pos(item.pos, item.room_num)
  if centre == nil then
    drop(item)
    return
  end
  local count = trx.random.randrange(SPLASH_COUNT_SPAN + 1) + SPLASH_COUNT_MIN
  for _ = 1, count do
    throw(
      from,
      item.room_num,
      item.rot.y + trx.random.randrange(0x4000) + 0x6000,
      0,
      own.splash + 1
    )
  end
  for _ = 1, 48 do
    trx.fx.sparks.fire_flame({
      pos = {
        x = floor(centre.x + shown(BLAST_RADIUS) - BLAST_RADIUS // 2),
        y = floor(centre.y + shown(BLAST_RADIUS) - BLAST_RADIUS // 2),
        z = floor(centre.z + shown(BLAST_RADIUS) - BLAST_RADIUS // 2),
      },
      variant = GREEN_FLAME,
    })
  end
  for ring = 1, 4 do
    burn_blast(centre, ring)
  end
  trx.fx.sparks.explosion_smoke({ pos = centre })
  -- The rings lean at random, so the blast throws a tangle rather than a
  -- stack of level rings.
  trx.fx.knockback({ pos = centre, tilt = 4096 })
  trx.sound.play(trx.catalog.samples.explosion_1, { pos = centre })
  trx.sound.play(trx.catalog.samples.explosion_2, { pos = centre })
  local lara = trx.lara.item
  for _, target in
    ipairs(trx.items.query:in_sphere(centre, BLAST_RADIUS):in_play():matches())
  do
    if target.is_alive and target.num ~= item.num then
      burn_green(target)
      if target.hit_points <= BLAST_DAMAGE then
        target:die({
          explode = true,
          gibs = { flame = true, smoke = true },
          flame_variant = GREEN_FLAME,
          sender = lara,
        })
      else
        target:take_damage(BLAST_DAMAGE, lara)
      end
    end
  end
  drop(item)
end

local function land(item, from)
  local at = { x = from.x, y = from.y, z = from.z }
  local ground = trx.rooms.floor_height(at)
  if ground ~= nil then
    at.y = ground
  end
  local gun = trx.items.spawn(gun_item, at, item.rot.y)
  if gun == nil then
    -- Nowhere to set it down, so it goes straight into the backpack rather
    -- than being lost.
    trx.inventory:give(gun_item)
    trx.inventory:set_shots(trx.catalog.weapons[WEAPON_KEY], FUN_AMMO)
    trx.console.log(trx.locale.get("console/cmd/bignasty/got"))
  end
  landed = gun
  for _ = 1, 16 do
    local spot = inside({
      x = at.x + shown(512) - 256,
      y = at.y - shown(256),
      z = at.z + shown(512) - 256,
    })
    if spot ~= nil then
      trx.fx.sparks.fire_flame({ pos = spot, variant = GREEN_FLAME })
    end
  end
  incoming = nil
  drop(item)
end

local function tracer(item, target)
  local steps = 8
  for i = 1, steps do
    local t = i / steps
    local at = inside({
      x = floor(item.pos.x + (target.pos.x - item.pos.x) * t),
      y = floor(item.pos.y + (target.pos.y - 256 - item.pos.y) * t),
      z = floor(item.pos.z + (target.pos.z - item.pos.z) * t),
    })
    if at ~= nil then
      trx.fx.sparks.spawn({
        pos = at,
        sprite_type = Spark.PARTICLE,
        draw_type = Draw.BLEND_ADD,
        color = trx.math.color(48, 255, 128),
        end_color = trx.math.color(32, 192, 96),
        life = 6,
        width = 8,
        height = 8,
      })
    end
  end
end

local function control(item)
  local own = ball_state(item)
  own.age = own.age + 1
  local from = { x = item.pos.x, y = item.pos.y, z = item.pos.z }
  local beat = own.age * 4
  local chasing = nil

  if own.splash ~= 0 then
    if own.delivery then
      -- A delivery drifts down rather than dropping, so the descent reads
      -- as one.
      own.fall_speed = math.min(own.fall_speed + 1, 16)
    else
      own.fall_speed = own.fall_speed + (own.splash ~= 1 and 1 or 0) + 1
    end
    if beat & 0xC == 0 then
      if own.speed ~= 0 then
        own.speed = own.speed - 1
      end
      flame(
        from,
        { x = 0, y = -shown(0x20), z = 0 },
        nil,
        own.splash <= 2 and 2 or 4
      )
    end
  else
    local target = homing_target(from, TRACER_RADIUS)
    chasing = target
    local target_num = target ~= nil and target.num or nil
    if own.target_num ~= target_num then
      own.target_num = target_num
      own.closest = nil
    end
    if target ~= nil then
      local dx = target.pos.x - from.x
      local dy = target.pos.y - 256 - from.y
      local dz = target.pos.z - from.z
      item.rot = {
        x = trx.math.atan(floor(math.sqrt(dx * dx + dz * dz)), -dy),
        y = trx.math.atan(dz, dx),
        z = 0,
      }
    end
    if own.speed < MAX_SPEED then
      own.speed = own.speed + (own.speed >> 4) + 4
    end
    if beat & 4 ~= 0 then
      flame(from, { x = 0, y = 0, z = 0 }, own.speed >> 1, 3)
    end
  end

  local forward = own.speed * trx.math.cos(item.rot.x)
  local pos = {
    x = floor(from.x + forward * trx.math.sin(item.rot.y)),
    y = floor(from.y - own.speed * trx.math.sin(item.rot.x) + own.fall_speed),
    z = floor(from.z + forward * trx.math.cos(item.rot.y)),
  }

  local room = item.room
  local floor = room:floor_height(pos)
  local ceiling = room:ceiling_height(pos)
  if
    floor == nil
    or ceiling == nil
    or pos.y >= floor
    or pos.y < ceiling
    or own.age > MAX_LIFE
  then
    if own.delivery then
      land(item, from)
    elseif own.splash == 0 then
      burst(item, own, from)
    else
      drop(item)
    end
    return
  end

  item.pos = pos

  if own.splash == 0 then
    local near = nearest_hostile(pos, HIT_RANGE)
    if near == nil and chasing ~= nil then
      -- A target the ball is turning away from has already been reached.
      local dist = chasing:distance_to(pos)
      if own.closest ~= nil and dist > own.closest then
        near = chasing
      end
      own.closest = own.closest == nil and dist or math.min(own.closest, dist)
    end
    if near ~= nil then
      burst(item, own, from)
      return
    end
    for _, target in
      ipairs(trx.items.query:in_sphere(pos, TRACER_RADIUS):in_play():matches())
    do
      if target.is_alive and target.is_hostile and target.num ~= item.num then
        -- The rays reach for what the ball is about to take, and no more:
        -- the blast is what kills, so nothing dies before the ball lands.
        tracer(item, target)
      end
    end
  end

  local falloff = FALLOFF[own.splash]
  if falloff ~= nil and falloff ~= 0 then
    trx.fx.emit_light({
      pos = pos,
      radius = falloff * 256,
      color = trx.math.color(
        shown(0x40),
        255 - shown(0x20),
        192 - shown(0x20)
      ),
    })
  end
end

local function deliver(lara)
  local yaw = lara.rot.y
  local spots = {
    {
      x = floor(lara.pos.x + 768 * trx.math.sin(yaw)),
      z = floor(lara.pos.z + 768 * trx.math.cos(yaw)),
    },
    { x = floor(lara.pos.x), z = floor(lara.pos.z) },
  }
  for _, spot in ipairs(spots) do
    local probe = { x = spot.x, y = floor(lara.pos.y) - 256, z = spot.z }
    local ceiling = trx.rooms.ceiling_height(probe, lara.room_num)
    if ceiling ~= nil then
      local item = throw({
        x = spot.x,
        y = math.max(ceiling + 256, floor(lara.pos.y) - 1024),
        z = spot.z,
      }, lara.room_num, yaw, 0, 1)
      if item ~= nil then
        local own = ball_state(item)
        own.delivery = true
        own.speed = 8
        own.fall_speed = 0
        incoming = item
        return true
      end
    end
  end
  return false
end

local function claimed()
  if incoming ~= nil and incoming:is_valid() then
    return true
  end
  incoming = nil
  return trx.inventory:has_weapon(weapon_id)
    or #trx.items.query:of_object(gun_item):present():matches() > 0
end

local function punish(lara)
  for _ = 1, 12 do
    local at = inside({
      x = lara.pos.x + shown(256) - 128,
      y = lara.pos.y - shown(768),
      z = lara.pos.z + shown(256) - 128,
    })
    trx.fx.blood({
      pos = at or lara.pos,
      angle = trx.random.draw:angle() - 0x8000,
      strength = 64 + shown(128),
    })
  end
  lara:take_damage(lara.hit_points // 2)
  trx.sound.play(trx.catalog.samples.lara_injury, { pos = lara.pos })
end

local GunState = trx.lara.GunState

local function in_hand()
  local status = trx.lara.gun_status
  return trx.lara.equipped_gun == weapon_id
    and (
      status == GunState.READY
      or status == GunState.DRAW
      or status == GunState.UNDRAW
    )
end

local function is_gun_shown()
  return in_hand() or trx.lara.back_gun == weapon_id
end

trx.objects.declare(ball, {
  radius = 128,
  shadow_size = 0,
  save_position = true,
  initialise = function(item)
    item.gravity = false
    -- The launching routine plays no sound of the weapon's own, so the ball
    -- reports as it leaves: it is made at the moment of the shot. What a
    -- burst throws is not a shot and stays quiet.
    if pending_splash == 0 then
      trx.sound.play(fire_sample, { pos = item.pos })
    end
  end,
  control = control,
})

if trx.weapons.get("mp5") == nil then
  return
end

trx.inject.declare(function()
  return { "bfg.bin" }
end)

trx.locale.declare({
  ["objects/dash:bfg_item/name"] = "BFG9000|bfg|bfg9000",
  ["objects/dash:bfg_ammo_item/name"] = "Plasma Cells|plasma_cells|bfg_ammo",
  ["objects/dash:bfg_ball/name"] = "Plasma Ball|plasma_ball",
})

trx.objects[gun_item]:add_family("pickup")
trx.objects[gun_item]:add_family("gun")
trx.objects[gun_item]:add_family("inventory")
trx.objects[ammo_item]:add_family("inventory")
trx.objects[ammo_item]:add_family("pickup")
trx.objects[ammo_item]:add_family("ammo")
trx.objects[ball]:add_family("projectile")

trx.objects[gun_item]:link("gun_to_ammo", ammo_item)

for i, key in ipairs({ GUN_KEY, AMMO_KEY }) do
  trx.inventory.declare_ring_item({
    object_id = key,
    frames_total = 1,
    anim_direction = 1,
    anim_speed = 1,
    meshes_sel = 1,
    meshes_drawn = 1,
    inv_pos = 19 + i,
  })
end

trx.events.on_game_start(function()
  incoming = nil
  landed = nil
  state = {}
end)

trx.weapons.declare(WEAPON_KEY, {
  base = "mp5",
  kind = "rifle",
  is_launcher = true,
  is_machine_gun = false,
  fire_delay = 20,
  fire = "grenade",
  damage = 250,
  is_available = true,
  objects = {
    pickup = GUN_KEY,
    ammo = AMMO_KEY,
    anim = "lara_mp5",
    projectile = BALL_KEY,
  },
  meshes = {
    object = HELD_KEY,
    hand_r = 0,
    torso = 1,
  },
  ammo = {
    initial_shots = 5,
    box_shots = 5,
    box_label_qty = 1,
  },
  sound = {
    fire = FIRE_KEY,
  },
  stow = {
    place = "back",
    order = 7,
  },
  save = {
    ammo_key = "dash_bfg9000",
    resume_has_key = "has_dash_bfg9000",
    resume_ammo_key = "dash_bfg9000_ammo",
  },
  cheat = {
    ammo = 50,
    key_ammo = 500,
  },
})

weapon_id = trx.catalog.weapons[WEAPON_KEY]

trx.events.after_control(function()
  local lara = trx.lara.item
  if lara == nil then
    return
  end
  if landed ~= nil then
    if not landed:is_valid() or not landed.is_visible then
      -- The gun only leaves the world by being taken, which is the moment
      -- it says its name.
      landed = nil
      if trx.inventory:has_weapon(weapon_id) then
        trx.inventory:set_shots(weapon_id, FUN_AMMO)
        trx.console.log(trx.locale.get("console/cmd/bignasty/got"))
      end
    else
      trx.fx.emit_light({
        pos = { x = landed.pos.x, y = landed.pos.y - 128, z = landed.pos.z },
        radius = 2048,
        color = trx.math.color(GLOW_TINT.r, GLOW_TINT.g, GLOW_TINT.b),
      })
    end
  end
  if not is_gun_shown() then
    return
  end
  trx.fx.emit_light({
    pos = { x = lara.pos.x, y = lara.pos.y - 384, z = lara.pos.z },
    radius = CARRY_LIGHT_RADIUS,
    color = trx.math.color(GLOW_TINT.r, GLOW_TINT.g, GLOW_TINT.b),
  })
end)

trx.locale.declare({
  ["console/cmd/bignasty/help"] = "Calls in a special delivery.",
  ["console/cmd/bignasty/success_1"] = "Special delivery!",
  ["console/cmd/bignasty/success_2"] = "Look up!",
  ["console/cmd/bignasty/success_3"] = "The cells hum…",
  ["console/cmd/bignasty/got"] = "You got the BFG9000.",
  ["console/cmd/bignasty/greedy"] = "You already have it.",
  ["console/cmd/bignasty/failure"] = "There is no room for it in this level.",
})

trx.console.register({
  name = "bignasty",
  help = "console/cmd/bignasty/help",
  run = function()
    local lara = trx.lara.item
    if lara == nil or not trx.inventory:can_add(gun_item) then
      return trx.console.Result.FAILURE,
        trx.locale.get("console/cmd/bignasty/failure")
    end
    if claimed() then
      punish(lara)
      return trx.console.Result.FAILURE,
        trx.locale.get("console/cmd/bignasty/greedy")
    end
    if not deliver(lara) then
      -- Nowhere above her to throw from, so the gun goes straight into the
      -- backpack rather than the cheat failing, and that is its taking.
      trx.inventory:give(gun_item)
      trx.inventory:set_shots(weapon_id, FUN_AMMO)
      trx.console.log(trx.locale.get("console/cmd/bignasty/got"))
    end
    trx.sound.play(arrive_sample, { pos = lara.pos })
    return trx.console.Result.SUCCESS,
      trx.locale.get(SUCCESS_LINES[shown(#SUCCESS_LINES) + 1])
  end,
})
