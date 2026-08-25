local raw = trxc.weapons
local api = trx.api

require("trx.math")
require("trx.catalog")

api.module("weapons", {
  order = 5,
  title = "Weapon",
  description = [[
What a weapon is, rather than what Lara has of it.

None of this differs between the inventory she carries and the one a level
keeps for her, so it belongs to neither: what she holds and how many shots she
has are `trx.inventory`.

A weapon is shared by every copy of it. Changes last for the rest of the
session, so levels should restore any values they change when they end.]],
})

local weapon_param = {
  name = "weapon",
  type = "catalog.weapons",
  description = "Which weapon. `UNKNOWN`, `UNARMED`, and out-of-range values raise.",
}

api.enum("weapons.Kind", {
  backing = "WEAPON_TYPE",
  strip = "WEAPON_TYPE_",
  description = "How the engine holds and fires a weapon, which decides which arm animations and "
    .. "firing routine it uses.",
  values = {
    DUAL_PISTOLS = "One in each hand, each arm aiming and firing on its own.",
    SINGLE_PISTOL = "One in the right hand.",
    RIFLE = "Held in both hands, drawn from Lara's back.",
    MOUNTED = "Fixed to a vehicle rather than held.",
    FLARE = "Held in one hand and burning, rather than fired.",
  },
})

api.type("weapons.AimLimits", {
  backing = "WEAPON_AIM_LIMITS",
  description = "How far off straight ahead an aim may go, as a pair of limits about each axis.",
  fields = {
    min_yaw = {
      from = "min_yaw",
      type = "math.Angle",
      description = "As far to the left as the aim reaches, which is a negative angle.",
    },
    max_yaw = {
      from = "max_yaw",
      type = "math.Angle",
      description = "As far to the right as it reaches.",
    },
    min_pitch = {
      from = "min_pitch",
      type = "math.Angle",
      description = "As far up as it reaches, which is a negative angle.",
    },
    max_pitch = {
      from = "max_pitch",
      type = "math.Angle",
      description = "As far down as it reaches.",
    },
  },
})

api.type("weapons.HandPos", {
  backing = "WEAPON_HAND_POS",
  description = "An offset in the frame of the hand that holds the weapon. A weapon held in one "
    .. "hand only uses the right.",
  fields = {
    right = {
      from = "right",
      type = "math.Vec3",
      description = "Offset in the right hand.",
    },
    left = {
      from = "left",
      type = "math.Vec3",
      description = "Offset in the left hand.",
    },
  },
})

api.type("weapons.Ammo", {
  backing = "WEAPON_AMMO_INFO",
  description = [[
What the weapon is fed. A shot is one pull of the trigger, which for the shotgun
spends six rounds; the flare counts a flare where a weapon counts a shot.]],
  fields = {
    initial_shots = {
      from = "initial_shots",
      type = "integer",
      description = "What the weapon arrives with the first time Lara picks it up.",
    },
    box_shots = {
      from = "box_shots",
      type = "integer",
      description = "What one box of ammunition is worth.",
    },
    box_label_qty = {
      from = "box_label_qty",
      type = "integer",
      description = "What a box shows on its inventory icon, which follows nothing else.",
    },
    infinite = {
      from = "infinite",
      type = "boolean",
      description = "Whether firing spends nothing, so the weapon never runs out and carries no "
        .. "counter.",
    },
  },
})

api.type("weapons.Flash", {
  backing = "WEAPON_FLASH_INFO",
  description = "The muzzle flash a shot draws.",
  fields = {
    time = {
      from = "time",
      type = "integer",
      description = "How many frames the flash stays on screen for.",
    },
    shade = {
      from = "shade",
      type = "integer",
      description = "How brightly it lights the model around it, in TR1 and TR2. Later games light "
        .. "it by `trx.weapons.Flash.color` instead.",
    },
    color = {
      from = "color",
      type = "math.Color",
      description = "What color it lights with, in TR3 and later.",
    },
  },

  extensions = {
    pos = {
      type = "weapons.HandPos",
      description = "Where the flash is drawn, in each hand.",
      impl = function(flash)
        return raw.get_flash_pos(flash)
      end,
    },
  },
})

api.type("weapons.Glow", {
  backing = "WEAPON_GLOW_INFO",
  description = "The glow sprite drawn where the weapon burns: a gun's muzzle, or a lit flare.",
  fields = {
    color = {
      from = "color",
      type = "math.Color",
      description = "What color the glow is drawn in.",
    },
    pos = {
      from = "pos",
      type = "math.Vec3",
      description = "Where it sits, in the frame of the mesh it follows.",
    },
    scale = {
      from = "scale",
      type = "number",
      description = "Multiplies the sprite's own size. `0` turns the glow off.",
    },
    flicker = {
      from = "flicker",
      type = "boolean",
      description = "Whether the brightness is randomized every frame, the way a flare burns.",
    },
  },
})

