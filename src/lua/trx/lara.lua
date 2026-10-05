require("trx.signal")

local raw = trxc.lara
local h = require("trx.internal.helpers")

-- trx.lara stands for one C struct, so reading trx.lara.air reads Lara's air.
-- What is reachable is what lara.Lara declares below, and nothing else.

---@class trx
---@field lara trx.lara

---Module for reading and nudging Lara's own state.
---
---Her position, room and hit points are not here: she is an item like any
---other and they live on it, as `trx.lara.item`.
---@trx.module 3
---@class (exact) trx.lara: trx.lara.Lara
---@trx.readonly animation_object, can_pose, has_pistol_weapon,
---  is_controllable, is_wet, item, target, vehicle, vehicle_gun
---@field can_pose boolean Whether poses are available for Lara to cycle through in photo mode. This is false when no poses are defined or during cutscenes.
---@field animation_object trx.catalog.objects The object Lara's animations are coming from. It is normally Lara herself, and something else while a vehicle or a scripted sequence drives her.
---@field item trx.items.Item Lara's own item, or `nil` outside a level. Her position, room and hit points are read and written there.
---@field target trx.items.Item The item Lara's guns are locked onto, or `nil` if she has none.
---@field vehicle trx.items.Item The vehicle Lara is riding, or `nil` when she is on her own feet. Its speed and position are the ones that move her while she rides it.
---@field is_controllable boolean Whether Lara answers to the player. False while she is dead, while the inventory or a dialog holds the game, and while a cutscene or flyby is active.
---@field outfit string The outfit Lara is wearing, by name, as defined in `cfg/outfits.json5`.
---@field holsters_visible boolean Whether Lara's holsters are drawn on her hips.
---@field speech_face number Which of her outfit's speech faces Lara wears while she talks, counted from 0, or `nil` for her own face. An outfit with no speech faces keeps her own.
---
---  The face is remembered, so putting her in another outfit mid-sentence
---  dresses her in that outfit's face rather than leaving the one she had.
---@field is_flying boolean Whether Lara is in the fly-mode cheat. Setting it enters or leaves fly mode.
---@field is_wet boolean Whether Lara is still shedding droplets after a swim. `trx.lara.dry` clears it.
---@field vehicle_gun trx.catalog.weapons? The weapon the vehicle Lara is riding carries. Her own weapons are put away while she rides, so this is what her ammunition counter shows. `nil` where she is riding nothing, or riding something unarmed.
---@field has_pistol_weapon boolean Whether Lara is carrying a pistol-class weapon, which is what decides whether she has holsters to show at all.
local M = h.module("lara")

---One of the fifteen meshes Lara is built from.
---@enum trx.lara.Mesh
local Mesh = {
  ---Hips, the mesh the rest hang off.
  HIPS = h.IntegerConstant,
  ---Left thigh.
  THIGH_L = h.IntegerConstant,
  ---Left calf.
  CALF_L = h.IntegerConstant,
  ---Left foot.
  FOOT_L = h.IntegerConstant,
  ---Right thigh.
  THIGH_R = h.IntegerConstant,
  ---Right calf.
  CALF_R = h.IntegerConstant,
  ---Right foot.
  FOOT_R = h.IntegerConstant,
  ---Torso.
  TORSO = h.IntegerConstant,
  ---Right upper arm.
  UARM_R = h.IntegerConstant,
  ---Right lower arm.
  LARM_R = h.IntegerConstant,
  ---Right hand.
  HAND_R = h.IntegerConstant,
  ---Left upper arm.
  UARM_L = h.IntegerConstant,
  ---Left lower arm.
  LARM_L = h.IntegerConstant,
  ---Left hand.
  HAND_L = h.IntegerConstant,
  ---Head.
  HEAD = h.IntegerConstant,
}
M.Mesh = h.enum("lara.Mesh", "LARA_MESH", Mesh)

-- The C names carry an EXTRA_MESH_ prefix, because cfg/outfits.json5 is keyed
-- by those exact strings and cannot move. Strip it here rather than say it
-- twice.

