---
title: Injections
---

## Injections

Injections defined in the global game flow will by default be applied to each
level. Individual levels can also specify their own specific injections to
include. In that case, the global injections are merged with the level's when
the level loads.

Individual levels can set `inherit_injections` to `false` and as a result they
will not use the global injection files. If those levels have their own
injections defined, only those will be used. And of course, if they have none
defined, nothing will be injected.

_Disclaimer: Custom levels should not use the injections mechanism and instead
should provide data that is already correct and consistent. Reports of bugs
about injection files not working for custom levels will not be considered. Note
however that the injections that relate to Lara can work in custom levels
that do not modify Lara's default mesh structure or animations. These injection
files are based on the original Lara model._

The game flow will ignore referenced injection files that do not exist, but it is
best practice to remove the references to maintain a clean game flow file.

Following is a summary of what each of the default injection files that are
provided with the game achieves.

<table>
  <tr valign="top" align="left">
    <th>Injection file</th>
    <th>Usage</th>
    <th>Purpose</th>
  </tr>
  <tr valign="top">
    <td>
      <code>*_cameras.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Injects positional adjustments for cameras that can otherwise cause visual
      issues, such as in Temple of the Cat.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>*_fd.bin</code>
    </td>
    <td>TR1, TR2</td>
    <td>
      Injects fixes for floor data issues in the original levels. Refer to the
      README for a full list of fixes.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>*_itemrots.bin</code>
    </td>
    <td>TR1, TR2</td>
    <td>
      Injects rotations on pickup items so they make more visual sense when
      using the 3D pickups option.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>*_meshfixes.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Injects miscellaneous mesh adjustments for objects, such as in Obelisk of
      Khamoon to avoid z-fighting.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>*_music_tracks.bin</code>
    </td>
    <td>TR2</td>
    <td>
      Injects trigger adjustments to convert music track numbers to match file
      names. These are suitable for OG levels only.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>*_pickup_meshes.bin</code>
    </td>
    <td>TR1, TR2</td>
    <td>
      Injects mesh edits to change the scale of various pickup models, such as
      the keys in St. Francis' Folly or the Prayer Wheel in Barkhang Monastry.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>*_sfx.bin</code>
    </td>
    <td>TR1, TR2</td>
    <td>
      Injects various SFX fixes or additions, such as the PSX Uzi SFX in TR1, or
      fixing the silent enemies in TR2's water levels.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>*_skybox.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Injects a predefined skybox model into specific levels.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>*_textures.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Injects fixes for texture issues in the original levels, such as gaps in
      the walls or wrongly colored models. Refer to the README for a full list
      of fixes.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>boat_bits.bin</code>
    </td>
    <td>TR2</td>
    <td>
      Injects a model in slot `O_BOAT_BITS` (221) which is used to show the boat
      exploding when it crosses mines.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>backpack.bin</code>
    </td>
    <td>TR1</td>
    <td rowspan="2">
      Injects mesh edits for Lara's backback, such that it becomes shallower.
      This is only applied when the braid is enabled, to avoid the braid
      merging with the backpack. The different files are needed to address mesh
      structure differences between cutscene and normal levels.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>backpack_cut.bin</code>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>braid.bin</code>
    </td>
    <td rowspan="4">TR1</td>
    <td rowspan="4">
      Injects a braid when the option for it is enabled. This also edits Lara's
      head meshes (object 0 and object 4) to make the braid fit better. A golden
      braid is also provided for the Midas Touch animation. Again, different
      files are needed to cater for mesh differences between cutscene and normal
      levels. The Lost Valley file comprises a head mesh edit for object 5 only.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>braid_cut1.bin</code>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>braid_cut2_cut4.bin</code>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>braid_valley.bin</code>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>bubbles.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Injects replacement sprite textures for Lara's underwater bubble sprites,
      which are cut off in OG.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>cistern_plants.bin</code>
    </td>
    <td>TR1</td>
    <td>
      This disables the animation on sprite ID 193 in The Cistern and Tomb of
      Tihocan.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>khamoon_mummy.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Injects the mummy in room 25 of City of Khamoon, which is present in the
      PS1 version but not the PC.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>lara_animations.bin</code>
    </td>
    <td>TR1, TR2</td>
    <td>
      Injects several animations, state changes and commands for Lara, such as
      responsive jumping, jump-twist, somersault, underwater roll, and wading.
      For custom levels, this injection file will work as-is, but if you are
      building a level and wish to customise any of Lara's animations, please
      refer to https://github.com/LostArtefacts/TRXInjectionTool/blob/main/docs/ASSETS.md
      for guidance.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>lara_gym_guns.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Injects all of Lara's weapons and weapon animations in TR1's gym level.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>explosion.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Injects explosion sprites for certain console commands.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>font.bin</code>
    </td>
    <td>TR1, TR2</td>
    <td>
      Injects replacement font sprites to support more characters than OG.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>guardian_death_commands.bin</code>
    </td>
    <td>TR2</td>
    <td>
      Injects an animation command for the bird guardian to end the level on the
      final frame of its death animation. The original hard-coded end-level
      behaviour is removed in TR2X.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>mines_pushblocks.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Injects animation command data for pushblock types 2, 3 and 4 to restore
      the missing scraping SFX when pulling these blocks.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>pickup_aid.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Injects a sprite sequence similar to the Midas twinkle effect, which is
      used when the option for pickup aids is enabled. Custom levels should
      define a suitable sprite sequence in slot 185.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>photo.bin</code>
    </td>
    <td>TR1, TR2</td>
    <td>
      Injects camera shutter sound effect for the photo mode, needed only for
      the cutscene levels.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>purple_crystal.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Injects a replacement savegame crystal model to match the PS1 style.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>scion_collision.bin</code>
    </td>
    <td>TR1</td>
    <td>
      Increases the collision radius on the (targetable) Scion such that it can
      be shot with the shotgun.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>seaweed_collision.bin</code>
    </td>
    <td>TR2</td>
    <td>
      Fixes the seaweed in Living Quarters blocking Lara from exiting the water.
    </td>
  </tr>
</table>
