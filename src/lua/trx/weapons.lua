local raw = trxc.weapons
local h = require("trx.internal.helpers")

require("trx.math")
require("trx.catalog")

---@class trx
---@field weapons trx.weapons

---What a weapon is, rather than what Lara has of it.
---
---None of this differs between the inventory she carries and the one a level
---keeps for her, so it belongs to neither: what she holds and how many shots
---she has are `trx.inventory`.
---
---A weapon is shared by every copy of it. Changes last for the rest of the
---session, so levels should restore any values they change when they end.
---@trx.module 6 Weapon
---@class (exact) trx.weapons: table<trx.catalog.weapons|string, trx.weapons.Weapon?>
---@trx.readonly all
---@field all trx.weapons.Weapon[] Every weapon the engine knows, in the order it holds them. `UNARMED` is not one of them.
local M = h.module("weapons")

---How the engine holds and fires a weapon, which decides which arm animations
---and firing routine it uses.
---@enum trx.weapons.Kind
local Kind = {
  DUAL_PISTOLS = "One in each hand, each arm aiming and firing on its own.",
  SINGLE_PISTOL = "One in the right hand.",
  RIFLE = "Held in both hands, drawn from Lara's back.",
  MOUNTED = "Fixed to a vehicle rather than held.",
  FLARE = "Held in one hand and burning, rather than fired.",
}
M.Kind = h.enum("weapons.Kind", "WEAPON_TYPE", Kind)

---How far off straight ahead an aim may go, as a pair of limits about each
---axis.
---@class (exact) trx.weapons.AimLimits
---@field min_yaw trx.math.Angle As far to the left as the aim reaches, which is a negative angle.
---@field max_yaw trx.math.Angle As far to the right as it reaches.
---@field min_pitch trx.math.Angle As far up as it reaches, which is a negative angle.
---@field max_pitch trx.math.Angle As far down as it reaches.
local AimLimits = h.handle("weapons.AimLimits", "WEAPON_AIM_LIMITS", {
  fields = {
    min_yaw = "min_yaw",
    max_yaw = "max_yaw",
    min_pitch = "min_pitch",
    max_pitch = "max_pitch",
  },
  writable = { "min_yaw", "max_yaw", "min_pitch", "max_pitch" },
})

---An offset in the frame of the hand that holds the weapon. A weapon held in
---one hand only uses the right.
---@class (exact) trx.weapons.HandPos
---@field right trx.math.Vec3 Offset in the right hand.
---@field left trx.math.Vec3 Offset in the left hand.
local HandPos = h.handle("weapons.HandPos", "WEAPON_HAND_POS", {
  fields = { right = "right", left = "left" },
  writable = { "right", "left" },
})

---What the weapon is fed. A shot is one pull of the trigger, which for the
---shotgun spends six rounds; the flare counts a flare where a weapon counts a
---shot.
---@class (exact) trx.weapons.Ammo
---@field initial_shots integer What the weapon arrives with the first time Lara picks it up.
---@field box_shots integer What one box of ammunition is worth.
---@field box_label_qty integer What a box shows on its inventory icon, which follows nothing else.
---@field infinite boolean Whether firing spends nothing, so the weapon never runs out and carries no counter.
local Ammo = h.handle("weapons.Ammo", "WEAPON_AMMO_INFO", {
  fields = {
    initial_shots = "initial_shots",
    box_shots = "box_shots",
    box_label_qty = "box_label_qty",
    infinite = "infinite",
  },
  writable = { "initial_shots", "box_shots", "box_label_qty", "infinite" },
})

---The muzzle flash a shot draws.
---@class (exact) trx.weapons.Flash
---@field time integer How many frames the flash stays on screen for.
---@field shade integer How brightly it lights the model around it, in TR1 and TR2. Later games light it by `trx.weapons.Flash.color` instead.
---@field color trx.math.Color What color it lights with, in TR3 and later.
---@field pos trx.weapons.HandPos Where the flash is drawn, in each hand.
local Flash = h.handle("weapons.Flash", "WEAPON_FLASH_INFO", {
  fields = { time = "time", shade = "shade", color = "color" },
  writable = { "time", "shade", "color" },
  extensions = {
    pos = function(flash)
      return raw.get_flash_pos(flash)
    end,
  },
})

