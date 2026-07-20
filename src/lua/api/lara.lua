local raw = trxc.lara
local api = trx.api

-- trx.lara stands for one C struct, so reading trx.lara.air reads Lara's air.
-- What is reachable is what lara.Lara declares below, and nothing else.
api.module("lara", {
  order = 3,
  description = "Module for reading and nudging Lara's own state.\n\n"
    .. "Her position, room and hit points are not here: she is an item like any other and they "
    .. "live on it, as `trx.lara.item`.",
  instance = raw.state,
})

api.enum("lara.Mesh", {
  backing = "LARA_MESH",
  description = "One of the fifteen meshes Lara is built from.",
  values = {
    HIPS = "Hips, the mesh the rest hang off.",
    THIGH_L = "Left thigh.",
    CALF_L = "Left calf.",
    FOOT_L = "Left foot.",
    THIGH_R = "Right thigh.",
    CALF_R = "Right calf.",
    FOOT_R = "Right foot.",
    TORSO = "Torso.",
    UARM_R = "Right upper arm.",
    LARM_R = "Right lower arm.",
    HAND_R = "Right hand.",
    UARM_L = "Left upper arm.",
    LARM_L = "Left lower arm.",
    HAND_L = "Left hand.",
    HEAD = "Head.",
  },
})

-- The C names carry an EXTRA_MESH_ prefix, because cfg/outfits.json5 is keyed
-- by those exact strings and cannot move. Strip it here rather than say it
-- twice.
api.enum("lara.ExtraMesh", {
  backing = "LARA_SKIN_EXTRA_MESH",
  strip = "EXTRA_MESH_",
  description = "A mesh Lara can carry on top of one of her own - the dagger in Home Sweet Home, "
    .. "the oar in a boat.",
  values = {
    TR1_BRAID_DEFAULT_HEAD = "Braided head, out of combat.",
    TR1_BRAID_COMBAT_HEAD = "Braided head, in combat.",
    TR1_BRAID_DEFAULT_TORSO = "Braided torso.",
    TR1_BRAID_MAULED_TORSO = "Braided torso, mauled.",
    TR1_BRAID_GOLD_HEAD = "Braided head, gold.",
    TR1_BRAID_GOLD_TORSO = "Braided torso, gold.",
    DAGGER_HAND = "Dagger, in hand.",
    DAGGER_HIPS = "Dagger, sheathed at the hips.",
    OAR = "Oar.",
    SPANNER = "Spanner.",
    DRINK_CAN = "Drink can.",
    GLASSES_OPAQUE = "Sunglasses.",
    GLASSES_TRANSPARENT = "Sunglasses, transparent lenses.",
    CROWBAR = "Crowbar.",
    WOODEN_TORCH = "Wooden torch.",
    BINOCULARS = "Binoculars.",
    HOOK_AND_POLE = "Hook and pole.",
    DETONATOR = "Detonator.",
    SHOVEL = "Shovel.",
    JERRYCAN = "Jerrycan.",
    SANDBAG = "Sandbag.",
    WATERSKIN = "Waterskin.",
  },
})

api.enum("lara.WaterState", {
  backing = "LARA_WATER_STATE",
  description = "Where Lara is with respect to water.",
  values = {
    ABOVE_WATER = "On dry land.",
    UNDERWATER = "Under the surface.",
    SURFACE = "Swimming at the surface.",
    WADE = "Wading, feet still on the floor.",
    CHEAT = "Flying, as the fly cheat leaves her.",
  },
})

api.enum("lara.GunState", {
  backing = "LARA_GUN_STATE",
  description = "What Lara's hands are doing.",
  values = {
    ARMLESS = "Empty-handed.",
    HANDS_BUSY = "Hands full, so nothing can be drawn.",
    DRAW = "Drawing a weapon.",
    UNDRAW = "Putting one away.",
    READY = "Armed, weapon out.",
    SPECIAL = "In a scripted sequence.",
  },
})

