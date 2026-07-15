---
title: Lara
order: 3
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  data/scripting/lara.lua. Edit it there.
-->

## Lara module

Module for reading and nudging Lara's own state.

Her position, room and hit points are not here: she is an item like any other and they live on it, as `trx.lara.item`.

### Properties

- **`trx.lara.item`** (Item). Lara's own `trx.items.Item`, or `nil` outside a level. Her position, room and hit points are read and written there. *(read-only)*
- **`trx.lara.target`** (Item). The item Lara's guns are locked onto, or `nil` if she has none. *(read-only)*
- **`trx.lara.outfit`** (string). The outfit Lara is wearing, by name, as defined in `cfg/outfits.json5`.
- **`trx.lara.holsters_visible`** (boolean). Whether Lara's holsters are drawn on her hips.
- **`trx.lara.has_pistol_weapon`** (boolean). Whether Lara is carrying a pistol-class weapon, which is what decides whether she has holsters to show at all. *(read-only)*

### Enums

- [lua]`trx.lara.Mesh`

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

- [lua]`trx.lara.ExtraMesh`

    A mesh Lara can carry on top of one of her own - the dagger in Home Sweet Home, the oar in a boat.

    - `trx.lara.ExtraMesh.TR1_BRAID_DEFAULT_HEAD` = `0`  
        Braided head, out of combat.
    - `trx.lara.ExtraMesh.TR1_BRAID_COMBAT_HEAD` = `1`  
        Braided head, in combat.
    - `trx.lara.ExtraMesh.TR1_BRAID_DEFAULT_TORSO` = `2`  
        Braided torso.
    - `trx.lara.ExtraMesh.TR1_BRAID_MAULED_TORSO` = `3`  
        Braided torso, mauled.
    - `trx.lara.ExtraMesh.TR1_BRAID_GOLD_HEAD` = `4`  
        Braided head, gold.
    - `trx.lara.ExtraMesh.TR1_BRAID_GOLD_TORSO` = `5`  
        Braided torso, gold.
    - `trx.lara.ExtraMesh.DAGGER_HAND` = `6`  
        Dagger, in hand.
    - `trx.lara.ExtraMesh.DAGGER_HIPS` = `7`  
        Dagger, sheathed at the hips.
    - `trx.lara.ExtraMesh.OAR` = `8`  
        Oar.
    - `trx.lara.ExtraMesh.SPANNER` = `9`  
        Spanner.
    - `trx.lara.ExtraMesh.DRINK_CAN` = `10`  
        Drink can.
    - `trx.lara.ExtraMesh.GLASSES_OPAQUE` = `11`  
        Sunglasses.
    - `trx.lara.ExtraMesh.GLASSES_TRANSPARENT` = `12`  
        Sunglasses, transparent lenses.
    - `trx.lara.ExtraMesh.CROWBAR` = `13`  
        Crowbar.
    - `trx.lara.ExtraMesh.WOODEN_TORCH` = `14`  
        Wooden torch.
    - `trx.lara.ExtraMesh.BINOCULARS` = `15`  
        Binoculars.
    - `trx.lara.ExtraMesh.HOOK_AND_POLE` = `16`  
        Hook and pole.
    - `trx.lara.ExtraMesh.DETONATOR` = `17`  
        Detonator.
    - `trx.lara.ExtraMesh.SHOVEL` = `18`  
        Shovel.
    - `trx.lara.ExtraMesh.JERRYCAN` = `19`  
        Jerrycan.
    - `trx.lara.ExtraMesh.SANDBAG` = `20`  
        Sandbag.
    - `trx.lara.ExtraMesh.WATERSKIN` = `21`  
        Waterskin.

- [lua]`trx.lara.WaterState`

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

- [lua]`trx.lara.GunState`

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

- [lua]`trx.lara.Lara`

    Lara's own state, reachable straight off `trx.lara`.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`air_bar`**: integer. Air remaining underwater, out of 1800. Runs down while she is under.
    - **`death_timer`**: integer. Frames Lara has been dead for. *(read-only)*
    - **`dive_timer`**: integer. Frames Lara has been diving for. *(read-only)*
    - **`electric`**: integer. How badly Lara is being electrocuted, and 0 when she is not.
    - **`equipped_gun`**: integer. The weapon Lara is holding. Compare against `trx.catalog.weapons`. *(read-only)*
    - **`exposure_bar`**: integer. Warmth remaining in the cold, out of 600. Only moves in a level whose rooms carry the `damaging` flag.
    - **`extra_anim`**: boolean. Whether a scripted animation is driving Lara rather than her own state machine. *(read-only)*
    - **`gun_status`**: integer. What Lara's hands are doing. Compare against `trx.lara.GunState`. *(read-only)*
    - **`hit_direction`**: integer. Which way the last hit came from, or -1 if she has not been hit. *(read-only)*
    - **`is_burning`**: boolean. Whether Lara is on fire. *(read-only)*
    - **`is_climbing`**: boolean. Whether Lara is on a climbable wall. *(read-only)*
    - **`is_crouched`**: boolean. Whether Lara is crouching. *(read-only)*
    - **`killed_loyal_item`**: boolean. Whether Lara has killed one of her own allies, which is what turns the rest of them on her. *(read-only)*
    - **`poison`**: integer. How poisoned Lara is, and 0 when she is not.
    - **`pose_count`**: integer. Frames Lara has stood still for, which is what starts an idle animation. *(read-only)*
    - **`requested_gun`**: integer. The weapon Lara is drawing, while she is drawing it. Compare against `trx.catalog.weapons`. *(read-only)*
    - **`sprint_timer`**: integer. Sprint left in her legs.
    - **`water_status`**: integer. Where Lara is with respect to water. Compare against `trx.lara.WaterState`. *(read-only)*

### Functions

- [lua]`trx.lara.set_extra_equipment(mesh, extra_mesh)`  
  Hangs an extra mesh on one of Lara's own, replacing whatever is there.

  Parameters:
  - **`mesh`** (integer). Which of Lara's meshes. Compare against `trx.lara.Mesh`.
  - **`extra_mesh`** (integer). The mesh to hang on it. Compare against `trx.lara.ExtraMesh`.

  Example:
  ```lua
  trx.lara.set_extra_equipment(trx.lara.Mesh.HAND_R, trx.lara.ExtraMesh.OAR)
  ```

- [lua]`trx.lara.clear_equipment(mesh)`  
  Takes the extra mesh back off, leaving Lara's own.

  Parameters:
  - **`mesh`** (integer). Which of Lara's meshes. Compare against `trx.lara.Mesh`.