api.type("weapons.Anim", {
  backing = "WEAPON_ANIM_INFO",
  description = "The animation numbers a rifle is drawn, put away and fired by. They count the "
    .. "animations and frames of the weapon's own object, not Lara's.",
  fields = {
    equip_anim = {
      from = "equip_anim_idx",
      type = "integer",
      description = "The animation the weapon starts on as Lara reaches for it. One the object "
        .. "does not have raises.",
    },
    draw_frame = {
      from = "draw_frame",
      type = "integer",
      description = "The frame the weapon appears in her hands on.",
    },
    undraw_frame = {
      from = "undraw_frame",
      type = "integer",
      description = "The frame it leaves her hands on.",
    },
    recoil_frame = {
      from = "recoil_frame",
      type = "integer",
      description = "The frame a pistol kicks on.",
    },
  },
})

api.type("weapons.Weapon", {
  backing = "WEAPON_INFO",
  description = "A weapon definition, reached as `trx.weapons.uzis` or by id.",

  fields = {
    id = {
      from = "id",
      type = "catalog.weapons",
      writable = false,
      description = "Which weapon this is, for the calls that take one: "
        .. "`trx.inventory:set_shots(weapon.id, 100)`.",
    },
    kind = {
      from = "type",
      type = "weapons.Kind",
      description = "How the engine holds and fires it.",
    },
    is_available = {
      from = "is_available",
      type = "boolean",
      description = "Whether the game allows the weapon at all. Turning one off keeps it out of "
        .. "the cheats and off the controls list, and a save that carries it arrives without it.",
    },
    aim_speed = {
      from = "aim_speed",
      type = "math.Angle",
      description = "How far the arms swing towards the target each frame.",
    },
    shot_accuracy = {
      from = "shot_accuracy",
      type = "math.Angle",
      description = "How wide a cone a shot may stray into. `0` never misses.",
    },
    gun_height = {
      from = "gun_height",
      type = "math.Distance",
      description = "How far above Lara's feet the shot leaves the barrel. It also decides how "
        .. "deep she can wade and still fire.",
    },
    damage = {
      from = "damage",
      type = "integer",
      description = "Hit points one shot takes off what it hits.",
    },
    target_dist = {
      from = "target_dist",
      type = "math.Distance",
      description = "How far the weapon reaches, both for auto-aim and for the shot itself.",
    },
    smoke_count = {
      from = "smoke_count",
      type = "integer",
      description = "How many puffs of smoke a shot leaves at the muzzle, in TR3. `0` for none.",
    },
    fire_sample = {
      from = "sample_num",
      type = "catalog.samples",
      description = "The sample a shot plays. One this game has no sound for is silent.",
    },
    fire_overlay_sample = {
      from = "sample_overlay_num",
      type = "catalog.samples",
      description = "The overlay sample a shot plays. One this game has no sound for is silent.",
    },
    fire_overlay_pitch = {
      from = "sample_overlay_pitch",
      type = "integer",
      description = "The pitch at which to play the overlay sample.",
    },
  },

  extensions = {
    object = {
      type = "catalog.objects",
      nullable = true,
      description = "The pickup the weapon is, for handing it to `trx.inventory:give`. `nil` where "
        .. "this game has no such weapon.",
      impl = function(weapon)
        return raw.get_object(weapon.id)
      end,
    },
    ammo_object = {
      type = "catalog.objects",
      nullable = true,
      description = "The box of ammunition it takes, or `nil` where it takes none.",
      impl = function(weapon)
        return raw.get_ammo_object(weapon.id)
      end,
    },
    rounds_per_shot = {
      type = "integer",
      description = "How many rounds one pull of the trigger spends: six for the shotgun, one for "
        .. "everything else. What a box is worth in shots is `trx.weapons.Ammo.box_shots`.",
      impl = function(weapon)
        return raw.rounds_per_shot(weapon.id)
      end,
    },
    lock = {
      type = "weapons.AimLimits",
      description = "Where auto-aim may lock on, measured from where Lara faces.",
      impl = function(weapon)
        return raw.get_lock(weapon)
      end,
    },
    left_arm = {
      type = "weapons.AimLimits",
      description = "How far the left arm may follow a target it has locked onto. A dual-wielded "
        .. "weapon drops the lock on the arm that cannot reach.",
      impl = function(weapon)
        return raw.get_left_arm(weapon)
      end,
    },
    right_arm = {
      type = "weapons.AimLimits",
      description = "How far the right arm may follow a target.",
      impl = function(weapon)
        return raw.get_right_arm(weapon)
      end,
    },
    ammo = {
      type = "weapons.Ammo",
      description = "What the weapon is fed.",
      impl = function(weapon)
        return raw.get_ammo(weapon)
      end,
    },
    anim = {
      type = "weapons.Anim",
      description = "The animation numbers it is drawn and fired by.",
      impl = function(weapon)
        return raw.get_anim(weapon)
      end,
    },
    flash = {
      type = "weapons.Flash",
      description = "The muzzle flash a shot draws.",
      impl = function(weapon)
        return raw.get_flash(weapon)
      end,
    },
    glow = {
      type = "weapons.Glow",
      description = "The glow drawn where it burns.",
      impl = function(weapon)
        return raw.get_glow(weapon)
      end,
    },
    muzzle_pos = {
      type = "weapons.HandPos",
      description = "Where the barrel ends, which is where smoke and sparks come from.",
      impl = function(weapon)
        return raw.get_muzzle_pos(weapon)
      end,
    },
    shell_pos = {
      type = "weapons.HandPos",
      description = "Where a spent shell is thrown from. A weapon that leaves no shells has this "
        .. "at the origin.",
      impl = function(weapon)
        return raw.get_shell_pos(weapon)
      end,
    },
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

local get = api.define("weapons.get", {
  description = "Retrieves a weapon definition by id or by name.",
  params = {
    {
      name = "key",
      type = { "catalog.weapons", "string" },
      description = 'Weapon id, or its catalog name: `trx.weapons["uzis"]`.',
    },
  },
  returns = {
    type = "weapons.Weapon",
    nullable = true,
    description = "`nil` if this game has no such weapon.",
  },
  examples = {
    [[local uzis = trx.weapons.get(trx.catalog.weapons.UZIS)
uzis.damage = 5]],
  },
  impl = function(key)
    local id = weapon_id(key)
    if id == nil or id <= trx.catalog.weapons.UNARMED then
      return nil
    end
    return raw.get(id)
  end,
})

api.property("weapons.all", {
  type = "weapons.Weapon",
  list = true,
  description = "Every weapon the engine knows, in the order it holds them. `UNARMED` is not one "
    .. "of them.",
  examples = {
    [[for _, weapon in ipairs(trx.weapons.all) do
  weapon.is_available = true
end]],
  },
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
})

api.define("weapons.is_available", {
  deprecated = "Read `trx.weapons.Weapon.is_available` instead.",
  description = [[
Whether the game allows this weapon at all. The game flow can keep one out, and
a cheat that hands it over anyway leaves Lara with a gun the level was built
without.]],
  params = { weapon_param },
  returns = {
    type = "boolean",
    description = "True where this game has the weapon at all.",
  },
  impl = raw.is_available,
})

api.define("weapons.object", {
  deprecated = "Read `trx.weapons.Weapon.object` instead.",
  description = "The pickup the weapon is, for handing it to `trx.inventory:give`.",
  params = { weapon_param },
  returns = {
    type = "catalog.objects",
    nullable = true,
    description = "The object id, or `nil` if this game has no such weapon.",
  },
  examples = {
    [[trx.inventory:give(trx.weapons.object(trx.catalog.weapons.SHOTGUN))]],
  },
  impl = raw.get_object,
})

api.define("weapons.ammo_object", {
  deprecated = "Read `trx.weapons.Weapon.ammo_object` instead.",
  description = "The box of ammunition the weapon takes.",
  params = { weapon_param },
  returns = {
    type = "catalog.objects",
    nullable = true,
    description = "The object id, or `nil` where the weapon takes no ammunition.",
  },
  impl = raw.get_ammo_object,
})

api.define("weapons.rounds_per_shot", {
  deprecated = "Read `trx.weapons.Weapon.rounds_per_shot` instead.",
  description = "How many rounds one pull of the trigger spends. Six for the shotgun, one for "
    .. "everything else.",
  params = { weapon_param },
  returns = { type = "integer", description = "Rounds, not shots." },
  impl = raw.rounds_per_shot,
})

api.define("weapons.shots_per_box", {
  deprecated = "Read `trx.weapons.Ammo.box_shots` instead, which is the same number.",
  description = "How many shots one box of ammunition for it is worth.",
  params = { weapon_param },
  returns = { type = "integer", description = "Shots, not rounds." },
  impl = raw.shots_per_box,
})

api.container("weapons", {
  description = "Indexing the module reaches a weapon definition, so `trx.weapons.uzis` is the "
    .. "uzis. Keyed by weapon id or catalog name, not by position.",
  key = {
    type = { "catalog.weapons", "string" },
    description = "Weapon id, or its catalog name.",
  },
  value = { type = "weapons.Weapon", nullable = true },
  examples = {
    [[trx.weapons.uzis.damage = 5
trx.weapons.shotgun.ammo.box_shots = 12
trx.weapons.flare.glow.color = "33e5ff"]],
  },
  get = get,
})