-- The fields of LARA_INFO, named for scripts. The C side says where each member
-- lives (see src/trx/game/lara/fields.c); this says which of them exist.
api.type("lara.Lara", {
  backing = "LARA_INFO",
  description = "Lara's own state, reachable straight off `trx.lara`.",

  fields = {
    air_bar = {
      from = "air",
      type = "integer",
      description = "Air remaining underwater, out of 1800. Runs down while she is under.",
    },
    exposure_bar = {
      from = "exposure_timer",
      type = "integer",
      description = "Warmth remaining in the cold, out of 600. Only moves in a level whose rooms "
        .. "carry the `damaging` flag.",
    },
    poison = {
      from = "poison.value",
      type = "integer",
      description = "How poisoned Lara is, and 0 when she is not.",
    },
    poison_target = {
      from = "poison.target",
      type = "integer",
      description = "The poison reservoir that drains into `poison` over time. TR4 only.",
    },
    electric = {
      from = "electric",
      type = "integer",
      description = "How badly Lara is being electrocuted, and 0 when she is not.",
    },
    is_burning = {
      from = "burn",
      type = "boolean",
      description = "Whether Lara is on fire. Setting it lights her or puts her out.",
    },
    is_crouched = {
      from = "is_crouched",
      type = "boolean",
      writable = false,
      description = "Whether Lara is crouching.",
    },
    is_climbing = {
      from = "climb_status",
      type = "boolean",
      writable = false,
      description = "Whether Lara is on a climbable wall.",
    },
    water_status = {
      from = "water_status",
      type = "integer",
      writable = false,
      enum = "lara.WaterState",
      description = "Where Lara is with respect to water.",
    },
    gun_status = {
      from = "gun_status",
      type = "integer",
      writable = false,
      enum = "lara.GunState",
      description = "What Lara's hands are doing.",
    },
    equipped_gun = {
      from = "gun_type",
      type = "integer",
      writable = false,
      enum = "catalog.weapons",
      description = "The weapon Lara is holding.",
    },
    requested_gun = {
      from = "request_gun_type",
      type = "integer",
      writable = false,
      enum = "catalog.weapons",
      description = "The weapon Lara is drawing, while she is drawing it.",
    },
    extra_anim = {
      from = "extra_anim",
      type = "boolean",
      writable = false,
      description = "Whether a scripted animation is driving Lara rather than her own state "
        .. "machine.",
    },
    killed_loyal_item = {
      from = "killed_loyal_item",
      type = "boolean",
      writable = false,
      description = "Whether Lara has killed one of her own allies, which is what turns the rest "
        .. "of them on her.",
    },
    dive_timer = {
      from = "dive_timer",
      type = "integer",
      writable = false,
      description = "Frames Lara has been diving for.",
    },
    death_timer = {
      from = "death_timer",
      type = "integer",
      writable = false,
      description = "Frames Lara has been dead for.",
    },
    sprint_timer = {
      from = "sprint_timer",
      type = "integer",
      description = "Sprint left in her legs.",
    },
    hit_direction = {
      from = "hit_direction",
      type = "integer",
      writable = false,
      description = "Which way the last hit came from, or -1 if she has not been hit.",
    },
    pose_count = {
      from = "pose_count",
      type = "integer",
      writable = false,
      description = "Frames Lara has stood still for, which is what starts an idle animation.",
    },
  },
})

api.property("lara.item", {
  type = "Item",
  description = "Lara's own `trx.items.Item`, or `nil` outside a level. Her position, room and hit "
    .. "points are read and written there.",
  get = function()
    return trx.items[raw.get_item()]
  end,
})

api.property("lara.target", {
  type = "Item",
  description = "The item Lara's guns are locked onto, or `nil` if she has none.",
  get = function()
    local target = raw.get_target()
    return target ~= nil and trx.items[target] or nil
  end,
})

api.property("lara.outfit", {
  type = "string",
  description = "The outfit Lara is wearing, by name, as defined in `cfg/outfits.json5`.",
  get = raw.get_outfit,
  set = raw.set_outfit,
})

api.property("lara.holsters_visible", {
  type = "boolean",
  description = "Whether Lara's holsters are drawn on her hips.",
  get = raw.are_holsters_visible,
  set = raw.set_holsters_visible,
})

api.property("lara.is_flying", {
  type = "boolean",
  description = "Whether Lara is in the fly-mode cheat. Setting it enters or leaves fly mode.",
  get = raw.is_flying,
  set = raw.set_flying,
})

api.property("lara.is_wet", {
  type = "boolean",
  description = "Whether Lara is still shedding droplets after a swim. `dry` clears it.",
  get = raw.is_wet,
})

api.property("lara.has_pistol_weapon", {
  type = "boolean",
  description = "Whether Lara is carrying a pistol-class weapon, which is what decides whether she "
    .. "has holsters to show at all.",
  get = raw.has_pistol_weapon,
})

api.define("lara.set_extra_equipment", {
  description = "Hangs an extra mesh on one of Lara's own, replacing whatever is there.",
  params = {
    {
      name = "mesh",
      type = "integer",
      enum = "lara.Mesh",
      description = "Which of Lara's meshes.",
    },
    {
      name = "extra_mesh",
      type = "integer",
      enum = "lara.ExtraMesh",
      description = "The mesh to hang on it.",
    },
  },
  examples = {
    [[trx.lara.set_extra_equipment(trx.lara.Mesh.HAND_R, trx.lara.ExtraMesh.OAR)]],
  },
  impl = raw.set_extra_equipment,
})

api.define("lara.cure_poison", {
  description = "Cures Lara's poisoning. Not the same as writing `0` to `poison`: the poison has a "
    .. "target as well as a current value, and clearing only the value lets it climb back.",
  impl = raw.cure_poison,
})

api.define("lara.extinguish", {
  description = "Puts Lara's fire out, and stops her being electrocuted with it.",
  impl = raw.extinguish,
})

api.define("lara.dry", {
  description = "Dries Lara off, clearing the wetness that sheds droplets after she leaves "
    .. "water.",
  impl = raw.dry,
})

api.define("lara.clear_equipment", {
  description = "Takes the extra mesh back off, leaving Lara's own.",
  params = {
    {
      name = "mesh",
      type = "integer",
      enum = "lara.Mesh",
      description = "Which of Lara's meshes.",
    },
  },
  impl = raw.clear_equipment,
})