---A mesh Lara can carry on top of one of her own - the dagger in Home Sweet
---Home, the oar in a boat.
---@enum trx.lara.ExtraMesh
local ExtraMesh = {
  ---Braided head, out of combat.
  TR1_BRAID_DEFAULT_HEAD = h.IntegerConstant,
  ---Braided head, in combat.
  TR1_BRAID_COMBAT_HEAD = h.IntegerConstant,
  ---Braided torso.
  TR1_BRAID_DEFAULT_TORSO = h.IntegerConstant,
  ---Braided torso, mauled.
  TR1_BRAID_MAULED_TORSO = h.IntegerConstant,
  ---Dagger, in hand.
  DAGGER_HAND = h.IntegerConstant,
  ---Dagger, sheathed at the hips.
  DAGGER_HIPS = h.IntegerConstant,
  ---Oar.
  OAR = h.IntegerConstant,
  ---Spanner.
  SPANNER = h.IntegerConstant,
  ---Drink can.
  DRINK_CAN = h.IntegerConstant,
  ---Sunglasses.
  GLASSES_OPAQUE = h.IntegerConstant,
  ---Sunglasses, transparent lenses.
  GLASSES_TRANSPARENT = h.IntegerConstant,
  ---Crowbar.
  CROWBAR = h.IntegerConstant,
  ---Wooden torch.
  WOODEN_TORCH = h.IntegerConstant,
  ---Binoculars.
  BINOCULARS = h.IntegerConstant,
  ---Hook and pole.
  HOOK_AND_POLE = h.IntegerConstant,
  ---Detonator.
  DETONATOR = h.IntegerConstant,
  ---Shovel.
  SHOVEL = h.IntegerConstant,
  ---Jerrycan.
  JERRYCAN = h.IntegerConstant,
  ---Sandbag.
  SANDBAG = h.IntegerConstant,
  ---Waterskin.
  WATERSKIN = h.IntegerConstant,
}
M.ExtraMesh = h.enum("lara.ExtraMesh", "LARA_SKIN_EXTRA_MESH", ExtraMesh)

---Where Lara is with respect to water.
---@enum trx.lara.WaterState
local WaterState = {
  ---On dry land.
  ABOVE_WATER = h.IntegerConstant,
  ---Under the surface.
  UNDERWATER = h.IntegerConstant,
  ---Swimming at the surface.
  SURFACE = h.IntegerConstant,
  ---Wading, feet still on the floor.
  WADE = h.IntegerConstant,
  ---Flying, as the fly cheat leaves her.
  CHEAT = h.IntegerConstant,
}
M.WaterState = h.enum("lara.WaterState", "LARA_WATER_STATE", WaterState)

---What Lara's hands are doing.
---@enum trx.lara.GunState
local GunState = {
  ---Empty-handed.
  ARMLESS = h.IntegerConstant,
  ---Hands full, so nothing can be drawn.
  HANDS_BUSY = h.IntegerConstant,
  ---Drawing a weapon.
  DRAW = h.IntegerConstant,
  ---Putting one away.
  UNDRAW = h.IntegerConstant,
  ---Armed, weapon out.
  READY = h.IntegerConstant,
  ---In a scripted sequence.
  SPECIAL = h.IntegerConstant,
}
M.GunState = h.enum("lara.GunState", "LARA_GUN_STATE", GunState)

-- The fields of LARA_INFO, named for scripts. The C side says where each member
-- lives (see src/trx/game/lara/fields.c); this says which of them exist.

