# Gameflow specification
The gameflow in TR1X is fully configurable and contributes to removing several
original hard-coded aspects of the game. This document describes each element
available to edit.

Jump to:
- [Global properties](#global-properties)
- [Level properties](#level-properties)
- [Sequences](#sequences)
- [Bonus levels](#bonus-levels)
- [Item drops](#item-drops)
- [Injections](#injections)

## Global properties
The following properties are in the root of the gameflow document and control
various pieces of global behaviour.

<details>
<summary>Show snippet</summary>
  
```json5
"main_menu_picture": "data/titleh.png",
"savegame_fmt_legacy": "saveati.%d",
"savegame_fmt_bson": "save_tr1_%02d.dat",
"force_game_modes": null,
"force_save_crystals": null,
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
"levels": [
    {
        "title": "Caves",
        // etc
    }
],
"strings": {
    "HEADING_INVENTORY": "INVENTORY",
    "HEADING_GAME_OVER": "GAME OVER",
    // etc
},
```
</details>

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Required</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td>
      <a name="convert-dropped-guns"></a>
      <code>convert_dropped_guns</code>
    </td>
    <td>Boolean</td>
    <td>No</td>
    <td>
      Forces guns dropped by enemies to be converted to the equivalent ammo
      if Lara already has the gun. See <a href="#item-drops">Item drops</a>
      for full details.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>demo_delay</code>
    </td>
    <td>Double</td>
    <td>Yes</td>
    <td>
      The number of seconds to pass in the main menu before playing the demo.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="draw-distance-fade"></a>
      <code>draw_distance_fade</code>
    </td>
    <td>Double</td>
    <td>Yes</td>
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
    <td>Double</td>
    <td>Yes</td>
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
      <code>force_game_modes</code>
    </td>
    <td>Optional Boolean</td>
    <td>No</td>
    <td>
      Forces game mode selection to be enabled if <code>true</code> or disabled
      if <code>false</code>, so the user can't select NG+ modes until a
      playthrough is completed. Overrides the config option
      <code>enable_game_modes</code>. Has no action if <code>null</code>.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>force_save_crystals</code>
    </td>
    <td>Optional Boolean</td>
    <td>No</td>
    <td>
      Forces save crystals to be enabled if <code>true</code> or disabled if
      <code>false</code>. Overrides the config option
      <code>enable_save_crystals</code>. Has no action if <code>null</code>.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>injections</code>
    </td>
    <td>String array</td>
    <td>No</td>
    <td>
      Global data injection file paths. Individual levels will inherit these
      unless <code>inherit_injections</code> is set to <code>false</code> on
      those levels. See <a href="#injections">Injections</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>levels</code>
    </td>
    <td>Object array</td>
    <td>Yes</td>
    <td>
      This is where the individual level details are defined - see
      <a href="#level-properties">Level properties</a> for full details.
    </td>
  </tr>  
  <tr valign="top">
    <td>
      <code>main_menu_picture</code>
    </td>
    <td>String</td>
    <td>Yes</td>
    <td>Path to the main menu background image.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>savegame_fmt_bson</code>
    </td>
    <td rowspan="2">String</td>
    <td rowspan="2">Yes</td>
    <td rowspan="2">Path to the savegame file.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>savegame_fmt_legacy</code>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>strings</code>
    </td>
    <td>String-to-string map</td>
    <td>Yes</td>
    <td>
      All language used in the game is defined here. Edit the right-hand values
      only.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="water-color"></a>
      <code>water_color</code>
    </td>
    <td>Float array</td>
    <td>No</td>
    <td>
      Water color (R, G, B). 1.0 means pass-through, 0.0 means no value at all.
      <ul>
        <li>[0.6, 0.7, 1.0] is the original DOS version filter.</li>
        <li>[0.45, 1.0, 1.0] is the default TombATI filter.</li>
      </ul>
    </td>
  </tr>
</table>

## Level properties
The `levels` section of the document defines how the game plays out. This is an
array of objects and can be defined in any order. The flow is controlled using
the correct [sequencing](#sequences) within each level itself.

Following are each of the properties available within a level.

<details>
<summary>Show snippet</summary>

```json5
{
    "title": "Example Level",
    "file": "data/example.phd",
    "type": "normal",
    "music": 57,
    "lara_type": 0,
    "water_color": [0.7, 0.5, 0.85],
    "draw_distance_fade": 34.0,
    "draw_distance_max": 50.0,
    "demo": true,
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
        {"type": "play_fmv", "fmv_path": "fmv/snow.avi"},
        {"type": "start_game"},
        // etc
    ],
    "strings": {
        "key1": "Silver Key",
        "puzzle2": "Machine Cog",
        // etc
    },
},
```
</details>

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Required</th>
    <th colspan="2">Description</th>
  </tr>
  <tr valign="top">
    <td>
      <code>demo</code>
    </td>
    <td>Boolean</td>
    <td>No</td>
    <td colspan="2">
      Flag to indicate that the level has available demo data to play out from
      the title screen.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>draw_distance_fade</code>
    </td>
    <td>Double</td>
    <td>No</td>
    <td colspan="2">
      Can be customized per level. See <a href="#draw-distance-fade">above</a>
      for details.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>draw_distance_max</code>
    </td>
    <td>Double</td>
    <td>No</td>
    <td colspan="2">
      Can be customized per level. See <a href="#draw-distance-max">above</a>
      for details.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>file</code>
    </td>
    <td>String</td>
    <td>Yes</td>
    <td colspan="2">The path to the level's data file.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>inherit_injections</code>
    </td>
    <td>Boolean</td>
    <td>No</td>
    <td colspan="2">
      A flag to indicate whether or not the level should use the globally
      defined injections. See <a href="#injections">Injections</a> for full
      details.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>injections</code>
    </td>
    <td>String array</td>
    <td>No</td>
    <td colspan="2">
      Injection file paths. See <a href="#injections">Injections</a> for full
      details.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>item_drops</code>
    </td>
    <td>Object array</td>
    <td>No</td>
    <td colspan="2">
      Instructions to allocate items to enemies who will drop those items when
      killed. See <a href="#item-drops">Item drops</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>lara_type</code>
    </td>
    <td>Integer</td>
    <td>No</td>
    <td colspan="2">
      Used only in cutscene levels to link the braid (if enabled) to the
      relevant cutscene actor object ID.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>music</code>
    </td>
    <td>Integer</td>
    <td>Yes</td>
    <td colspan="2">The ambient music track ID.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>sequence</code>
    </td>
    <td>Object array</td>
    <td>Yes</td>
    <td colspan="2">
      Instructions to define how a level plays out. See
      <a href="#sequences">Sequences</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td rowspan="12">
      <code>strings</code>
    </td>
    <td rowspan="12">String-to-string map</td>
    <td rowspan="12">Yes</td>
    <td colspan="2">
      Key and puzzle item names. The possible types are as follows. An empty map
      is permitted.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <strong>String ID</strong>
    </td>
    <td>
      <strong>Description</strong>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>key1</code>
    </td>
    <td>Describes pickup object ID 129 in the inventory top-ring.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>key2</code>
    </td>
    <td>Describes pickup object ID 130 in the inventory top-ring.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>key3</code>
    </td>
    <td>Describes pickup object ID 131 in the inventory top-ring.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>key4</code>
    </td>
    <td>Describes pickup object ID 132 in the inventory top-ring.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>pickup1</code>
    </td>
    <td>Describes pickup object ID 141 in the inventory top-ring.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>pickup2</code>
    </td>
    <td>Describes pickup object ID 142 in the inventory top-ring.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>puzzle1</code>
    </td>
    <td>Describes pickup object ID 110 in the inventory top-ring.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>puzzle2</code>
    </td>
    <td>Describes pickup object ID 111 in the inventory top-ring.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>puzzle3</code>
    </td>
    <td>Describes pickup object ID 112 in the inventory top-ring.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>puzzle4</code>
    </td>
    <td>Describes pickup object ID 113 in the inventory top-ring.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>title</code>
    </td>
    <td>String</td>
    <td>Yes</td>
    <td colspan="2">The title of the level.</td>
  </tr>  
  <tr valign="top">
    <td rowspan="8">
      <code>type</code>
    </td>
    <td rowspan="8">String</td>
    <td rowspan="8">Yes</td>
    <td colspan="2">
      The level type, which must be one of the following values.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <strong>Type</strong>
    </td>
    <td>
      <strong>Description</strong>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>bonus</code>
    </td>
    <td>
      Only playable when all secrets are collected. See
      <a href="#bonus-levels">Bonus levels</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>current</code>
    </td>
    <td>
      One level of this type is necessary to read TombATI's save files. OG has a
      special level called <code>LV_CURRENT</code> to handle save/load logic.
      TR1X does away with this hack. However, the existing save games expect the
      level count to match, otherwise the game will crash.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>cutscene</code>
    </td>
    <td>A cutscene level.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>gym</code>
    </td>
    <td>
      At most one of these can be defined. Accessed from the photo option
      (object ID 73) on the title screen. If omitted, the photo option is not
      displayed.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>normal</code>
    </td>
    <td>A standard level.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>title</code>
    </td>
    <td>The main menu level. One - and only one - of these must be defined.</td>
  </tr>  
  <tr valign="top">
    <td>
      <code>unobtainable_kills</code>
    </td>
    <td>Integer</td>
    <td>No</td>
    <td colspan="2">
      A count of enemies that will be excluded from kill statistics.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>unobtainable_pickups</code>
    </td>
    <td>Integer</td>
    <td>No</td>
    <td colspan="2">
      A count of items that will be excluded from pickup statistics.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>water_color</code>
    </td>
    <td>Float array</td>
    <td>No</td>
    <td colspan="2">
      Can be customized per level. See <a href="#water-color">above</a> for
      details.
    </td>
  </tr>
</table>

## Sequences
The following describes each available gameflow sequence type and the required
parameters for each. Note that while this table is displayed in alphabetical
order, care must be taken to define sequences in the correct order. Refer to the
default gameflow for examples.

<table>
  <tr valign="top" align="left">
    <th>Sequence</th>
    <th>Parameter</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td rowspan="2">
      <code>display_picture</code>
    </td>
    <td>
      <code>picture_path</code>
    </td>
    <td>String</td>
    <td rowspan="2">
      Displays the specified picture for the given number of seconds.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>display_time</code>
    </td>
    <td>Double</td>
  </tr>
  <tr valign="top">
    <td rowspan="2">
      <code>loading_screen</code>
    </td>
    <td>
      <code>picture_path</code>
    </td>
    <td>String</td>
    <td rowspan="2">
      Displays the specified picture for the given number of seconds. Functions identically to display_picture except these pictures can be enabled/disabled by the user with the loading screen option in the config tool.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>display_time</code>
    </td>
    <td>Double</td>
  </tr>
  <tr valign="top">
    <td>
      <code>exit_to_cine</code>
    </td>
    <td>
      <code>level_id</code>
    </td>
    <td>Integer</td>
    <td>Exits to the specified cinematic level.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>exit_to_level</code>
    </td>
    <td>
      <code>level_id</code>
    </td>
    <td>Integer</td>
    <td>Exits to the specified level.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>exit_to_title</code>
    </td>
    <td colspan="2" align="center">N/A</td>
    <td>Returns to the title level.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>flip_map</code>
    </td>
    <td colspan="2" align="center">N/A</td>
    <td>Triggers the flip map.</td>
  </tr>
  <tr valign="top">
    <td rowspan="2">
      <a name="give-item"></a>
      <code>give_item</code>
    </td>
    <td>
      <code>object_id</code>
    </td>
    <td>Integer</td>
    <td rowspan="2">
      Adds the specified item and quantity to Lara's inventory. If used, this
      must appear <em>after</em> the <code>start_game</code> sequence.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>quantity</code>
    </td>
    <td>Integer</td>
  </tr>
  <tr valign="top">
    <td>
      <code>level_stats</code>
    </td>
    <td>
      <code>level_id</code>
    </td>
    <td>Integer</td>
    <td>Displays the end of level statistics for the given level number.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>loop_cine</code>
    </td>
    <td colspan="2" align="center">N/A</td>
    <td>Plays the cinematic loop.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>loop_game</code>
    </td>
    <td colspan="2" align="center">N/A</td>
    <td>Plays the main game loop.</td>
  </tr>
  <tr valign="top">
    <td rowspan="3">
      <code>mesh_swap</code>
    </td>
    <td>
      <code>object1_id</code>
    </td>
    <td>Integer</td>
    <td rowspan="3">Swaps the given mesh ID between the given objects.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>object2_id</code>
    </td>
    <td>Integer</td>
  </tr>
  <tr valign="top">
    <td>
      <code>mesh_id</code>
    </td>
    <td>Integer</td>
  </tr>
  <tr valign="top">
    <td>
      <code>play_fmv</code>
    </td>
    <td>
      <code>fmv_path</code>
    </td>
    <td>String</td>
    <td>Plays the specified FMV file.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>play_synced_audio</code>
    </td>
    <td>
      <code>audio_id</code>
    </td>
    <td>Integer</td>
    <td>Plays the given audio track.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>remove_ammo</code>
    </td>
    <td colspan="2" align="center">N/A</td>
    <td rowspan="4">
      Any combination of these four sequences can be used to modify Lara's
      inventory at the start of a level. There are a few simple points to note:
      <ul>
        <li>
          If they are specified, they must appear <em>before</em> the
          <code>start_game</code> sequence.
        </li>
        <li>
          <code>remove_guns</code> does not remove the ammo for those guns, and
          equally <code>remove_ammo</code> does not remove the guns. Each works
          independently of the other.
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
    <td>
      <code>remove_guns</code>
    </td>
    <td colspan="2" align="center">N/A</td>
  </tr>
  <tr valign="top">
    <td>
      <code>remove_medipacks</code>
    </td>
    <td colspan="2" align="center">N/A</td>
  </tr>
  <tr valign="top">
    <td>
      <code>remove_scions</code>
    </td>
    <td colspan="2" align="center">N/A</td>
  </tr>
  <tr valign="top">
    <td>
      <code>set_cam_angle</code>
    </td>
    <td>
      <code>value</code>
    </td>
    <td>Integer</td>
    <td>Sets the camera's angle.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>set_cam_x</code>
    </td>
    <td>
      <code>value</code>
    </td>
    <td>Integer</td>
    <td>Sets the camera's X position.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>set_cam_y</code>
    </td>
    <td>
      <code>value</code>
    </td>
    <td>Integer</td>
    <td>Sets the camera's Y position.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>set_cam_z</code>
    </td>
    <td>
      <code>value</code>
    </td>
    <td>Integer</td>
    <td>Sets the camera's Z position.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>setup_bacon_lara</code>
    </td>
    <td>
      <code>anchor_room</code>
    </td>
    <td>Integer</td>
    <td>
      Sets the room number in which Bacon Lara will be anchored to enable
      correct mirroring behaviour with Lara.
    </td>
  </tr>

  <tr valign="top">
    <td>
      <code>start_cine</code>
    </td>
    <td colspan="2" align="center">N/A</td>
    <td>Starts a new cinematic level.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>start_game</code>
    </td>
    <td colspan="2" align="center">N/A</td>
    <td>Starts a new level.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>stop_game</code>
    </td>
    <td colspan="2" align="center">N/A</td>
    <td>Ends a level.</td>
  </tr>
  <tr valign="top">
    <td>
      <code>total_stats</code>
    </td>
    <td>
      <code>picture_path</code>
    </td>
    <td>String</td>
    <td>
      Displays the end of game statistics with the given picture file shown as
      a background.
    </td>
  </tr>
</table>

## Bonus levels
The gameflow supports bonus levels, which are unlocked only when the player
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
    // gym level definition
},

{
    "title": "Level 1",
    "file": "data/level1.phd",
    "type": "normal",
    "music": 57,
    "sequence": [
         {"type": "start_game"},
         {"type": "loop_game"},
         {"type": "stop_game"},
         {"type": "level_stats", "level_id": 1},
         {"type": "exit_to_level", "level_id": 2},
    ],
    "strings": {},
},

{
    "title": "Level 2",
    "file": "data/level2.phd",
    "type": "normal",
    "music": 57,
    "sequence": [
         {"type": "start_game"},
         {"type": "loop_game"},
         {"type": "stop_game"},
         {"type": "level_stats", "level_id": 2},
         {"type": "exit_to_level", "level_id": 3},
    ],
    "strings": {},
},

{
    "title": "Level 3",
    "file": "data/level3.phd",
    "type": "normal",
    "music": 57,
    "sequence": [
         {"type": "start_game"},
         {"type": "loop_game"},
         {"type": "stop_game"},
         {"type": "level_stats", "level_id": 3},               
         {"type": "play_synced_audio", "audio_id": 19},
         {"type": "display_picture", "picture_path": "data/end.pcx", "display_time": 7.5},
         {"type": "display_picture", "picture_path": "data/cred1.pcx", "display_time": 7.5},
         {"type": "display_picture", "picture_path": "data/cred2.pcx", "display_time": 7.5},
         {"type": "display_picture", "picture_path": "data/cred3.pcx", "display_time": 7.5},
         {"type": "total_stats", "picture_path": "data/install.pcx"},
         {"type": "exit_to_level", "level_id": 4},
    ],
    "strings": {},
},

{
    "title": "Level 4",
    "file": "data/bonus1.phd",
    "type": "bonus",
    "music": 57,
    "sequence": [
         {"type": "play_fmv", "fmv_path": "fmv/snow.avi"},
         {"type": "start_game"},
         {"type": "loop_game"},
         {"type": "stop_game"},
         {"type": "exit_to_cine", "level_id": 6},
    ],
    "strings": {},
},

{
    "title": "Level 5",
    "file": "data/bonus2.phd",
    "type": "bonus",
    "music": 57,
    "sequence": [
         {"type": "start_game"},
         {"type": "loop_game"},
         {"type": "stop_game"},
         {"type": "level_stats", "level_id": 5},
         {"type": "play_synced_audio", "audio_id": 14},
         {"type": "total_stats", "picture_path": "data/install.pcx"},
         {"type": "exit_to_title"},
    ],
    "strings": {},
},

{
    "title": "Bonus Cut Scene",
    "file": "data/bonuscut1.phd",
    "type": "cutscene",
    "music": 0,
    "sequence": [
        {"type": "start_cine"},
        {"type": "set_cam_x", "value": 36668},
        {"type": "set_cam_z", "value": 63180},
        {"type": "set_cam_angle", "value": -23312},
        {"type": "play_synced_audio", "audio_id": 23},
        {"type": "loop_cine"},
        {"type": "level_stats", "level_id": 4},
        {"type": "exit_to_level", "level_id": 5},
    ],
    "strings": {},
},
```
</details>

## Item drops
In the original game, items dropped by enemies were hardcoded such that only
specific enemies could drop, and the items and quantities that they dropped were
immutable. This is no longer the case, with the gameflow providing a mechanism
to allow the _majority_ of enemy types to carry and drop items. Note that this
also means by default that the original enemies who did drop items will not do
so unless the gameflow has been configured as such.

Item drops are defined in the `item_drops` section of a level's definition by
creating objects with the following parameter structure. You can define at most
one entry per enemy, but that definition can have as many drop items as
necessary (within the engine's overall item limit).

<details>
<summary>Show example setup</summary>

```json5
{
    "title": "Example Level",
    "file": "data/example.phd",
    "type": "normal",
    "music": 57,
    "item_drops": [
        {"enemy_num": 17, "object_ids": [86]},
        {"enemy_num": 50, "object_ids": [87]},
        {"enemy_num": 12, "object_ids": [93, 93]},
        {"enemy_num": 47, "object_ids": [111]},
    ],
    "sequence": [
         {"type": "start_game"},
         {"type": "loop_game"},
         {"type": "stop_game"},
         {"type": "level_stats", "level_id": 1},
         {"type": "exit_to_level", "level_id": 2},
    ],
    "strings": {
        "key1": "Silver Key",
        "puzzle2": "Machine Cog",
    },
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
    <td>Integer array</td>
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
enemies as well as Atlantean pods (objects 163, 181) and centaur statues
(object 161). For pods, the items will be allocated to the creature within
(obviously empty pods are excluded).

Items dropped by flying or swimming creatures will fall to the ground.

For clarity, following is a list of all TR1 enemy type IDs, which you can
reference when building your gameflow. The gameflow will ignore drops for
non-enemy type objects, and a suitable warning message will be produced in the
log file.

<table>
  <tr valign="top" align="left">
    <th>Object ID
    <th>Name</th>
  </tr>
  <tr>
    <td>7</td>
    <td>Wolf</td>
  </tr>
  <tr>
    <td>8</td>
    <td>Bear</td>
  </tr>
  <tr>
    <td>9</td>
    <td>Bat</td>
  </tr>
  <tr>
    <td>10</td>
    <td>Crocodile</td>
  </tr>
  <tr>
    <td>11</td>
    <td>Alligator</td>
  </tr>
  <tr>
    <td>12</td>
    <td>Lion</td>
  </tr>
  <tr>
    <td>13</td>
    <td>Lioness</td>
  </tr>
  <tr>
    <td>14</td>
    <td>Puma</td>
  </tr>
  <tr>
    <td>15</td>
    <td>Ape</td>
  </tr>
  <tr>
    <td>16</td>
    <td>Rat</td>
  </tr>
  <tr>
    <td>17</td>
    <td>Vole</td>
  </tr>
  <tr>
    <td>18</td>
    <td>T-rex</td>
  </tr>
  <tr>
    <td>19</td>
    <td>Raptor</td>
  </tr>
  <tr>
    <td>20</td>
    <td>Flying mutant</td>
  </tr>
  <tr>
    <td>21</td>
    <td>Grounded mutant (shooter)</td>
  </tr>
  <tr>
    <td>22</td>
    <td>Grounded mutant (non-shooter)</td>
  </tr>
  <tr>
    <td>23</td>
    <td>Centaur</td>
  </tr>
  <tr>
    <td>24</td>
    <td>Mummy (Tomb of Qualopec)</td>
  </tr>
  <tr>
    <td>27</td>
    <td>Larson</td>
  </tr>
  <tr>
    <td>28</td>
    <td>Pierre (not runaway)</td>
  </tr>
  <tr>
    <td>30</td>
    <td>Skate kid</td>
  </tr>
  <tr>
    <td>31</td>
    <td>Cowboy</td>
  </tr>
  <tr>
    <td>32</td>
    <td>Kold</td>
  </tr>
  <tr>
    <td>33</td>
    <td>Natla (items drop after second phase)</td>
  </tr>
  <tr>
    <td>34</td>
    <td>Torso</td>
  </tr>
</table>

### Item validity
The following object types are capable of being carried and dropped. The
gameflow will ignore anything that is not in this list, and a suitable warning
message will be produced in the log file.

<table>
  <tr valign="top" align="left">
    <th>Object ID</th>
    <th>Name</th>
  </tr>
  <tr>
    <td>84</td>
    <td>Pistols</td>
  </tr>
  <tr>
    <td>85</td>
    <td>Shotgun</td>
  </tr>
  <tr>
    <td>86</td>
    <td>Magnums</td>
  </tr>
  <tr>
    <td>87</td>
    <td>Uzis</td>
  </tr>
  <tr>
    <td>89</td>
    <td>Shotgun ammo</td>
  </tr>
  <tr>
    <td>90</td>
    <td>Magnum ammo</td>
  </tr>
  <tr>
    <td>91</td>
    <td>Uzi ammo</td>
  </tr>
  <tr>
    <td>93</td>
    <td>Small medipack</td>
  </tr>
  <tr>
    <td>94</td>
    <td>Large medipack</td>
  </tr>
  <tr>
    <td>110</td>
    <td>Puzzle1</td>
  </tr>
  <tr>
    <td>111</td>
    <td>Puzzle2</td>
  </tr>
  <tr>
    <td>112</td>
    <td>Puzzle3</td>
  </tr>
  <tr>
    <td>113</td>
    <td>Puzzle4</td>
  </tr>
  <tr>
    <td>126</td>
    <td>Lead bar</td>
  </tr>
  <tr>
    <td>129</td>
    <td>Key1</td>
  </tr>
  <tr>
    <td>130</td>
    <td>Key2</td>
  </tr>
  <tr>
    <td>131</td>
    <td>Key3</td>
  </tr>
  <tr>
    <td>132</td>
    <td>Key4</td>
  </tr>
  <tr>
    <td>141</td>
    <td>Pickup1</td>
  </tr>
  <tr>
    <td>142</td>
    <td>Pickup2</td>
  </tr>
  <tr>
    <td>144</td>
    <td>Scion (à la Pierre)</td>
  </tr>
</table>

## Injections
Injections defined in the global gameflow will by default be applied to each
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

The gameflow will ignore referenced injection files that do not exist, but it is
best practice to remove the references to maintain a clean gameflow file.

Following is a summary of what each of the default injection files that are
provided with the game achieves.

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
    <td rowspan="3">
      Injects a braid when the option for it is enabled. This also edits Lara's
      head meshes (object 0 and object 4) to make the braid fit better. A golden
      braid is also provided for the Midas Touch animation. Again, different
      files are needed to cater for mesh differences between cutscene and normal
      levels.
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
      Injects several animations for Lara, such as the jump-twist, somersault,
      and underwater roll.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>lara_jumping.bin</code>
    </td>
    <td>
      Injects animation change/dispatch edits for Lara to cater for responsive
      jumping, if that option is enabled.
    </td>
  </tr>
</table>
