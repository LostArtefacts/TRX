require("trx.signal")

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
  instance_type = "lara.Lara",
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
  description = "A mesh Lara can carry on top of one of her own - the dagger in Home Sweet Home, "
    .. "the oar in a boat.",
  values = {
    TR1_BRAID_DEFAULT_HEAD = "Braided head, out of combat.",
    TR1_BRAID_COMBAT_HEAD = "Braided head, in combat.",
    TR1_BRAID_DEFAULT_TORSO = "Braided torso.",
    TR1_BRAID_MAULED_TORSO = "Braided torso, mauled.",
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
      description = "Warmth remaining in the cold, out of `trx.rules.exposure.max`. Only moves "
        .. "in a level whose rooms carry the `trx.rooms.Room.damaging` flag.",
    },
    poison = {
      from = "poison.value",
      type = "integer",
      description = "How poisoned Lara is, and 0 when she is not.",
    },
    poison_target = {
      from = "poison.target",
      type = "integer",
      description = "The poison reservoir that drains into `trx.lara.poison` over time. TR4 only.",
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
      type = "lara.WaterState",
      writable = false,
      description = "Where Lara is with respect to water.",
    },
    gun_status = {
      from = "gun_status",
      type = "lara.GunState",
      writable = false,
      description = "What Lara's hands are doing.",
    },
    equipped_gun = {
      from = "gun_type",
      type = "catalog.weapons",
      writable = false,
      description = "The weapon Lara is holding.",
    },
    requested_gun = {
      from = "request_gun_type",
      type = "catalog.weapons",
      writable = false,
      description = "The weapon Lara is drawing, while she is drawing it.",
    },
    extra_anim = {
      from = "extra_anim",
      type = "boolean",
      writable = false,
      description = "Whether a scripted animation is driving Lara rather than her own state "
        .. "machine.",
    },
    dive_timer = {
      from = "dive_timer",
      type = "game.Frames",
      writable = false,
      description = "How long Lara has been diving.",
    },
    death_timer = {
      from = "death_timer",
      type = "game.Frames",
      writable = false,
      description = "How long Lara has been dead.",
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
      type = "game.Frames",
      writable = false,
      description = "How long Lara has stood still, which is what starts an idle animation.",
    },
    left_arm_anim_num = {
      from = "left_arm.anim_num",
      type = "integer",
      writable = false,
      description = "The animation Lara's left arm is playing, which follows the weapon in it "
        .. "rather than the rest of her.",
    },
    left_arm_frame_num = {
      from = "left_arm.frame_num",
      type = "integer",
      writable = false,
      description = "The frame that animation is on.",
    },
    right_arm_anim_num = {
      from = "right_arm.anim_num",
      type = "integer",
      writable = false,
      description = "The animation Lara's right arm is playing.",
    },
    right_arm_frame_num = {
      from = "right_arm.frame_num",
      type = "integer",
      writable = false,
      description = "The frame that animation is on.",
    },
    flare_control = {
      from = "flare.control",
      type = "boolean",
      writable = false,
      description = "Whether the flare Lara holds is driving her arm.",
    },
    interact_item_num = {
      from = "interact_target.item_num",
      type = "integer",
      writable = false,
      description = "The item Lara is lining herself up with, by number, or -1 for none.",
    },
    interact_move_count = {
      from = "interact_target.move_count",
      type = "integer",
      writable = false,
      description = "How many frames she has spent moving into place for it.",
    },
    is_interact_moving = {
      from = "interact_target.is_moving",
      type = "boolean",
      writable = false,
      description = "Whether Lara is still moving towards her interaction target.",
    },
  },
})

api.property("lara.animation_object", {
  type = "catalog.objects",
  description = "The object Lara's animations are coming from. It is normally Lara herself, "
    .. "and something else while a vehicle or a scripted sequence drives her.",
  get = function()
    return raw.get_animation_object()
  end,
})

api.const("lara.MAX_AIR", {
  value = trxc.lara.get_max_air(),
  description = "Lara's maximum air, which is what her air runs down from.",
})

api.const("lara.MAX_SPRINT", {
  value = trxc.lara.get_max_sprint(),
  description = "Lara's maximum sprint, which is what her sprint runs down from.",
})

api.property("lara.item", {
  type = "items.Item",
  description = "Lara's own item, or `nil` outside a level. Her position, room and hit "
    .. "points are read and written there.",
  get = function()
    return trx.items[raw.get_item()]
  end,
})