---The glow sprite drawn where the weapon burns: a gun's muzzle, or a lit
---flare.
---@class (exact) trx.weapons.Glow
---@field color trx.math.Color What color the glow is drawn in.
---@field pos trx.math.Vec3 Where it sits, in the frame of the mesh it follows.
---@field scale number Multiplies the sprite's own size. `0` turns the glow off.
---@field flicker boolean Whether the brightness is randomized every frame, the way a flare burns.
local Glow = h.handle("weapons.Glow", "WEAPON_GLOW_INFO", {
  fields = {
    color = "color",
    pos = "pos",
    scale = "scale",
    flicker = "flicker",
  },
  writable = { "color", "pos", "scale", "flicker" },
})

---The animation numbers a rifle is drawn, put away and fired by. They count
---the animations and frames of the weapon's own object, not Lara's.
---@class (exact) trx.weapons.Anim
---@field equip_anim integer The animation the weapon starts on as Lara reaches for it. One the object does not have raises.
---@field draw_frame integer The frame the weapon appears in her hands on.
---@field undraw_frame integer The frame it leaves her hands on.
---@field recoil_frame integer The frame a pistol kicks on.
local Anim = h.handle("weapons.Anim", "WEAPON_ANIM_INFO", {
  fields = {
    equip_anim = "equip_anim_idx",
    draw_frame = "draw_frame",
    undraw_frame = "undraw_frame",
    recoil_frame = "recoil_frame",
  },
  writable = { "equip_anim", "draw_frame", "undraw_frame", "recoil_frame" },
})