---Lara's own state, reachable straight off `trx.lara`.
---@class (exact) trx.lara.Lara
---@trx.readonly back_gun, death_timer, dive_timer, equipped_gun, extra_anim,
---  flare_control, gun_status, head_rot, hit_direction, holsters_gun,
---  interact_item_num, interact_move_count, is_climbing, is_crouched,
---  is_interact_moving, left_arm_anim_num, left_arm_frame_num, left_arm_rot,
---  move_angle, pose_count, requested_gun, right_arm_anim_num,
---  right_arm_frame_num, right_arm_rot, torso_rot, turn_rate, water_status
---@field air_bar integer Air remaining underwater, out of 1800. Runs down while she is under.
---@field exposure_bar integer Warmth remaining in the cold, out of `trx.rules.exposure.max`. Only moves in a level whose rooms carry the `trx.rooms.Room.damaging` flag.
---@field poison integer How poisoned Lara is, and 0 when she is not.
---@field poison_target integer The poison reservoir that drains into `trx.lara.poison` over time. TR4 only.
---@field electric integer How badly Lara is being electrocuted, and 0 when she is not.
---@field is_burning boolean Whether Lara is on fire. Setting it lights her or puts her out.
---@field move_angle trx.math.Angle The direction Lara moves in. It leaves `trx.lara.item` facing elsewhere while she sidesteps, backflips or swims sideways.
---@field turn_rate trx.math.Angle The angle by which Lara turns in each frame. It is 0 when she is not turning.
---@field head_rot trx.math.Rot The direction in which Lara's head points, relative to her torso. The engine sets it in each frame from where she looks and aims.
---@field torso_rot trx.math.Rot The direction in which Lara's torso points, relative to `trx.lara.item`. The engine sets it in each frame from where she looks and aims.
---@field is_crouched boolean Whether Lara is crouching.
---@field is_climbing boolean Whether Lara is on a climbable wall.
---@field water_status trx.lara.WaterState Where Lara is with respect to water.
---@field gun_status trx.lara.GunState What Lara's hands are doing.
---@field equipped_gun trx.catalog.weapons The weapon Lara is holding.
---@field requested_gun trx.catalog.weapons The weapon Lara is drawing, while she is drawing it.
---@field back_gun trx.catalog.weapons The weapon drawn on Lara's back.
---@field holsters_gun trx.catalog.weapons The weapon drawn in Lara's holsters.
---@field extra_anim boolean Whether a scripted animation is driving Lara rather than her own state machine.
---@field dive_timer trx.game.Frames How long Lara has been diving.
---@field death_timer trx.game.Frames How long Lara has been dead.
---@field sprint_timer integer Sprint left in her legs.
---@field hit_direction integer Which way the last hit came from, or -1 if she has not been hit.
---@field pose_count trx.game.Frames How long Lara has stood still, which is what starts an idle animation.
---@field left_arm_anim_num integer The animation Lara's left arm is playing, which follows the weapon in it rather than the rest of her.
---@field left_arm_frame_num integer The frame that animation is on.
---@field left_arm_rot trx.math.Rot The direction in which Lara's left arm aims, relative to her torso.
---@field right_arm_anim_num integer The animation Lara's right arm is playing.
---@field right_arm_frame_num integer The frame that animation is on.
---@field right_arm_rot trx.math.Rot The direction in which Lara's right arm aims, relative to her torso.
---@field flare_control boolean Whether the flare Lara holds is driving her arm.
---@field interact_item_num integer The item Lara is lining herself up with, by number, or -1 for none.
---@field interact_move_count integer How many frames she has spent moving into place for it.
---@field is_interact_moving boolean Whether Lara is still moving towards her interaction target.
local Lara = h.handle("lara.Lara", "LARA_INFO", {
  fields = {
    air_bar = "air",
    exposure_bar = "exposure_timer",
    poison = "poison.value",
    poison_target = "poison.target",
    electric = "electric",
    is_burning = "burn",
    move_angle = "move_angle",
    turn_rate = "turn_rate",
    head_rot = "head_rot",
    torso_rot = "torso_rot",
    is_crouched = "is_crouched",
    is_climbing = "climb_status",
    water_status = "water_status",
    gun_status = "gun_status",
    equipped_gun = "gun_type",
    requested_gun = "request_gun_type",
    back_gun = "back_gun_type",
    holsters_gun = "holsters_gun_type",
    extra_anim = "extra_anim",
    dive_timer = "dive_timer",
    death_timer = "death_timer",
    sprint_timer = "sprint_timer",
    hit_direction = "hit_direction",
    pose_count = "pose_count",
    left_arm_anim_num = "left_arm.anim_num",
    left_arm_frame_num = "left_arm.frame_num",
    left_arm_rot = "left_arm.rot",
    right_arm_anim_num = "right_arm.anim_num",
    right_arm_frame_num = "right_arm.frame_num",
    right_arm_rot = "right_arm.rot",
    flare_control = "flare.control",
    interact_item_num = "interact_target.item_num",
    interact_move_count = "interact_target.move_count",
    is_interact_moving = "interact_target.is_moving",
  },
  writable = {
    "air_bar",
    "exposure_bar",
    "poison",
    "poison_target",
    "electric",
    "is_burning",
    "sprint_timer",
  },
})

---Lara's maximum air, which is what her air runs down from.
---@type integer
---@trx.value 1800
M.MAX_AIR = h.const("lara.MAX_AIR", trxc.lara.get_max_air())