api.property("lara.target", {
  type = "items.Item",
  description = "The item Lara's guns are locked onto, or `nil` if she has none.",
  get = function()
    local target = raw.get_target()
    return target ~= nil and trx.items[target] or nil
  end,
})

api.namespace("lara.signals", {
  description = "The signals Lara's own state speaks through, for a script that would rather "
    .. "hear about a change than ask after one. Each is read once a frame and compared, so a "
    .. "listener runs when the value moved and a value that stood still costs nothing.\n\n"
    .. "What names an item is its number rather than the item itself, because a handle is made "
    .. "afresh on every read and a signal holding one would report a change every frame.",
})

-- What a signal carries is a number, a string or nothing, never a handle: a
-- handle is made afresh on every read, so a signal holding one would report a
-- change every frame. A script woken by one of these reads the handle itself.
local SIGNALS = {
  {
    "exists",
    "boolean",
    "Says when Lara enters the world, and when she leaves it.",
    function()
      return trx.lara.item ~= nil
    end,
  },
  {
    "hp",
    "integer",
    "Says when Lara's hit points change.",
    function()
      local item = trx.lara.item
      return item ~= nil and item.hit_points or nil
    end,
  },
  {
    "max_hp",
    "integer",
    "Says when Lara's maximum hit points change.",
    function()
      local item = trx.lara.item
      return item ~= nil and item.max_hit_points or nil
    end,
  },
  {
    "poison",
    "integer",
    "Says when Lara's poison value changes.",
    function()
      return trx.lara.poison
    end,
  },
  {
    "air",
    "integer",
    "Says when the air Lara has left underwater changes.",
    function()
      return trx.lara.air_bar
    end,
  },
  {
    "sprint",
    "integer",
    "Says when the sprint Lara has left changes.",
    function()
      return trx.lara.sprint_timer
    end,
  },
  {
    "exposure",
    "integer",
    "Says when the warmth Lara has left in the cold changes.",
    function()
      return trx.lara.exposure_bar
    end,
  },
  {
    "gun_status",
    "lara.GunState",
    "Says when Lara draws a weapon or puts one away.",
    function()
      return trx.lara.gun_status
    end,
  },
  {
    "water_status",
    "lara.WaterState",
    "Says when Lara enters or leaves the water.",
    function()
      return trx.lara.water_status
    end,
  },
  {
    "room_num",
    "integer",
    "Says when Lara changes rooms. Read `trx.lara.item.room` for the room itself.",
    function()
      local item = trx.lara.item
      return item ~= nil and item.room_num or nil
    end,
  },
  {
    "is_controllable",
    "boolean",
    "Says when Lara stops answering to the player, or starts again.",
    function()
      return trx.lara.is_controllable
    end,
  },
  {
    "target",
    "integer",
    "Says when what Lara's guns are locked onto changes. Read `trx.lara.target` for "
      .. "the item itself.",
    function()
      local target = trx.lara.target
      return target ~= nil and target.num or nil
    end,
  },
  {
    "vehicle",
    "integer",
    "Says when Lara gets on or off a vehicle. Read `trx.lara.vehicle` for it.",
    function()
      local vehicle = trx.lara.vehicle
      return vehicle ~= nil and vehicle.num or nil
    end,
  },
  {
    "equipped_gun",
    "string",
    "Says when Lara changes weapon.",
    function()
      return trx.lara.equipped_gun
    end,
  },
}

for _, entry in ipairs(SIGNALS) do
  local name, value_type, description, read =
    entry[1], entry[2], entry[3], entry[4]
  local held = nil
  api.property("lara.signals." .. name, {
    type = "signal.Signal",
    description = description,
    get = function()
      if held == nil then
        held = trx.signal.polled(read)
      end
      return held
    end,
  })
end

api.property("lara.vehicle", {
  type = "items.Item",
  description = "The vehicle Lara is riding, or `nil` when she is on her own feet. Its speed "
    .. "and position are the ones that move her while she rides it.",
  get = function()
    local vehicle = raw.get_vehicle()
    return vehicle ~= nil and trx.items[vehicle] or nil
  end,
})

