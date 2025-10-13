---
title: Sequences
---

## Sequences

The following describes each available game flow sequence type and the required
parameters for each. Note that while this table is displayed in alphabetical
order, care must be taken to define sequences in the correct order. Refer to the
default game flow for examples.

<table>
  <tr valign="top" align="left">
    <th>Sequence</th>
    <th>Parameter</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>loop_game</code></td>
    <td colspan="2" align="center">N/A</td>
    <td>Plays the main game loop.</td>
  </tr>
  <tr valign="top">
    <td><code>level_complete</code></td>
    <td colspan="2" align="center">N/A</td>
    <td>Ends the current level and plays the next one, if available.</td>
  </tr>
  <tr valign="top">
    <td><code>exit_to_title</code></td>
    <td colspan="2" align="center">N/A</td>
    <td>Returns to the title level.</td>
  </tr>
  <tr valign="top">
    <td><code>level_stats</code></td>
    <td colspan="2" align="center">N/A</td>
    <td>
        Displays the end of level statistics for the current level. In a Gym
        level, this fades the screen to black.
    </td>
  </tr>
  <tr valign="top">
    <td><code>total_stats</code></td>
    <td><code>path</code></td>
    <td>String</td>
    <td>
      Displays the end of game statistics with the given picture file shown as
      a background.
    </td>
  </tr>
  <tr valign="top">
    <td rowspan="4">
      <code>display_picture</code>
    </td>
    <td><code>path</code></td>
    <td>String</td>
    <td>
      Displays the specified picture for a fixed time.
      Files that are needed to function only with a specific aspect ratio can
      be placed in a directory adjacent to the main image, named according to
      the aspect ratio – for example, 4x3/title.png or 16x10/title.png. The
      game won't attempt to match these precisely; instead, it will select the
      file with the aspect ratio closest to the game's viewport. The main image
      designated by <code>path</code> is presumed to have a 16:9 aspect ratio
      for this purpose, and as such there's no need for 16x9-specific
      directory.<br/>
      This logic applies to all images.
    </td>
  </tr>
  <tr valign="top">
    <td><code>display_time</code></td>
    <td>Double</td>
    <td> Number of seconds to display the picture for (default: 5). </td>
  </tr>
  <tr valign="top">
    <td><code>fade_in_time</code></td>
    <td>Double</td>
    <td>
      Number of seconds to do the fade-in animation, if enabled (default: 1).
    </td>
  </tr>
  <tr valign="top">
    <td><code>fade_out_time</code></td>
    <td>Double</td>
    <td>
      Number of seconds to do the fade-out animation, if enabled (default: 0.33).
    </td>
  </tr>
  <tr valign="top">
    <td rowspan="4"><code>loading_screen</code><strong>¹</strong></td>
    <td><code>path</code></td>
    <td>String</td>
    <td rowspan="4">
      Shows a picture prior to loading a level. Functions identically to
      <code>display_picture</code>, except these pictures can be
      enabled/disabled by the user with the loading screen option in the config
      tool.
    </td>
  </tr>
  <tr valign="top">
    <td><code>display_time</code></td>
    <td>Double</td>
  </tr>
  <tr valign="top">
    <td><code>fade_in_time</code></td>
    <td>Double</td>
  </tr>
  <tr valign="top">
    <td><code>fade_out_time</code></td>
    <td>Double</td>
  </tr>
  <tr valign="top">
    <td><code>play_cutscene</code></td>
    <td><code>cutscene_id</code></td>
    <td>Integer</td>
    <td>
      Plays the specified cinematic level (from the <code>cutscenes</code>).
    </td>
  </tr>
  <tr valign="top">
    <td><code>play_fmv</code></td>
    <td><code>fmv_id</code></td>
    <td>String</td>
    <td>
      Plays the specified FMV. <code>fmv_id</code> must be a valid index into
      the <code>fmvs</code> root key.
    </td>
  </tr>
  <tr valign="top">
    <td><code>flip_map</code><strong>¹</strong></td>
    <td colspan="2" align="center">N/A</td>
    <td>Triggers the flip map.</td>
  </tr>
  <tr valign="top">
    <td rowspan="2">
      <a name="give-item"></a>
      <code>give_item</code>
    </td>
    <td><code>object_id</code></td>
    <td>Integer / String</td>
    <td rowspan="2">
      Adds the specified item and quantity to Lara's inventory.
    </td>
  </tr>
  <tr valign="top">
    <td><code>quantity</code></td>
    <td>Integer</td>
  </tr>
  <tr valign="top">
    <td rowspan="2"><code>add_secret_reward</code><strong>²</strong></td>
    <td><code>object_id</code></td>
    <td>Integer / String</td>
    <td rowspan="2">
      Adds the specified item to the current level's list of rewards for
      collecting all secrets.
    </td>
  </tr>
  <tr valign="top">
    <td><code>quantity</code></td>
    <td>Integer</td>
  </tr>
  <tr valign="top">
    <td rowspan="3"><code>mesh_swap</code><strong>¹</strong></td>
    <td><code>object1_id</code></td>
    <td>Integer</td>
    <td rowspan="3">Swaps the given mesh ID between the given objects.</td>
  </tr>
  <tr valign="top">
    <td><code>object2_id</code></td>
    <td>Integer</td>
  </tr>
  <tr valign="top">
    <td><code>mesh_id</code></td>
    <td>Integer</td>
  </tr>
  <tr valign="top">
    <td><code>play_music</code></td>
    <td><code>music_track</code></td>
    <td>Integer</td>
    <td>Plays the given audio track.</td>
  </tr>
  <tr valign="top">
    <td><code>remove_ammo</code></td>
    <td colspan="2" align="center">N/A</td>
    <td rowspan="5">
      Any combination of these sequences can be used to modify Lara's
      inventory at the start of a level. There are a few simple points to note:
      <ul>
        <li>
          <code>remove_weapons</code> does not remove the ammo for those guns,
          and equally <code>remove_ammo</code> does not remove the guns. Each
          works independently of the other.
        </li>
        <li>
          These sequences can also work together with
          <code><a href="./2-SEQUENCES.md#give-item">give_item</a></code> - so, item removal is
          performed first, followed by addition.
        </li>
      </ul>
    </td>
  </tr>
  <tr valign="top">
    <td><code>remove_weapons</code></td>
    <td colspan="2" align="center">N/A</td>
  </tr>
  <tr valign="top">
    <td><code>remove_medipacks</code></td>
    <td colspan="2" align="center">N/A</td>
  </tr>
  <tr valign="top">
    <td><code>remove_flares</code><strong>²</strong></td>
    <td colspan="2" align="center">N/A</td>
  </tr>
  <tr valign="top">
    <td><code>remove_scions</code><strong>¹</strong></td>
    <td colspan="2" align="center">N/A</td>
  </tr>
  <tr valign="top">
    <td rowspan="3"><code>set_cutscene_pos</code><strong>¹</strong></td>
    <td><code><code>x</code></code></td>
    <td>Integer</td>
    <td rowspan="3">Sets the camera's position.</td>
  </tr>
  <tr valign="top">
    <td><code><code>y</code></code></td>
    <td>Integer</td>
  </tr>
  <tr valign="top">
    <td><code><code>z</code></code></td>
    <td>Integer</td>
  </tr>
  <tr valign="top">
    <td><code>set_cutscene_angle</code></td>
    <td><code>value</code></td>
    <td>Integer</td>
    <td>Sets the camera's angle.</td>
  </tr>
  <tr valign="top">
    <td><code>setup_bacon_lara</code></td>
    <td><code>anchor_room</code></td>
    <td>Integer</td>
    <td>
      Sets the room number in which Bacon Lara will be anchored to enable
      correct mirroring behaviour with Lara.
    </td>
  </tr>
  <tr valign="top">
    <td><code>enable_sunset</code><strong>²</strong></td>
    <td colspan="2" align="center">N/A</td>
    <td>
      Enables the sunset effect, like in Bartoli's Hideout. At present, this
      feature is hardcoded to gradually darken the game 40 minutes into playing
      a level.
    </td>
  </tr>
  <tr valign="top">
    <td><code>set_lara_start_anim</code><strong>²</strong></td>
    <td><code>value</code></td>
    <td>Integer</td>
    <td>
      Applies the selected animation to Lara when the level begins. This is
      used, for example, in the Offshore Rig of Tomb Raider II.
    </td>
  </tr>
  <tr valign="top">
    <td><code>disable_floor</code></td>
    <td><code>value</code></td>
    <td>Integer</td>
    <td>
      Configures a specific height (with 256 representing 1 click and 1024
      representing 1 sector) to define an abyss that will invariably lead to
      Lara's death if she falls into it. Additionally, it employs special
      rendering to ensure it isn't treated as solid ground. This is used, for
      example, in the Floating Islands of Tomb Raider II.
    </td>
  </tr>
</table>

**¹** Tomb Raider 1 only.  
**²** Tomb Raider 2 only.