---A weapon definition, reached as `trx.weapons.uzis` or by id.
---@class (exact) trx.weapons.Weapon
---@trx.readonly id
---@field id trx.catalog.weapons Which weapon this is, for the calls that take one: `trx.inventory:set_shots(weapon.id, 100)`.
---@field kind trx.weapons.Kind How the engine holds and fires it.
---@field is_available boolean Whether the game allows the weapon at all. Turning one off keeps it out of the cheats and off the controls list, and a save that carries it arrives without it.
---@field given_in_ngplus boolean Whether a bonus game gives Lara the weapon, loaded, at level start. A weapon added by a script is not given unless this is true.
---@field aim_speed trx.math.Angle How far the arms swing towards the target each frame, in engine units. A spec says the same thing in degrees, as `aim.speed`. <!--noref: aim.speed-->
---@field shot_accuracy trx.math.Angle How wide a cone a shot may stray into, in engine units. `0` never misses. A spec says the same thing in degrees, as `aim.accuracy`. <!--noref: aim.accuracy-->
---@field gun_height trx.math.Distance How far above Lara's feet the shot leaves the barrel. It also decides how deep she can wade and still fire.
---@field damage integer Hit points one shot takes off what it hits.
---@field target_dist trx.math.Distance How far the weapon reaches, both for auto-aim and for the shot itself, in world units. A spec says the same thing in sectors, as `aim.target_dist`, and `trx.math.from_sectors` converts. <!--noref: aim.target_dist-->
---@field smoke_count integer How many puffs of smoke a shot leaves at the muzzle, in TR3. `0` for none.
---@field fire_sample trx.catalog.samples The sample a shot plays. One this game has no sound for is silent.
---@field fire_overlay_sample trx.catalog.samples The overlay sample a shot plays. One this game has no sound for is silent.
---@field fire_overlay_pitch integer The pitch at which to play the overlay sample.
---@field object trx.catalog.objects? The pickup the weapon is, for handing it to `trx.inventory:give`. `nil` where this game has no such weapon.
---@field ammo_object trx.catalog.objects? The box of ammunition it takes, or `nil` where it takes none.
---@field has_infinite_ammo boolean Whether the weapon never runs dry. The pistols do in most games, and a level or a script may say so of any weapon. A count of shots left means nothing in this context.
---@field ammo_icon string? The markup drawn beside the ammunition count in TR1. Later games count without one, and a weapon that carries no icon answers with `nil`.
---@field rounds_per_shot integer How many rounds one pull of the trigger spends: six for the shotgun, one for everything else. What a box is worth in shots is `trx.weapons.Ammo.box_shots`.
---@field lock trx.weapons.AimLimits Where auto-aim may lock on, measured from where Lara faces.
---@field left_arm trx.weapons.AimLimits How far the left arm may follow a target it has locked onto. A dual-wielded weapon drops the lock on the arm that cannot reach.
---@field right_arm trx.weapons.AimLimits How far the right arm may follow a target.
---@field ammo trx.weapons.Ammo What the weapon is fed.
---@field anim trx.weapons.Anim The animation numbers it is drawn and fired by.
---@field flash trx.weapons.Flash The muzzle flash a shot draws.
---@field glow trx.weapons.Glow The glow drawn where it burns.
---@field muzzle_pos trx.weapons.HandPos Where the barrel ends, which is where smoke and sparks come from.
---@field shell_pos trx.weapons.HandPos Where a spent shell is thrown from. A weapon that leaves no shells has this at the origin.
local Weapon = h.handle("weapons.Weapon", "WEAPON_INFO", {
  fields = {
    id = "id",
    kind = "type",
    is_available = "is_available",
    given_in_ngplus = "given_in_ngplus",
    aim_speed = "aim_speed",
    shot_accuracy = "shot_accuracy",
    gun_height = "gun_height",
    damage = "damage",
    target_dist = "target_dist",
    smoke_count = "smoke_count",
    fire_sample = "sample_num",
    fire_overlay_sample = "sample_overlay_num",
    fire_overlay_pitch = "sample_overlay_pitch",
  },
  writable = {
    "kind",
    "is_available",
    "given_in_ngplus",
    "aim_speed",
    "shot_accuracy",
    "gun_height",
    "damage",
    "target_dist",
    "smoke_count",
    "fire_sample",
    "fire_overlay_sample",
    "fire_overlay_pitch",
  },
  extensions = {
    object = function(weapon)
      return raw.get_object(weapon.id)
    end,
    ammo_object = function(weapon)
      return raw.get_ammo_object(weapon.id)
    end,
    has_infinite_ammo = function(weapon)
      return raw.has_infinite_ammo(weapon.id)
    end,
    ammo_icon = function(weapon)
      return raw.ammo_icon(weapon.id)
    end,
    rounds_per_shot = function(weapon)
      return raw.rounds_per_shot(weapon.id)
    end,
    lock = function(weapon)
      return raw.get_lock(weapon)
    end,
    left_arm = function(weapon)
      return raw.get_left_arm(weapon)
    end,
    right_arm = function(weapon)
      return raw.get_right_arm(weapon)
    end,
    ammo = function(weapon)
      return raw.get_ammo(weapon)
    end,
    anim = function(weapon)
      return raw.get_anim(weapon)
    end,
    flash = function(weapon)
      return raw.get_flash(weapon)
    end,
    glow = function(weapon)
      return raw.get_glow(weapon)
    end,
    muzzle_pos = function(weapon)
      return raw.get_muzzle_pos(weapon)
    end,
    shell_pos = function(weapon)
      return raw.get_shell_pos(weapon)
    end,
  },
})

local function weapon_id(key)
  if type(key) == "number" then
    return key
  end
  if type(key) == "string" then
    return trx.catalog.weapons[key]
  end
  return nil
end