api.property("lara.is_controllable", {
  type = "boolean",
  description = "Whether Lara answers to the player. False while she is dead, while the "
    .. "inventory or a dialog holds the game, and while a cutscene or flyby is active.",
  get = raw.is_controllable,
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

api.property("lara.speech_face", {
  type = "number",
  description = "Which of her outfit's speech faces Lara wears while she talks, counted from "
    .. "0, or `nil` for her own face. An outfit with no speech faces keeps her own.\n\n"
    .. "The face is remembered, so putting her in another outfit mid-sentence dresses her in "
    .. "that outfit's face rather than leaving the one she had.",
  get = function()
    local index = raw.get_speech_face()
    return index >= 0 and index or nil
  end,
  set = function(index)
    raw.set_speech_face(index)
  end,
  examples = {
    [[trx.lara.speech_face = trx.random.randint(0, 3)]],
  },
})

api.property("lara.is_flying", {
  type = "boolean",
  description = "Whether Lara is in the fly-mode cheat. Setting it enters or leaves fly mode.",
  get = raw.is_flying,
  set = raw.set_flying,
})

api.property("lara.is_wet", {
  type = "boolean",
  description = "Whether Lara is still shedding droplets after a swim. `trx.lara.dry` clears it.",
  get = raw.is_wet,
})

api.property("lara.vehicle_gun", {
  type = "catalog.weapons",
  nullable = true,
  description = "The weapon the vehicle Lara is riding carries. Her own weapons are put away "
    .. "while she rides, so this is what her ammunition counter shows. `nil` where she is "
    .. "riding nothing, or riding something unarmed.",
  get = raw.vehicle_gun,
})

api.property("lara.has_pistol_weapon", {
  type = "boolean",
  description = "Whether Lara is carrying a pistol-class weapon, which is what decides whether she "
    .. "has holsters to show at all.",
  get = raw.has_pistol_weapon,
})

api.define("lara.set_extra_equipment", {
  description = "Hangs an extra mesh on one of Lara's own, replacing the mesh there.",
  params = {
    {
      name = "mesh",
      type = "lara.Mesh",
      description = "Which of Lara's meshes.",
    },
    {
      name = "extra_mesh",
      type = "lara.ExtraMesh",
      description = "The mesh to hang on it.",
    },
  },
  examples = {
    [[trx.lara.set_extra_equipment(trx.lara.Mesh.HAND_R, trx.lara.ExtraMesh.OAR)]],
  },
  impl = raw.set_extra_equipment,
})

api.define("lara.teleport", {
  description = "Moves Lara to a world position, putting her down on the floor there. She is "
    .. "taken off any vehicle, her weapons are put away and the camera follows her over.\n\n"
    .. "The position is nudged into valid room geometry, so a spot inside a wall lands her beside "
    .. "it rather than in it. Somewhere with no floor within reach moves nothing.",
  params = {
    { name = "pos", type = "math.Vec3", description = "World position." },
    {
      name = "room_num",
      type = "rooms.Num",
      optional = true,
      description = "Without it, the room is found from the position.",
    },
  },
  returns = { type = "boolean", description = "Whether she was moved." },
  examples = {
    [[trx.lara.teleport(trx.items.query:of_object("wolf"):first().pos)]],
  },
  impl = raw.teleport,
})

api.define("lara.cure_poison", {
  description = "Cures Lara's poisoning. Not the same as writing `0` to `trx.lara.poison`: the poison has a "
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

api.define("lara.set_mesh", {
  description = [[
    Puts another object's mesh on one of Lara's own, in place of whatever her
    outfit gives her there.

    It outlives an outfit change, because applying an outfit reads it, which is
    what lets a level dress her from its own geometry rather than from the
    outfit. Her head is the exception: a combat or speech face replaces it
    directly, and takes it back from an override with it.

    The override is dropped when the level ends, along with the meshes it could
    name.
  ]],
  params = {
    {
      name = "mesh",
      type = "lara.Mesh",
      description = "Which of Lara's meshes.",
    },
    {
      name = "object",
      type = "catalog.Id",
      description = "The object to take a mesh from. Raises if this level does not carry it.",
    },
    {
      name = "mesh_num",
      type = "objects.MeshNum",
      description = "Which of that object's meshes.",
    },
  },
  examples = {
    [[-- the torso young Lara wears before she picks up her backpack
trx.lara.set_mesh(trx.lara.Mesh.TORSO, trx.catalog.objects.lara_skin, 7)]],
  },
  impl = raw.set_mesh,
})

api.define("lara.clear_mesh", {
  description = "Takes the override back off, leaving the mesh Lara's outfit gives her.",
  params = {
    {
      name = "mesh",
      type = "lara.Mesh",
      description = "Which of Lara's meshes.",
    },
  },
  impl = raw.clear_mesh,
})

api.define("lara.clear_equipment", {
  description = "Takes the extra mesh back off, leaving Lara's own.",
  params = {
    {
      name = "mesh",
      type = "lara.Mesh",
      description = "Which of Lara's meshes.",
    },
  },
  impl = raw.clear_equipment,
})
