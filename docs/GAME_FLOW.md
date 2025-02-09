# Game flow specification

In the original Tomb Raider 1, the game flow was completely hard-coded,
including the levels, limiting the builders' flexibility. Tomb Raider 2
improved upon this by introducing a tombpc.dat binary file for game
configuration, although it remained cryptic and required builders to use
dedicated tooling. TR1X and TR2X have transitioned away from these earlier
methods, choosing to manage game flow with a JSON file that provides a unified
structure for both games. This document details the elements that can be
modified using this updated format.

Jump to:
- [Global properties](#global-properties)
- [Level properties](#level-properties)
- [Sequences](#sequences)
- [Bonus levels](#bonus-levels)
- [Item drops](#item-drops)
- [Injections](#injections)
- [User configuration](#user-configuration)

## Global properties
The following properties are in the root of the game flow document and control
various pieces of global behaviour. Currently, the majority of this section
remains distinct for each game.

#### TR1

<details>
<summary>Show snippet</summary>

```json5
"main_menu_picture": "data/titleh.png",
"savegame_fmt_legacy": "saveati.%d",
"savegame_fmt_bson": "save_tr1_%02d.dat",
"demo_delay": 16,
"water_color": [0.45, 1.0, 1.0],
"draw_distance_fade": 22.0,
"draw_distance_max": 30.0,
"injections": [
    "data/global_injection1.bin",
    "data/global_injection2.bin",
    // etc
],
"convert_dropped_guns": false,
"enforced_config": {
    "enable_save_crystals": false,
},
"levels": [
    {
        "path": "data/gym.phd",
        // etc
    },
],
"cutscenes": [
    {
        "path": "data/cut1.phd",
        // etc
    },
],
"demos": [
    {
        "path": "data/gym.phd",
        // etc
    },
],
"fmvs": [
    {"path": "data/snow.rpl"},
    // etc
},
```
</details>

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td>
      <a name="convert-dropped-guns"></a>
      <code>convert_dropped_guns</code>
    </td>
    <td>Boolean</td>
    <td>
      Forces guns dropped by enemies to be converted to the equivalent ammo
      if Lara already has the gun. See <a href="#item-drops">Item drops</a>
      for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>demo_delay</code></td>
    <td>Double<strong>*</strong></td>
    <td>
      The number of seconds to pass in the main menu before playing the demo.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="draw-distance-fade"></a>
      <code>draw_distance_fade</code>
    </td>
    <td>Double<strong>*</strong></td>
    <td>
      The distance (in tiles) at which objects and the world start to fade into
      blackness.
      <ul>
        <li>The default hardcoded value in TR1 is 12.</li>
        <li>The default (disabled) value in TombATI is 72.</li>
      </ul>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="draw-distance-max"></a>
      <code>draw_distance_max</code>
    </td>
    <td>Double<strong>*</strong></td>
    <td>
      The distance (in tiles) at which objects and the world are clipped away.
      <ul>
        <li>The default hardcoded value in TR1 is 20.</li>
        <li>The default (disabled) value in TombATI is 80.</li>
      </ul>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="enable-killer-pushblocks"></a>
      <code>enable_killer_pushblocks</code>
    </td>
    <td>Boolean</td>
    <td>
      If enabled, when a pushblock falls from the air and lands on Lara, it will
      kill her outright. Otherwise, Lara will clip on top of the block and
      survive.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="enable-tr2-item-drops"></a>
      <code>enable_tr2_item_drops</code>
    </td>
    <td>Boolean</td>
    <td>
      Forces enemies who are placed in the same position as pickup items to
      carry those items and drop them when killed, similar to TR2+. See
      <a href="#item-drops">Item drops</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><a name="enforced-config"></a>
    <code>enforced_config</code></td>
    <td>String-to-object map</td>
    <td>
      This allows <em>any</em> regular game config setting to be overriden. See
      <a href="#user-configuration">User configuration</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>injections</code></td>
    <td>String array</td>
    <td>
      Global data injection file paths. Individual levels will inherit these
      unless <code>inherit_injections</code> is set to <code>false</code> on
      those levels. See <a href="#injections">Injections</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>levels</code></td>
    <td>Object array<strong>*</strong></td>
    <td>
      This is where the individual level details are defined - see
      <a href="#level-properties">Level properties</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>main_menu_picture</code></td>
    <td>String<strong>*</strong></td>
    <td>Path to the main menu background image.</td>
  </tr>
  <tr valign="top">
    <td><code>savegame_fmt_bson</code></td>
    <td>String<strong>*</strong></td>
    <td>Path pattern to look for the savegame files.</td>
  </tr>
  <tr valign="top">
    <td><code>savegame_fmt_legacy</code></td>
    <td>String<strong>*</strong></td>
    <td>Path pattern to look for the old TombATI savegame files.</td>
  </tr>
  <tr valign="top">
    <td>
      <a name="water-color"></a>
      <code>water_color</code>
    </td>
    <td>Float array</td>
    <td>
      Water color (R, G, B). 1.0 means pass-through, 0.0 means no value at all.
      <ul>
        <li>[0.6, 0.7, 1.0] is the original DOS version filter.</li>
        <li>[0.45, 1.0, 1.0] is the default TombATI filter.</li>
      </ul>
    </td>
  </tr>
</table>

**\*** Required property.

#### TR2

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>cmd_init</code></td>
    <td rowspan="6">Object</td>
    <td>
      The command to run when the game is first launched. See <a
      href="#game-flow-commands">Game flow commands</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>cmd_title</code></td>
    <td>The command to run when the main menu is to be shown.</td>
  </tr>
  <tr valign="top">
    <td><code>cmd_death_in_game</code></td>
    <td>The command to run when Lara dies.</td>
  </tr>
  <tr valign="top">
    <td><code>cmd_death_in_demo</code></td>
    <td>The command to run when Lara dies during a demo (not used in the original game).</td>
  </tr>
  <tr valign="top">
    <td><code>cmd_demo_interrupt</code></td>
    <td>The command to run when the player interrupts a demo.</td>
  </tr>
  <tr valign="top">
    <td><code>cmd_demo_end</code></td>
    <td>The command to run when a demo finishes playback.</td>
  </tr>
  <tr valign="top">
    <td><code>cheat_keys</code></td>
    <td>Boolean</td>
    <td>
      Whether to enable original game cheats (the ones where Lara turns around
      three times).
    </td>
  </tr>
  <tr valign="top">
    <td><code>load_save_disabled</code></td>
    <td>Boolean</td>
    <td>Whether to disable saving and loading the game.</td>
  </tr>
  <tr valign="top">
    <td><code>play_any_level</code></td>
    <td>Boolean</td>
    <td>
      Whether to show a full list of all levels in place of the New Game
      passport page.
    </td>
  </tr>
  <tr valign="top">
    <td><code>lockout_option_ring</code></td>
    <td>Boolean</td>
    <td>Whether to disallow the players to use control ring while ingame.</td>
  </tr>
  <tr valign="top">
    <td><code>gym_enabled</code></td>
    <td>Boolean</td>
    <td>Whether to enable Lara's Home.</td>
  </tr>
  <tr valign="top">
    <td><code>demo_version</code></td>
    <td>Boolean</td>
    <td>Legacy setting scheduled for removal at a later time.</td>
  </tr>
  <tr valign="top">
    <td><code>single_level</code></td>
    <td>Integer</td>
    <td>Force the player to only play this one level.</td>
  </tr>
  <tr valign="top">
    <td><code>demo_delay</code></td>
    <td>Double</td>
    <td>
      The number of seconds to pass in the main menu before playing the demo.
    </td>
  </tr>
  <tr valign="top">
    <td><code>secret_track</code></td>
    <td>Integer</td>
    <td>
      Music track to play when a secret is found. -1 to not play anything.
    </td>
  </tr>
</table>

## Game flow commands

The command allows you to modify the original game flow, but please note that
deviations from the original script may result in unexpected behavior. If you
encounter any bugs, we encourage you to report your experience by opening an
issue on GitHub. The overall structure is as follows:

```json5
{
  "command": "play_level",
  "param": 5,
}
```

Currently the following commands are available.

<table>
  <tr>
    <th>Command</th>
    <th>Description</th>
    <th>Parameter</th>
  </tr>
  <tr>
    <td><code>noop</code></td>
    <td>Continue the flow as normal.</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td><code>play_level</code></td>
    <td>Play a specific level.</td>
    <td>Level to play.</td>
  </tr>
  <tr>
    <td><code>load_saved_game</code></td>
    <td>Load a specific savegame.</td>
    <td>Save slot number to use</td>
  </tr>
  <tr>
    <td><code>play_cutscene</code></td>
    <td>Play a specific cutscene.</td>
    <td>Cutscene number to play</td>
  </tr>
  <tr>
    <td><code>play_demo</code></td>
    <td>Play a specific demo.</td>
    <td>Demo number to play.</td>
  </tr>
  <tr>
    <td><code>play_fmv</code></td>
    <td>Play a specific movie.</td>
    <td>Movie number to play.</td>
  </tr>
  <tr>
    <td><code>exit_to_title</code></td>
    <td>Return the game to the title screen.</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td><code>level_complete</code></td>
    <td>
      End the current sequence inside level sequences, do nothing otherwise.
    </td>
    <td>N/A</td>
  </tr>
  <tr>
    <td><code>exit_game</code></td>
    <td>Exit the game to desktop.</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td><code>select_level</code></td>
    <td>Play a specific level (and reset inventory).</td>
    <td>Level number to play.</td>
  </tr>
  <tr>
    <td><code>restart_level</code>¹</td>
    <td>Restart the currently played level.</td>
    <td>N/A</td>
  </tr>
  <tr>
    <td><code>story_so_far</code>¹</td>
    <td>Play the movies and cutscenes up until the currently played level.</td>
    <td>Save slot number to use</td>
  </tr>
</table>

**¹** Tomb Raider 1 only.

Additional notes:
- All numbers (levels, cutscenes, ...) start with 0.

## Level properties
The `levels` section of the document defines how the game plays out. This is an
array of objects and can be defined in any order. The flow is controlled using
the correct [sequencing](#sequences) within each level itself.

Following are each of the properties available within a level.

<details>
<summary>Show snippet</summary>

```json5
{
    "path": "data/example.phd",
    "music_track": 57,
    "lara_type": 0,
    "water_color": [0.7, 0.5, 0.85],
    "draw_distance_fade": 34.0,
    "draw_distance_max": 50.0,
    "unobtainable_pickups": 1,
    "unobtainable_kills": 1,
    "inherit_injections": false,
    "injections": [
        "data/level_injection1.bin",
        "data/level_injection2.bin",
    ],
    "item_drops": [
        {"enemy_num": 17, "object_ids": [86]},
        {"enemy_num": 50, "object_ids": [87]},
        // etc
    ],
    "sequence": [
        {"type": "play_fmv", "fmv_id": 0},
        // etc
    ],
},
```
</details>

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th colspan="2">Description</th>
  </tr>
  <tr valign="top">
    <td><code>path</code></td>
    <td>String<strong>*</strong></td>
    <td colspan="2">The path to the level's data file.</td>
  </tr>
  <tr valign="top">
    <td rowspan="7">
      <code>type</code>
    </td>
    <td rowspan="7">String</td>
    <td colspan="2">
      The level type, which must be one of the following values.
      Defaults to normal level.
    </td>
  </tr>
  <tr valign="top">
    <td><strong>Type</strong></td>
    <td><strong>Description</strong></td>
  </tr>
  <tr valign="top">
    <td><code>normal</code></td>
    <td>A standard level.</td>
  </tr>
  <tr valign="top">
    <td><code>gym</code></td>
    <td>
      At most one of these can be defined. Accessed from the photo option
      (object ID 73) on the title screen. If omitted, the photo option is not
      displayed.
    </td>
  </tr>
  <tr valign="top">
    <td><code>bonus</code></td>
    <td>
      Only playable when all secrets are collected. See
      <a href="#bonus-levels">Bonus levels</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>current</code></td>
    <td>
      One level of this type is necessary to read TombATI's save files. OG has a
      special level called <code>LV_CURRENT</code> to handle save/load logic.
      TR1X does away with this hack. However, the existing save games expect the
      level count to match, otherwise the game will crash.
    </td>
  </tr>
  <tr valign="top">
    <td><code>dummy</code></td>
    <td>A placeholder level necessary to read TombATI's save files.</td>
  </tr>
  <tr valign="top">
    <td><code>sequence</code></td>
    <td>Object array<strong>*</strong></td>
    <td colspan="2">
      Instructions to define how a level plays out. See
      <a href="#sequences">Sequences</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>music_track</code></td>
    <td>Integer<strong>*</strong></td>
    <td colspan="2">The ambient music track ID.</td>
  </tr>
  <tr valign="top">
    <td><code>draw_distance_fade</code><strong>¹</strong></td>
    <td>Double</td>
    <td colspan="2">
      Can be customized per level. See <a href="#draw-distance-fade">above</a>
      for details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>draw_distance_max</code><strong>¹</strong></td>
    <td>Double</td>
    <td colspan="2">
      Can be customized per level. See <a href="#draw-distance-max">above</a>
      for details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>injections</code></td>
    <td>String array</td>
    <td colspan="2">
      Injection file paths. See <a href="#injections">Injections</a> for full
      details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>inherit_injections</code></td>
    <td>Boolean</td>
    <td colspan="2">
      A flag to indicate whether or not the level should use the globally
      defined injections. See <a href="#injections">Injections</a> for full
      details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>item_drops</code><strong>¹</strong></td>
    <td>Object array</td>
    <td colspan="2">
      Instructions to allocate items to enemies who will drop those items when
      killed. See <a href="#item-drops">Item drops</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>lara_type</code><strong>¹</strong></td>
    <td>Integer / string</td>
    <td colspan="2">
      Used only in cutscene levels to link the braid (if enabled) to the
      relevant cutscene actor object ID.
    </td>
  </tr>
  <tr valign="top">
    <td><code>unobtainable_kills</code><strong>¹</strong></td>
    <td>Integer</td>
    <td colspan="2">
      A count of enemies that will be excluded from kill statistics.
    </td>
  </tr>
  <tr valign="top">
    <td><code>unobtainable_pickups</code><strong>¹</strong></td>
    <td>Integer</td>
    <td colspan="2">
      A count of items that will be excluded from pickup statistics.
    </td>
  </tr>
  <tr valign="top">
    <td><code>unobtainable_secrets</code><strong>¹</strong></td>
    <td>Integer</td>
    <td colspan="2">
      A count of secrets that will be excluded from secret statistics. Useful for level demos.
    </td>
  </tr>
  <tr valign="top">
    <td><code>water_color</code><strong>¹</strong></td>
    <td>Float array</td>
    <td colspan="2">
      Can be customized per level. See <a href="#water-color">above</a> for
      details.
    </td>
  </tr>
</table>

**\*** Required property.  
**¹** Tomb Raider 1 only.

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
    <td> Displays the specified picture for a fixed time. </td>
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
          <a href="#give-item"><code>give_item</code></a> - so, item removal is
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
    <td><code>setup_bacon_lara</code><strong>¹</strong></td>
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
    <td><code>set_secret_count</code><strong>²</strong></td>
    <td><code>value</code></td>
    <td>Integer</td>
    <td>
      Sets the current level secret count to this number. In Tomb Raider II
      this is mainly used to mark certain levels, such as Dragon's Lair, as
      having no secrets.
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
    <td><code>disable_floor</code><strong>²</strong></td>
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

## Cutscenes
The `cutscenes` section contains all the cinematic levels, used with the
`play_cutscene` sequence. Its structure is identical to the `levels` section.


## Demos
The `demos` section contains all the levels that can play a demo when the player
leaves the main inventory screen idle for a while or by using the `/demo`
command. For the demos to work, these levels need to have demo data built-in.
Aside from this requirement, this section works just like the `levels` section.

## Bonus levels
The game flow supports bonus levels, which are unlocked only when the player
collects all secrets in the game's normal levels. These bonus levels behave just
like normal levels, so you can include FMVs, cutscenes in-between and so on.

Statistics are maintained separately, so normal end-game statistics are shown
once, and then separate bonus level statistics are shown on completion of those
levels.

Following is a sample level configuration with three normal levels and two bonus
levels. After the end-game credits are played following level 3, if the player
has collected all secrets, they will then be taken to level 4. Otherwise, the
game will exit to title.

<details>
<summary>Show example setup</summary>

```json5
{
    "levels": [
        {
            // gym level definition
        },

        {
            "path": "data/level1.phd",
            "music_track": 57,
            "sequence": [
                {"type": "loop_game"},
                {"type": "level_stats"},
                {"type": "level_complete"},
            ],
        },

        {
            "path": "data/level2.phd",
            "music_track": 57,
            "sequence": [
                {"type": "loop_game"},
                {"type": "level_stats"},
                {"type": "level_complete"},
            ],
        },

        {
            "path": "data/level3.phd",
            "music_track": 57,
            "sequence": [
                {"type": "loop_game"},
                {"type": "level_stats"},
                {"type": "play_music", "music_track": 19},
                {"type": "display_picture", "path": "data/end.pcx", "display_time": 7.5},
                {"type": "display_picture", "path": "data/cred1.pcx", "display_time": 7.5},
                {"type": "display_picture", "path": "data/cred2.pcx", "display_time": 7.5},
                {"type": "display_picture", "path": "data/cred3.pcx", "display_time": 7.5},
                {"type": "total_stats", "background_path": "data/install.pcx"},
                {"type": "level_complete"},
            ],
        },

        {
            "path": "data/bonus1.phd",
            "type": "bonus",
            "music_track": 57,
            "sequence": [
                {"type": "play_fmv", "fmv_path": "fmv/snow.avi"},
                {"type": "loop_game"},
                {"type": "play_cutscene", "cutscene_id": 0},
                {"type": "level_stats"},
                {"type": "level_complete"},
            ],
        },

        {
            "path": "data/bonus2.phd",
            "type": "bonus",
            "music_track": 57,
            "sequence": [
                {"type": "loop_game"},
                {"type": "level_stats"},
                {"type": "play_music", "music_track": 14},
                {"type": "total_stats", "background_path": "data/install.pcx"},
                {"type": "exit_to_title"},
            ],
        },
    ],

    "cutscenes": [
        {
            "path": "data/bonuscut1.phd",
            "music_track": 23,
            "sequence": [
                {"type": "set_cutscene_angle", "value": -23312},
                {"type": "loop_game"},
            ],
        },
    ],
}
```
</details>

## Item drops
In the original Tomb Raider I, items dropped by enemies were hardcoded such
that only specific enemies could drop, and the items and quantities that they
dropped were immutable. This is no longer the case, with the game flow providing
a mechanism to allow the _majority_ of enemy types to carry and drop items.
Note that this also means by default that the original enemies who did drop
items will not do so unless the game flow has been configured as such.

Item drops can be defined in two ways. If `enable_tr2_item_drops` is `true`,
then custom level builders can add items directly to the level file, setting
their position to be the same as the enemies who should drop them.

For the original levels, `enable_tr2_item_drops` is `false`. Item drops are
instead defined in the `item_drops` section of a level's definition by creating
objects with the following parameter structure. You can define at most one entry
per enemy, but that definition can have as many drop items as necessary (within
the engine's overall item limit).

<details>
<summary>Show example setup</summary>

```json5
{
    "path": "data/example.phd",
    "music_track": 57,
    "item_drops": [
        {"enemy_num": 17, "object_ids": [86]},
        {"enemy_num": 50, "object_ids": [87]},
        {"enemy_num": 12, "object_ids": [93, 93]},
        {"enemy_num": 47, "object_ids": [111]},
    ],
    "sequence": [
         {"type": "loop_game"},
         {"type": "level_stats"},
         {"type": "level_complete"},
    ],
},
```

This translates as follows.
- Enemy #17 will drop the magnums
- Enemy #50 will drop the uzis
- Enemy #12 will drop two small medipacks
- Enemy #47 will drop puzzle 2
</details>

<table>
  <tr valign="top" align="left">
    <th>Parameter</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td>
      <code>enemy_num</code>
    </td>
    <td>Integer</td>
    <td>The index of the enemy in the level's item list.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>object_ids</code>
    </td>
    <td>Integer / string array</td>
    <td>
      A list of item <em>types</em> to drop. These items will spawn dynamically
      and do not need to be added to the level file. Duplicate IDs are permitted
      in the same array.
    </td>
  </tr>
</table>

You can also toggle `convert_dropped_guns` in
[global properties](#convert-dropped-guns). When `true`, if an enemy drops a gun
that Lara already has, it will be converted to the equivalent ammo. When
`false`, the gun will always be dropped.

### Enemy validity

All enemy types are permitted to carry and drop items. This includes regular
enemies as well as TR1 Atlantean pods (objects 163, 181), TR1 centaur
statues (object 161), and TR2 statues (objects 42, 44). For pods, the items will be allocated to the creature
within (obviously empty pods are excluded).

Items dropped by flying or swimming creatures will fall to the ground (TR1 only).

For clarity, following is a list of all enemy type IDs which you can
reference when building your game flow. The game flow will ignore drops for
non-enemy type objects, and a suitable warning message will be produced in the
log file.

<table>
  <tr><th colspan="2">TR1</th><th colspan="2">TR2</th></tr>
  <tr valign="top" align="left"><th>Object ID <th>Name</th><th>Object ID <th>Name</th></tr>
  <tr><td>7</td><td>Wolf</td><td>15</td><td>Dog</td></tr>
  <tr><td>8</td><td>Bear</td><td>16</td><td>Masked Goon 1</td></tr>
  <tr><td>9</td><td>Bat</td><td>17</td><td>Masked Goon 2</td></tr>
  <tr><td>10</td><td>Crocodile</td><td>18</td><td>Masked Goon 3</td></tr>
  <tr><td>11</td><td>Alligator</td><td>19</td><td>Knife Thrower</td></tr>
  <tr><td>12</td><td>Lion</td><td>20</td><td>Shotgun Goon</td></tr>
  <tr><td>13</td><td>Lioness</td><td>21</td><td>Rat</td></tr>
  <tr><td>14</td><td>Puma</td><td>22</td><td>Dragon Front</td></tr>
  <tr><td>15</td><td>Ape</td><td>25</td><td>Shark</td></tr>
  <tr><td>16</td><td>Rat</td><td>26</td><td>Eel</td></tr>
  <tr><td>17</td><td>Vole</td><td>27</td><td>Big Eel</td></tr>
  <tr><td>18</td><td>T-rex</td><td>28</td><td>Barracuda</td></tr>
  <tr><td>19</td><td>Raptor</td><td>29</td><td>Scuba Diver</td></tr>
  <tr><td>20</td><td>Flying mutant</td><td>30</td><td>Gunman Goon 1</td></tr>
  <tr><td>21</td><td>Grounded mutant (shooter)</td><td>31</td><td>Gunman Goon 2</td></tr>
  <tr><td>22</td><td>Grounded mutant (non-shooter)</td><td>32</td><td>Stick Wielding Goon 1</td></tr>
  <tr><td>23</td><td>Centaur</td><td>33</td><td>Stick Wielding Goon 2</td></tr>
  <tr><td>24</td><td>Mummy (Tomb of Qualopec)</td><td>34</td><td>Flamethrower Goon</td></tr>
  <tr><td>27</td><td>Larson</td><td>35</td><td>Jellyfish</td></tr>
  <tr><td>28</td><td>Pierre (not runaway)</td><td>36</td><td>Spider</td></tr>
  <tr><td>30</td><td>Skate kid</td><td>37</td><td>Giant Spider</td></tr>
  <tr><td>31</td><td>Cowboy</td><td>38</td><td>Crow</td></tr>
  <tr><td>32</td><td>Kold</td><td>39</td><td>Tiger</td></tr>
  <tr><td>33</td><td>Natla (items drop after second phase)</td><td>40</td><td>Marco Bartoli</td></tr>
  <tr><td>34</td><td>Torso</td><td>41</td><td>Xian Spearman</td></tr>
  <tr><td colspan="2" rowspan="11"></td><td>42</td><td>Xian Spearman Statue</td></tr>
  <tr><td>43</td><td>Xian Knight</td></tr>
  <tr><td>44</td><td>Xian Knight</td></tr>
  <tr><td>45</td><td>Yeti</td></tr>
  <tr><td>46</td><td>Bird Monster</td></tr>
  <tr><td>47</td><td>Eagle</td></tr>
  <tr><td>48</td><td>Mercenary 1</td></tr>
  <tr><td>49</td><td>Mercenary 2</td></tr>
  <tr><td>50</td><td>Mercenary 3</td></tr>
  <tr><td>52</td><td>Black Snowmobile Driver</td></tr>
  <tr><td>214</td><td>T-Rex</td></tr>
</table>

### Item validity
The following object types are capable of being carried and dropped. The
game flow will ignore anything that is not in this list, and a suitable warning
message will be produced in the log file.

<table>
  <tr><th colspan="2">TR1</th><th colspan="2">TR2</th></tr>
  <tr valign="top" align="left"><th>Object ID</th><th>Name</th><th>Object ID</th><th>Name</th></tr>
  <tr><td>84</td><td>Pistols</td><td>135</td><td>Pistols</td></tr>
  <tr><td>85</td><td>Shotgun</td><td>136</td><td>Shotgun</td></tr>
  <tr><td>86</td><td>Magnums</td><td>137</td><td>Automatic Pistols</td></tr>
  <tr><td>87</td><td>Uzis</td><td>138</td><td>Uzis</td></tr>
  <tr><td>89</td><td>Shotgun ammo</td><td>139</td><td>Harpoon Gun</td></tr>
  <tr><td>90</td><td>Magnum ammo</td><td>140</td><td>M16</td></tr>
  <tr><td>91</td><td>Uzi ammo</td><td>141</td><td>Grenade Launcher</td></tr>
  <tr><td>93</td><td>Small medipack</td><td>142</td><td>Pistol Clips</td></tr>
  <tr><td>94</td><td>Large medipack</td><td>143</td><td>Shotgun Shells</td></tr>
  <tr><td>110</td><td>Puzzle1</td><td>144</td><td>Automatic Pistol Clips</td></tr>
  <tr><td>111</td><td>Puzzle2</td><td>145</td><td>Uzi Clips</td></tr>
  <tr><td>112</td><td>Puzzle3</td><td>146</td><td>Harpoons</td></tr>
  <tr><td>113</td><td>Puzzle4</td><td>147</td><td>M16 Clips</td></tr>
  <tr><td>126</td><td>Lead bar</td><td>148</td><td>Grenades</td></tr>
  <tr><td>129</td><td>Key1</td><td>149</td><td>Small Medipack</td></tr>
  <tr><td>130</td><td>Key2</td><td>150</td><td>Large Medipack</td></tr>
  <tr><td>131</td><td>Key3</td><td>152</td><td>Flare</td></tr>
  <tr><td>132</td><td>Key4</td><td>151</td><td>Flares Box</td></tr>
  <tr><td>141</td><td>Pickup1</td><td>174</td><td>Puzzle Item 1</td></tr>
  <tr><td>142</td><td>Pickup2</td><td>175</td><td>Puzzle Item 2</td></tr>
  <tr><td>144</td><td>Scion (à la Pierre)</td><td>176</td><td>Puzzle Item 3</td></tr>
  <tr><td rowspan="10" colspan="2"></td><td>177</td><td>Puzzle Item 4</td></tr>
  <tr><td>193</td><td>Key 1</td></tr>
  <tr><td>194</td><td>Key 2</td></tr>
  <tr><td>195</td><td>Key 3</td></tr>
  <tr><td>196</td><td>Key 4</td></tr>
  <tr><td>205</td><td>Pickup Item 1</td></tr>
  <tr><td>206</td><td>Pickup Item 2</td></tr>
  <tr><td>190</td><td>Secret 1</td></tr>
  <tr><td>191</td><td>Secret 2</td></tr>
  <tr><td>192</td><td>Secret 3</td></tr>
</table>



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

#### TR1

<table>
  <tr valign="top" align="left">
    <th>Injection file</th>
    <th>Purpose</th>
  </tr>
  <tr valign="top">
    <td>
      <code>*_fd.bin</code>
    </td>
    <td>
      Injects fixes for floor data issues in the original levels. Refer to the
      README for a full list of fixes.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>*_itemrots.bin</code>
    </td>
    <td>
      Injects rotations on pickup items so they make more visual sense when
      using the 3D pickups option.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>*_skybox.bin</code>
    </td>
    <td>
      Injects a predefined skybox model into specific levels.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>*_textures.bin</code>
    </td>
    <td>
      Injects fixes for texture issues in the original levels, such as gaps in
      the walls or wrongly colored models. Refer to the README for a full list
      of fixes.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>backpack.bin</code>
    </td>
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
      <code>cistern_plants.bin</code>
    </td>
    <td>
      This disables the animation on sprite ID 193 in The Cistern and Tomb of
      Tihocan.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>khamoon_mummy.bin</code>
    </td>
    <td>
      Injects the mummy in room 25 of City of Khamoon, which is present in the
      PS1 version but not the PC.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>lara_animations.bin</code>
    </td>
    <td>
      Injects several animations, state changes and commands for Lara, such as
      responsive jumping, jump-twist, somersault, underwater roll, and wading.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>explosion.bin</code>
    </td>
    <td>
      Injects explosion sprites for certain console commands.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>mines_pushblocks.bin</code>
    </td>
    <td>
      Injects animation command data for pushblock types 2, 3 and 4 to restore
      the missing scraping SFX when pulling these blocks.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>pickup_aid.bin</code>
    </td>
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
    <td>
      Injects camera shutter sound effect for the photo mode, needed only for
      the cutscene levels.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>purple_crystal.bin</code>
    </td>
    <td>
      Injects a replacement savegame crystal model to match the PS1 style.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>scion_collision.bin</code>
    </td>
    <td>
      Increases the collision radius on the (targetable) Scion such that it can
      be shot with the shotgun.
    </td>
  </tr>
</table>

#### TR2

TBD

## FMVs

The FMVs section of the document defines how to play video content. This is an
array of objects and can be defined in any order. The flow is controlled using
the correct sequencing within each level itself.

Following are each of the properties available within an FMV.

<details>
<summary>Show snippet</summary>

```json5
{
    "path": "data/example.avi",
}
```
</details>

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th colspan="2">Description</th>
  </tr>
  <tr valign="top">
    <td>
      <code>path</code>
    </td>
    <td>String<strong>*</strong></td>
    <td colspan="2">The path to the FMV's video file.</td>
  </tr>
</table>

**\*** Required property.

## User Configuration
TRX ships with a configuration tool to allow users to adjust game settings to
their taste. This tool writes to `cfg\TR1X.json5` and `cfg\TR2X.json5`. As a
level builder, you may however wish to enforce some settings to match how your
level is designed.

As an example, let's say you do not wish to add save crystals to your level, and
as a result you wish to prevent the player from enabling that option in the
config tool. To achieve this, open `cfg\TR1X_gameflow.json5` in a suitable text
editor and add the following.

```json
"enforced_config" : {
  "enable_save_crystals" : false,
}
```

This means that the game will enforce your chosen value for this particular
config setting. If the player launches the config tool, the option to toggle
save crystals will be greyed out.

You can add as many settings within the `enforced_config` section as needed.
Refer to the key names within `cfg\TR1X.json5` and `cfg\TR2X.json5` for
reference.

Note that you do not need to ship a full configuration with your level, and
indeed it is not recommended to do so if you have, for example, your own custom
keyboard or controller layouts defined.

If you do not have any requirement to enforce settings, you can omit the
`enforced_config` section from your game flow altogether.