---Lara's maximum sprint, which is what her sprint runs down from.
---@type integer
---@trx.value 120
M.MAX_SPRINT = h.const("lara.MAX_SPRINT", trxc.lara.get_max_sprint())

---The signals Lara's own state speaks through, for a script that would rather
---hear about a change than ask after one. Each is read once a frame and
---compared, so a listener runs when the value moved and a value that stood
---still costs nothing.
---
---What names an item is its number rather than the item itself, because a
---handle is made afresh on every read and a signal holding one would report a
---change every frame.
---@class (exact) trx.lara.signals
---@trx.readonly air, equipped_gun, exists, exposure, gun_status, hp,
---  is_controllable, max_hp, poison, room_num, sprint, target, vehicle,
---  water_status
---@field exists trx.signal.Signal Says when Lara enters the world, and when she leaves it.
---@field hp trx.signal.Signal Says when Lara's hit points change.
---@field max_hp trx.signal.Signal Says when Lara's maximum hit points change.
---@field poison trx.signal.Signal Says when Lara's poison value changes.
---@field air trx.signal.Signal Says when the air Lara has left underwater changes.
---@field sprint trx.signal.Signal Says when the sprint Lara has left changes.
---@field exposure trx.signal.Signal Says when the warmth Lara has left in the cold changes.
---@field gun_status trx.signal.Signal Says when Lara draws a weapon or puts one away.
---@field water_status trx.signal.Signal Says when Lara enters or leaves the water.
---@field room_num trx.signal.Signal Says when Lara changes rooms. Read `trx.lara.item.room` for the room itself.
---@field is_controllable trx.signal.Signal Says when Lara stops answering to the player, or starts again.
---@field target trx.signal.Signal Says when what Lara's guns are locked onto changes. Read `trx.lara.target` for the item itself.
---@field vehicle trx.signal.Signal Says when Lara gets on or off a vehicle. Read `trx.lara.vehicle` for it.
---@field equipped_gun trx.signal.Signal Says when Lara changes weapon.
M.signals = h.namespace("lara.signals")

-- What a signal carries is a number, a string or nothing, never a handle: a
-- handle is made afresh on every read, so a signal holding one would report a
-- change every frame. A script woken by one of these reads the handle itself.
local SIGNALS = {
  {
    "exists",
    function()
      return trx.lara.item ~= nil
    end,
  },
  {
    "hp",
    function()
      local item = trx.lara.item
      return item ~= nil and item.hit_points or nil
    end,
  },
  {
    "max_hp",
    function()
      local item = trx.lara.item
      return item ~= nil and item.max_hit_points or nil
    end,
  },
  {
    "poison",
    function()
      return trx.lara.poison
    end,
  },
  {
    "air",
    function()
      return trx.lara.air_bar
    end,
  },
  {
    "sprint",
    function()
      return trx.lara.sprint_timer
    end,
  },
  {
    "exposure",
    function()
      return trx.lara.exposure_bar
    end,
  },
  {
    "gun_status",
    function()
      return trx.lara.gun_status
    end,
  },
  {
    "water_status",
    function()
      return trx.lara.water_status
    end,
  },
  {
    "room_num",
    function()
      local item = trx.lara.item
      return item ~= nil and item.room_num or nil
    end,
  },
  {
    "is_controllable",
    function()
      return trx.lara.is_controllable
    end,
  },
  {
    "target",
    function()
      local target = trx.lara.target
      return target ~= nil and target.num or nil
    end,
  },
  {
    "vehicle",
    function()
      local vehicle = trx.lara.vehicle
      return vehicle ~= nil and vehicle.num or nil
    end,
  },
  {
    "equipped_gun",
    function()
      return trx.lara.equipped_gun
    end,
  },
}

local signal_props = {}
for _, entry in ipairs(SIGNALS) do
  local name, read = entry[1], entry[2]
  local held = nil
  signal_props[name] = {
    get = function()
      if held == nil then
        held = trx.signal.polled(read)
      end
      return held
    end,
  }
end
h.properties(M.signals, "lara.signals", signal_props)

