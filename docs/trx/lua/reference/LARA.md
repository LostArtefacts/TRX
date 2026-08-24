---
title: Lara
order: 3
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/lara.lua. Edit it there.
-->

## <a id="lara" name="lara"></a>Lara module

Module for reading and nudging Lara's own state.

Her position, room and hit points are not here: she is an item like any other and they live on it, as [`trx.lara.item`](#lara.item).

### Properties

- <a id="lara.animation_object" name="lara.animation_object"></a>**`trx.lara.animation_object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The object Lara's animations are coming from. It is normally Lara herself, and something else while a vehicle or a scripted sequence drives her. *(read-only)*
- <a id="lara.item" name="lara.item"></a>**`trx.lara.item`** ([trx.items.Item](ITEMS.md#items.Item)). Lara's own item, or `nil` outside a level. Her position, room and hit points are read and written there. *(read-only)*
- <a id="lara.target" name="lara.target"></a>**`trx.lara.target`** ([trx.items.Item](ITEMS.md#items.Item)). The item Lara's guns are locked onto, or `nil` if she has none. *(read-only)*
- <a id="lara.signals.exists" name="lara.signals.exists"></a>**`trx.lara.signals.exists`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when Lara enters the world, and when she leaves it. *(read-only)*
- <a id="lara.signals.hp" name="lara.signals.hp"></a>**`trx.lara.signals.hp`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when Lara's hit points change. *(read-only)*
- <a id="lara.signals.max_hp" name="lara.signals.max_hp"></a>**`trx.lara.signals.max_hp`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when Lara's maximum hit points change. *(read-only)*
- <a id="lara.signals.poison" name="lara.signals.poison"></a>**`trx.lara.signals.poison`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when Lara's poison value changes. *(read-only)*
- <a id="lara.signals.air" name="lara.signals.air"></a>**`trx.lara.signals.air`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when the air Lara has left underwater changes. *(read-only)*
- <a id="lara.signals.sprint" name="lara.signals.sprint"></a>**`trx.lara.signals.sprint`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when the sprint Lara has left changes. *(read-only)*
- <a id="lara.signals.exposure" name="lara.signals.exposure"></a>**`trx.lara.signals.exposure`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when the warmth Lara has left in the cold changes. *(read-only)*
- <a id="lara.signals.gun_status" name="lara.signals.gun_status"></a>**`trx.lara.signals.gun_status`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when Lara draws a weapon or puts one away. *(read-only)*
- <a id="lara.signals.water_status" name="lara.signals.water_status"></a>**`trx.lara.signals.water_status`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when Lara enters or leaves the water. *(read-only)*
- <a id="lara.signals.room_num" name="lara.signals.room_num"></a>**`trx.lara.signals.room_num`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when Lara changes rooms. Read [`trx.lara.item.room`](ITEMS.md#items.Item.room) for the room itself. *(read-only)*
- <a id="lara.signals.is_controllable" name="lara.signals.is_controllable"></a>**`trx.lara.signals.is_controllable`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when Lara stops answering to the player, or starts again. *(read-only)*
- <a id="lara.signals.target" name="lara.signals.target"></a>**`trx.lara.signals.target`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when what Lara's guns are locked onto changes. Read [`trx.lara.target`](#lara.target) for the item itself. *(read-only)*
- <a id="lara.signals.vehicle" name="lara.signals.vehicle"></a>**`trx.lara.signals.vehicle`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when Lara gets on or off a vehicle. Read [`trx.lara.vehicle`](#lara.vehicle) for it. *(read-only)*
- <a id="lara.signals.equipped_gun" name="lara.signals.equipped_gun"></a>**`trx.lara.signals.equipped_gun`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when Lara changes weapon. *(read-only)*
- <a id="lara.vehicle" name="lara.vehicle"></a>**`trx.lara.vehicle`** ([trx.items.Item](ITEMS.md#items.Item)). The vehicle Lara is riding, or `nil` when she is on her own feet. Its speed and position are the ones that move her while she rides it. *(read-only)*
- <a id="lara.is_controllable" name="lara.is_controllable"></a>**`trx.lara.is_controllable`** (boolean). Whether Lara answers to the player. False while she is dead, while the inventory or a dialog holds the game, and while a cutscene or flyby is active. *(read-only)*
- <a id="lara.outfit" name="lara.outfit"></a>**`trx.lara.outfit`** (string). The outfit Lara is wearing, by name, as defined in `cfg/outfits.json5`.
- <a id="lara.holsters_visible" name="lara.holsters_visible"></a>**`trx.lara.holsters_visible`** (boolean). Whether Lara's holsters are drawn on her hips.
- <a id="lara.speech_face" name="lara.speech_face"></a>**`trx.lara.speech_face`** (number). Which of her outfit's speech faces Lara wears while she talks, counted from 0, or `nil` for her own face. An outfit with no speech faces keeps her own.
  The face is remembered, so putting her in another outfit mid-sentence dresses her in that outfit's face rather than leaving the one she had.
- <a id="lara.is_flying" name="lara.is_flying"></a>**`trx.lara.is_flying`** (boolean). Whether Lara is in the fly-mode cheat. Setting it enters or leaves fly mode.
- <a id="lara.is_wet" name="lara.is_wet"></a>**`trx.lara.is_wet`** (boolean). Whether Lara is still shedding droplets after a swim. [`trx.lara.dry`](#lara.dry) clears it. *(read-only)*
- <a id="lara.vehicle_gun" name="lara.vehicle_gun"></a>**`trx.lara.vehicle_gun`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). The weapon the vehicle Lara is riding carries. Her own weapons are put away while she rides, so this is what her ammunition counter shows. `nil` where she is riding nothing, or riding something unarmed. *(read-only)*
- <a id="lara.has_pistol_weapon" name="lara.has_pistol_weapon"></a>**`trx.lara.has_pistol_weapon`** (boolean). Whether Lara is carrying a pistol-class weapon, which is what decides whether she has holsters to show at all. *(read-only)*

### Constants

- <a id="lara.MAX_AIR" name="lara.MAX_AIR"></a>[lua]`trx.lara.MAX_AIR` = `1800`  
  Lara's maximum air, which is what her air runs down from.

- <a id="lara.MAX_SPRINT" name="lara.MAX_SPRINT"></a>[lua]`trx.lara.MAX_SPRINT` = `120`  
  Lara's maximum sprint, which is what her sprint runs down from.

### Enums

- <a id="lara.Mesh" name="lara.Mesh"></a>[lua]`trx.lara.Mesh`

    One of the fifteen meshes Lara is built from.

    - `trx.lara.Mesh.HIPS` = `0`  
        Hips, the mesh the rest hang off.
    - `trx.lara.Mesh.THIGH_L` = `1`  
        Left thigh.
    - `trx.lara.Mesh.CALF_L` = `2`  
        Left calf.
    - `trx.lara.Mesh.FOOT_L` = `3`  
        Left foot.
    - `trx.lara.Mesh.THIGH_R` = `4`  
        Right thigh.
    - `trx.lara.Mesh.CALF_R` = `5`  
        Right calf.
    - `trx.lara.Mesh.FOOT_R` = `6`  
        Right foot.
    - `trx.lara.Mesh.TORSO` = `7`  
        Torso.
    - `trx.lara.Mesh.UARM_R` = `8`  
        Right upper arm.
    - `trx.lara.Mesh.LARM_R` = `9`  
        Right lower arm.
    - `trx.lara.Mesh.HAND_R` = `10`  
        Right hand.
    - `trx.lara.Mesh.UARM_L` = `11`  
        Left upper arm.
    - `trx.lara.Mesh.LARM_L` = `12`  
        Left lower arm.
    - `trx.lara.Mesh.HAND_L` = `13`  
        Left hand.
    - `trx.lara.Mesh.HEAD` = `14`  
        Head.

- <a id="lara.ExtraMesh" name="lara.ExtraMesh"></a>[lua]`trx.lara.ExtraMesh`

    A mesh Lara can carry on top of one of her own - the dagger in Home Sweet Home, the oar in a boat.

    - `trx.lara.ExtraMesh.TR1_BRAID_DEFAULT_HEAD` = `0`  
        Braided head, out of combat.
    - `trx.lara.ExtraMesh.TR1_BRAID_COMBAT_HEAD` = `1`  
        Braided head, in combat.
    - `trx.lara.ExtraMesh.TR1_BRAID_DEFAULT_TORSO` = `2`  
        Braided torso.
    - `trx.lara.ExtraMesh.TR1_BRAID_MAULED_TORSO` = `3`  
        Braided torso, mauled.
    - `trx.lara.ExtraMesh.DAGGER_HAND` = `4`  
        Dagger, in hand.
    - `trx.lara.ExtraMesh.DAGGER_HIPS` = `5`  
        Dagger, sheathed at the hips.
    - `trx.lara.ExtraMesh.OAR` = `6`  
        Oar.
    - `trx.lara.ExtraMesh.SPANNER` = `7`  
        Spanner.
    - `trx.lara.ExtraMesh.DRINK_CAN` = `8`  
        Drink can.
    - `trx.lara.ExtraMesh.GLASSES_OPAQUE` = `9`  
        Sunglasses.
    - `trx.lara.ExtraMesh.GLASSES_TRANSPARENT` = `10`  
        Sunglasses, transparent lenses.
    - `trx.lara.ExtraMesh.CROWBAR` = `11`  
        Crowbar.
    - `trx.lara.ExtraMesh.WOODEN_TORCH` = `12`  
        Wooden torch.
    - `trx.lara.ExtraMesh.BINOCULARS` = `13`  
        Binoculars.
    - `trx.lara.ExtraMesh.HOOK_AND_POLE` = `14`  
        Hook and pole.
    - `trx.lara.ExtraMesh.DETONATOR` = `15`  
        Detonator.
    - `trx.lara.ExtraMesh.SHOVEL` = `16`  
        Shovel.
    - `trx.lara.ExtraMesh.JERRYCAN` = `17`  
        Jerrycan.
    - `trx.lara.ExtraMesh.SANDBAG` = `18`  
        Sandbag.
    - `trx.lara.ExtraMesh.WATERSKIN` = `19`  
        Waterskin.

- <a id="lara.WaterState" name="lara.WaterState"></a>[lua]`trx.lara.WaterState`

    Where Lara is with respect to water.

    - `trx.lara.WaterState.ABOVE_WATER` = `0`  
        On dry land.
    - `trx.lara.WaterState.UNDERWATER` = `1`  
        Under the surface.
    - `trx.lara.WaterState.SURFACE` = `2`  
        Swimming at the surface.
    - `trx.lara.WaterState.CHEAT` = `3`  
        Flying, as the fly cheat leaves her.
    - `trx.lara.WaterState.WADE` = `4`  
        Wading, feet still on the floor.

- <a id="lara.GunState" name="lara.GunState"></a>[lua]`trx.lara.GunState`

    What Lara's hands are doing.

    - `trx.lara.GunState.ARMLESS` = `0`  
        Empty-handed.
    - `trx.lara.GunState.HANDS_BUSY` = `1`  
        Hands full, so nothing can be drawn.
    - `trx.lara.GunState.DRAW` = `2`  
        Drawing a weapon.
    - `trx.lara.GunState.UNDRAW` = `3`  
        Putting one away.
    - `trx.lara.GunState.READY` = `4`  
        Armed, weapon out.
    - `trx.lara.GunState.SPECIAL` = `5`  
        In a scripted sequence.

### Structures

- <a id="lara.Lara" name="lara.Lara"></a>[lua]`trx.lara.Lara`

    Lara's own state, reachable straight off [`trx.lara`](#lara).

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="lara.Lara.air_bar" name="lara.Lara.air_bar"></a>**`air_bar`**: integer. Air remaining underwater, out of 1800. Runs down while she is under.
    - <a id="lara.Lara.death_timer" name="lara.Lara.death_timer"></a>**`death_timer`**: [trx.game.Frames](GAME.md#game.Frames). How long Lara has been dead. *(read-only)*
    - <a id="lara.Lara.dive_timer" name="lara.Lara.dive_timer"></a>**`dive_timer`**: [trx.game.Frames](GAME.md#game.Frames). How long Lara has been diving. *(read-only)*
    - <a id="lara.Lara.electric" name="lara.Lara.electric"></a>**`electric`**: integer. How badly Lara is being electrocuted, and 0 when she is not.
    - <a id="lara.Lara.equipped_gun" name="lara.Lara.equipped_gun"></a>**`equipped_gun`**: [trx.catalog.weapons](CATALOG.md#catalog.weapons). The weapon Lara is holding. *(read-only)*
    - <a id="lara.Lara.exposure_bar" name="lara.Lara.exposure_bar"></a>**`exposure_bar`**: integer. Warmth remaining in the cold, out of [`trx.rules.exposure.max`](RULES.md#rules.exposure.max). Only moves in a level whose rooms carry the [`trx.rooms.Room.damaging`](ROOMS.md#rooms.Room.damaging) flag.
    - <a id="lara.Lara.extra_anim" name="lara.Lara.extra_anim"></a>**`extra_anim`**: boolean. Whether a scripted animation is driving Lara rather than her own state machine. *(read-only)*
    - <a id="lara.Lara.flare_control" name="lara.Lara.flare_control"></a>**`flare_control`**: boolean. Whether the flare Lara holds is driving her arm. *(read-only)*
    - <a id="lara.Lara.gun_status" name="lara.Lara.gun_status"></a>**`gun_status`**: [trx.lara.GunState](#lara.GunState). What Lara's hands are doing. *(read-only)*
    - <a id="lara.Lara.hit_direction" name="lara.Lara.hit_direction"></a>**`hit_direction`**: integer. Which way the last hit came from, or -1 if she has not been hit. *(read-only)*
    - <a id="lara.Lara.interact_item_num" name="lara.Lara.interact_item_num"></a>**`interact_item_num`**: integer. The item Lara is lining herself up with, by number, or -1 for none. *(read-only)*
    - <a id="lara.Lara.interact_move_count" name="lara.Lara.interact_move_count"></a>**`interact_move_count`**: integer. How many frames she has spent moving into place for it. *(read-only)*
    - <a id="lara.Lara.is_burning" name="lara.Lara.is_burning"></a>**`is_burning`**: boolean. Whether Lara is on fire. Setting it lights her or puts her out.
    - <a id="lara.Lara.is_climbing" name="lara.Lara.is_climbing"></a>**`is_climbing`**: boolean. Whether Lara is on a climbable wall. *(read-only)*
    - <a id="lara.Lara.is_crouched" name="lara.Lara.is_crouched"></a>**`is_crouched`**: boolean. Whether Lara is crouching. *(read-only)*
    - <a id="lara.Lara.is_interact_moving" name="lara.Lara.is_interact_moving"></a>**`is_interact_moving`**: boolean. Whether Lara is still moving towards her interaction target. *(read-only)*
    - <a id="lara.Lara.left_arm_anim_num" name="lara.Lara.left_arm_anim_num"></a>**`left_arm_anim_num`**: integer. The animation Lara's left arm is playing, which follows the weapon in it rather than the rest of her. *(read-only)*
    - <a id="lara.Lara.left_arm_frame_num" name="lara.Lara.left_arm_frame_num"></a>**`left_arm_frame_num`**: integer. The frame that animation is on. *(read-only)*
    - <a id="lara.Lara.poison" name="lara.Lara.poison"></a>**`poison`**: integer. How poisoned Lara is, and 0 when she is not.
    - <a id="lara.Lara.poison_target" name="lara.Lara.poison_target"></a>**`poison_target`**: integer. The poison reservoir that drains into [`trx.lara.poison`](#lara.Lara.poison) over time. TR4 only.
    - <a id="lara.Lara.pose_count" name="lara.Lara.pose_count"></a>**`pose_count`**: [trx.game.Frames](GAME.md#game.Frames). How long Lara has stood still, which is what starts an idle animation. *(read-only)*
    - <a id="lara.Lara.requested_gun" name="lara.Lara.requested_gun"></a>**`requested_gun`**: [trx.catalog.weapons](CATALOG.md#catalog.weapons). The weapon Lara is drawing, while she is drawing it. *(read-only)*
    - <a id="lara.Lara.right_arm_anim_num" name="lara.Lara.right_arm_anim_num"></a>**`right_arm_anim_num`**: integer. The animation Lara's right arm is playing. *(read-only)*
    - <a id="lara.Lara.right_arm_frame_num" name="lara.Lara.right_arm_frame_num"></a>**`right_arm_frame_num`**: integer. The frame that animation is on. *(read-only)*
    - <a id="lara.Lara.sprint_timer" name="lara.Lara.sprint_timer"></a>**`sprint_timer`**: integer. Sprint left in her legs.
    - <a id="lara.Lara.water_status" name="lara.Lara.water_status"></a>**`water_status`**: [trx.lara.WaterState](#lara.WaterState). Where Lara is with respect to water. *(read-only)*

### Functions

- <a id="lara.signals" name="lara.signals"></a>[lua]`trx.lara.signals`  
  The signals Lara's own state speaks through, for a script that would rather hear about a change than ask after one. Each is read once a frame and compared, so a listener runs when the value moved and a value that stood still costs nothing.

  What names an item is its number rather than the item itself, because a handle is made afresh on every read and a signal holding one would report a change every frame.

- <a id="lara.set_extra_equipment" name="lara.set_extra_equipment"></a>[lua]`trx.lara.set_extra_equipment(mesh, extra_mesh)`  
  Hangs an extra mesh on one of Lara's own, replacing the mesh there.

  Parameters:
  - <a id="lara.set_extra_equipment.mesh" name="lara.set_extra_equipment.mesh"></a>**`mesh`** ([trx.lara.Mesh](#lara.Mesh)). Which of Lara's meshes.
  - <a id="lara.set_extra_equipment.extra_mesh" name="lara.set_extra_equipment.extra_mesh"></a>**`extra_mesh`** ([trx.lara.ExtraMesh](#lara.ExtraMesh)). The mesh to hang on it.

  Example:
  ```lua
  trx.lara.set_extra_equipment(trx.lara.Mesh.HAND_R, trx.lara.ExtraMesh.OAR)
  ```

- <a id="lara.teleport" name="lara.teleport"></a>[lua]`trx.lara.teleport(pos, [room_num])`  
  Moves Lara to a world position, putting her down on the floor there. She is taken off any vehicle, her weapons are put away and the camera follows her over.

  The position is nudged into valid room geometry, so a spot inside a wall lands her beside it rather than in it. Somewhere with no floor within reach moves nothing.

  Parameters:
  - <a id="lara.teleport.pos" name="lara.teleport.pos"></a>**`pos`** ([trx.math.Vec3](MATH.md#math.Vec3)). World position.
  - <a id="lara.teleport.room_num" name="lara.teleport.room_num"></a>**`room_num`** ([trx.rooms.Num](ROOMS.md#rooms.Num), optional). Without it, the room is found from the position.

  Returns: boolean. Whether she was moved.

  Example:
  ```lua
  trx.lara.teleport(trx.items.query:of_object("wolf"):first().pos)
  ```

- <a id="lara.cure_poison" name="lara.cure_poison"></a>[lua]`trx.lara.cure_poison()`  
  Cures Lara's poisoning. Not the same as writing `0` to [`trx.lara.poison`](#lara.Lara.poison): the poison has a target as well as a current value, and clearing only the value lets it climb back.

- <a id="lara.extinguish" name="lara.extinguish"></a>[lua]`trx.lara.extinguish()`  
  Puts Lara's fire out, and stops her being electrocuted with it.

- <a id="lara.dry" name="lara.dry"></a>[lua]`trx.lara.dry()`  
  Dries Lara off, clearing the wetness that sheds droplets after she leaves water.

- <a id="lara.set_mesh" name="lara.set_mesh"></a>[lua]`trx.lara.set_mesh(mesh, object, mesh_num)`  
  Puts another object's mesh on one of Lara's own, in place of whatever her
  outfit gives her there.

  It outlives an outfit change, because applying an outfit reads it, which is
  what lets a level dress her from its own geometry rather than from the
  outfit. Her head is the exception: a combat or speech face replaces it
  directly, and takes it back from an override with it.

  The override is dropped when the level ends, along with the meshes it could
  name.

  Parameters:
  - <a id="lara.set_mesh.mesh" name="lara.set_mesh.mesh"></a>**`mesh`** ([trx.lara.Mesh](#lara.Mesh)). Which of Lara's meshes.
  - <a id="lara.set_mesh.object" name="lara.set_mesh.object"></a>**`object`** ([trx.catalog.Id](CATALOG.md#catalog.Id)). The object to take a mesh from. Raises if this level does not carry it.
  - <a id="lara.set_mesh.mesh_num" name="lara.set_mesh.mesh_num"></a>**`mesh_num`** ([trx.objects.MeshNum](OBJECTS.md#objects.MeshNum)). Which of that object's meshes.

  Example:
  ```lua
  -- the torso young Lara wears before she picks up her backpack
  trx.lara.set_mesh(trx.lara.Mesh.TORSO, trx.catalog.objects.lara_skin, 7)
  ```

- <a id="lara.clear_mesh" name="lara.clear_mesh"></a>[lua]`trx.lara.clear_mesh(mesh)`  
  Takes the override back off, leaving the mesh Lara's outfit gives her.

  Parameters:
  - <a id="lara.clear_mesh.mesh" name="lara.clear_mesh.mesh"></a>**`mesh`** ([trx.lara.Mesh](#lara.Mesh)). Which of Lara's meshes.

- <a id="lara.clear_equipment" name="lara.clear_equipment"></a>[lua]`trx.lara.clear_equipment(mesh)`  
  Takes the extra mesh back off, leaving Lara's own.

  Parameters:
  - <a id="lara.clear_equipment.mesh" name="lara.clear_equipment.mesh"></a>**`mesh`** ([trx.lara.Mesh](#lara.Mesh)). Which of Lara's meshes.