---Adds a weapon of a script's own, under a name the game does not hold yet.
---A weapon of a kind the engine implements is held, drawn and put away as the
---weapons of that kind are, and only what it does when it fires is a script's
---to write. Raises where the spec says neither a kind nor a base, and where
---the name is taken, so that two mods claiming one weapon are heard;
---`trx.weapons.patch` changes a weapon that is there already.
---
---```lua
---trx.weapons.declare("mymod:bigger_gun", {
---  base = "shotgun",
---  kind = "rifle",
---  objects = {
---    pickup = "shotgun_item",
---    ammo = "shotgun_ammo_item",
---    anim = "lara_shotgun",
---  },
---  ammo = { initial_shots = 12, box_shots = 12 },
---  damage = 30,
---  fire = function(weapon, running)
---    trx.sound.play(trx.catalog.samples.explosion)
---  end,
---})
---```
---@param weapon trx.catalog.weapons|string Which weapon, by id or by name. A name of its own wants a prefix, so that two mods do not claim one weapon.
---@param spec table Describes the weapon with the same groups as its weapons file entry: `kind`, (`objects`, `meshes`, `ammo`, `aim`, `anim`, `flash`, `glow`, `muzzle`, `smoke`, `shell`, `sound`, `stow`, `save`, `cheat`), and its own numbers beside them. `base` starts the weapon from another one, and `fire` accepts an engine routine name or a function. An omitted key keeps the weapon's current value. An unknown key or an invalid value raises an error and writes nothing.
---
---  `meshes` states where Lara is drawn from while she holds the weapon. It
---  names an `object` and the offsets `hand_r`, `hand_l`, `torso`, `thigh_r`
---  and `thigh_l` into it. An offset of `-1` draws nothing in that place. A
---  weapon with no `meshes` is drawn from the outfit, like every weapon the
---  game ships.
---
---  A spec uses the units of a weapons file: angles in degrees and distances
---  in sectors. Weapon fields use the engine's units, as other API fields do,
---  so a spec that says `aim.speed = 10` reads back as `weapon.aim_speed ==
---  1820`. <!--noref: kind, objects, meshes, ammo, aim, anim, flash, glow,
---  muzzle, smoke, shell, sound, stow, save, cheat, base, fire, object,
---  hand_r, hand_l, torso, thigh_r, thigh_l-->
---@return trx.weapons.Weapon # The weapon, to read or write the rest of its numbers.
function M.declare(weapon, spec)
  assert(type(spec) == "table", "trx.weapons.declare expects a table")
  return raw.declare(weapon, spec)
end

---Writes a spec into a weapon the game already holds, and raises where it
---holds no such weapon. This is `trx.weapons.declare` for a script that would
---rather hear about a name it got wrong than mint a weapon nothing draws.
---
---```lua
---trx.weapons.patch("uzis", {
---  damage = 2,
---  ammo = { box_shots = 80 },
---})
---```
---@param weapon trx.catalog.weapons|string Which weapon, by id or by name.
---@param spec table Describes the weapon with the same groups as its weapons file entry: `kind`, (`objects`, `meshes`, `ammo`, `aim`, `anim`, `flash`, `glow`, `muzzle`, `smoke`, `shell`, `sound`, `stow`, `save`, `cheat`), and its own numbers beside them. `base` starts the weapon from another one, and `fire` accepts an engine routine name or a function. An omitted key keeps the weapon's current value. An unknown key or an invalid value raises an error and writes nothing.
---
---  `meshes` states where Lara is drawn from while she holds the weapon. It
---  names an `object` and the offsets `hand_r`, `hand_l`, `torso`, `thigh_r`
---  and `thigh_l` into it. An offset of `-1` draws nothing in that place. A
---  weapon with no `meshes` is drawn from the outfit, like every weapon the
---  game ships.
---
---  A spec uses the units of a weapons file: angles in degrees and distances
---  in sectors. Weapon fields use the engine's units, as other API fields do,
---  so a spec that says `aim.speed = 10` reads back as `weapon.aim_speed ==
---  1820`. <!--noref: kind, objects, meshes, ammo, aim, anim, flash, glow,
---  muzzle, smoke, shell, sound, stow, save, cheat, base, fire, object,
---  hand_r, hand_l, torso, thigh_r, thigh_l-->
---@return trx.weapons.Weapon # The weapon, to read or write the rest of its numbers.
function M.patch(weapon, spec)
  assert(type(spec) == "table", "trx.weapons.patch expects a table")
  return raw.patch(weapon, spec)
end

---States what a weapon does when it is fired, in place of the routine it
---fired with before. One weapon holds one handler, so a second call replaces
---the first rather than adding to it. The handler is given the weapon and
---whether Lara is running as she fires, and states the same thing as `fire`
---in a spec.
---<!--noref: fire-->
---
---```lua
---trx.weapons.set_fire("mymod:bigger_gun", function(weapon, running)
---  trx.sound.play(trx.catalog.samples.explosion)
---end)
---```
---@param weapon trx.catalog.weapons|string Which weapon, by id or by name.
---@param handler function Called as the weapon fires.
---@type fun(weapon: trx.catalog.weapons|string, handler: function)
M.set_fire = raw.set_fire