h.properties(M, "lara", {
  can_pose = { get = raw.can_pose },
  animation_object = {
    get = function()
      return raw.get_animation_object()
    end,
  },
  item = {
    get = function()
      return trx.items[raw.get_item()]
    end,
  },
  target = {
    get = function()
      local target = raw.get_target()
      return target ~= nil and trx.items[target] or nil
    end,
  },
  vehicle = {
    get = function()
      local vehicle = raw.get_vehicle()
      return vehicle ~= nil and trx.items[vehicle] or nil
    end,
  },
  is_controllable = { get = raw.is_controllable },
  outfit = { get = raw.get_outfit, set = raw.set_outfit },
  holsters_visible = {
    get = raw.are_holsters_visible,
    set = raw.set_holsters_visible,
  },
  speech_face = {
    get = function()
      local index = raw.get_speech_face()
      return index >= 0 and index or nil
    end,
    set = function(index)
      raw.set_speech_face(index)
    end,
  },
  is_flying = { get = raw.is_flying, set = raw.set_flying },
  is_wet = { get = raw.is_wet },
  vehicle_gun = { get = raw.vehicle_gun },
  has_pistol_weapon = { get = raw.has_pistol_weapon },
})

---Hangs an extra mesh on one of Lara's own, replacing the mesh there.
---
---```lua
---trx.lara.set_extra_equipment(trx.lara.Mesh.HAND_R, trx.lara.ExtraMesh.OAR)
---```
---@param mesh trx.lara.Mesh Which of Lara's meshes.
---@param extra_mesh trx.lara.ExtraMesh The mesh to hang on it.
---@type fun(mesh: trx.lara.Mesh, extra_mesh: trx.lara.ExtraMesh)
M.set_extra_equipment = raw.set_extra_equipment

---Moves Lara to a world position, putting her down on the floor there. She is
---taken off any vehicle, her weapons are put away and the camera follows her
---over.
---
---The position is nudged into valid room geometry, so a spot inside a wall
---lands her beside it rather than in it. Somewhere with no floor within reach
---moves nothing.
---
---```lua
---trx.lara.teleport(trx.items.query:of_object("wolf"):first().pos)
---```
---@param pos trx.math.Vec3 World position.
---@param room_num? trx.rooms.Num Without it, the room is found from the position.
---@return boolean # Whether she was moved.
---@type fun(pos: trx.math.Vec3, room_num?: trx.rooms.Num): boolean
M.teleport = raw.teleport

---Cures Lara's poisoning. Not the same as writing `0` to `trx.lara.poison`:
---the poison has a target as well as a current value, and clearing only the
---value lets it climb back.
---@type fun()
M.cure_poison = raw.cure_poison

---Puts Lara's fire out, and stops her being electrocuted with it.
---@type fun()
M.extinguish = raw.extinguish

---Dries Lara off, clearing the wetness that sheds droplets after she leaves
---water.
---@type fun()
M.dry = raw.dry

---Puts another object's mesh on one of Lara's own, in place of whatever her
---outfit gives her there.
---
---It outlives an outfit change, because applying an outfit reads it, which is
---what lets a level dress her from its own geometry rather than from the
---outfit. Her head is the exception: a combat or speech face replaces it
---directly, and takes it back from an override with it.
---
---The override is dropped when the level ends, along with the meshes it could
---name.
---
---```lua
----- the torso young Lara wears before she picks up her backpack
---trx.lara.set_mesh(trx.lara.Mesh.TORSO, trx.catalog.objects.lara_skin, 7)
---```
---@param mesh trx.lara.Mesh Which of Lara's meshes.
---@param object trx.catalog.Id The object to take a mesh from. Raises if this level does not carry it.
---@param mesh_num trx.objects.MeshNum Which of that object's meshes.
---@type fun(mesh: trx.lara.Mesh, object: trx.catalog.Id, mesh_num: trx.objects.MeshNum)
M.set_mesh = raw.set_mesh

---Takes the override back off, leaving the mesh Lara's outfit gives her.
---@param mesh trx.lara.Mesh Which of Lara's meshes.
---@type fun(mesh: trx.lara.Mesh)
M.clear_mesh = raw.clear_mesh

---Takes the extra mesh back off, leaving Lara's own.
---@param mesh trx.lara.Mesh Which of Lara's meshes.
---@type fun(mesh: trx.lara.Mesh)
M.clear_equipment = raw.clear_equipment

h.instance(M, "lara", raw.state)

local _ = Lara