---Retrieves a weapon definition by id or by name.
---
---```lua
---local uzis = trx.weapons.get(trx.catalog.weapons.UZIS)
---uzis.damage = 5
---```
---@param key trx.catalog.weapons|string Weapon id, or its catalog name: `trx.weapons["uzis"]`.
---@return trx.weapons.Weapon? # `nil` if this game has no such weapon.
function M.get(key)
  local id = weapon_id(key)
  if id == nil or id <= trx.catalog.weapons.UNARMED then
    return nil
  end
  return raw.get(id)
end

h.properties(M, "weapons", {
  all = {
    get = function()
      local out = {}
      for _, id in pairs(trx.catalog.weapons) do
        if id > trx.catalog.weapons.UNARMED then
          out[#out + 1] = { id, raw.get(id) }
        end
      end
      table.sort(out, function(a, b)
        return a[1] < b[1]
      end)
      for i, entry in ipairs(out) do
        out[i] = entry[2]
      end
      return out
    end,
  },
})

---Whether the game allows this weapon at all. The game flow can keep one out,
---and a cheat that hands it over anyway leaves Lara with a gun the level was
---built without.
---@deprecated Read `trx.weapons.Weapon.is_available` instead.
---@param weapon trx.catalog.weapons Which weapon. `UNKNOWN`, `UNARMED`, and out-of-range values raise.
---@return boolean # True where this game has the weapon at all.
---@type fun(weapon: trx.catalog.weapons): boolean
M.is_available = raw.is_available

---The pickup the weapon is, for handing it to `trx.inventory:give`.
---
---```lua
---trx.inventory:give(trx.weapons.object(trx.catalog.weapons.SHOTGUN))
---```
---@deprecated Read `trx.weapons.Weapon.object` instead.
---@param weapon trx.catalog.weapons Which weapon. `UNKNOWN`, `UNARMED`, and out-of-range values raise.
---@return trx.catalog.objects? # The object id, or `nil` if this game has no such weapon.
---@type fun(weapon: trx.catalog.weapons): trx.catalog.objects?
M.object = raw.get_object

---The box of ammunition the weapon takes.
---@deprecated Read `trx.weapons.Weapon.ammo_object` instead.
---@param weapon trx.catalog.weapons Which weapon. `UNKNOWN`, `UNARMED`, and out-of-range values raise.
---@return trx.catalog.objects? # The object id, or `nil` where the weapon takes no ammunition.
---@type fun(weapon: trx.catalog.weapons): trx.catalog.objects?
M.ammo_object = raw.get_ammo_object

---How many rounds one pull of the trigger spends. Six for the shotgun, one
---for everything else.
---@deprecated Read `trx.weapons.Weapon.rounds_per_shot` instead.
---@param weapon trx.catalog.weapons Which weapon. `UNKNOWN`, `UNARMED`, and out-of-range values raise.
---@return integer # Rounds, not shots.
---@type fun(weapon: trx.catalog.weapons): integer
M.rounds_per_shot = raw.rounds_per_shot

---How many shots one box of ammunition for it is worth.
---@deprecated Read `trx.weapons.Ammo.box_shots` instead, which is the same number.
---@param weapon trx.catalog.weapons Which weapon. `UNKNOWN`, `UNARMED`, and out-of-range values raise.
---@return integer # Shots, not rounds.
---@type fun(weapon: trx.catalog.weapons): integer
M.shots_per_box = raw.shots_per_box

---Indexing the module reaches a weapon definition, so `trx.weapons.uzis` is
---the uzis. Keyed by weapon id or catalog name, not by position.
---
---```lua
---trx.weapons.uzis.damage = 5
---trx.weapons.shotgun.ammo.box_shots = 12
---trx.weapons.flare.glow.color = "33e5ff"
---```
---@type table<trx.catalog.weapons|string, trx.weapons.Weapon?>
---@trx.key Weapon id, or its catalog name.
h.container("weapons", { base = 0, by_name = true, get = M.get }, M)

local _ = { AimLimits, HandPos, Ammo, Flash, Glow, Anim, Weapon }
