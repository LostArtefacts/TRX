---
title: Global properties
---

## Global properties
The following properties are in the root of the game flow document and control
various pieces of global behaviour. Currently, the majority of this section
remains distinct for each game.

### TR1

#### Example structure

<details>
<summary>Show snippet</summary>

```json5
{
    "main_menu_picture": "data/titleh.png",
    "savegame_fmt_legacy": "saveati.%d",
    "savegame_fmt_bson": "save_tr1_%02d.dat",
    "demo_delay": 16,
    "water_color": [0.45, 1.0, 1.0],
    "fog_transparency": false,
    "fog_color": [0.0, 0.0, 0.0],
    "fog_start": 22.0,
    "fog_end": 30.0,
    "injections": [
        "data/global_injection1.bin",
        "data/global_injection2.bin",
        // etc
    ],
    "convert_dropped_guns": false,
    "enforced_config": {
        "enable_save_crystals": false,
    },
    "hidden_config": [
        "enable_legal",
    ],
    // Optional global Lua script file
    "main_script": "data/scripts/global.lua",
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
}
```

</details>

#### Reference

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td>
      <a name="cold-water"></a>
      <code>cold_water</code>
    </td>
    <td>Boolean</td>
    <td>
      Enables an exposure meter for Lara when she is in cold water.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="convert-dropped-guns"></a>
      <code>convert_dropped_guns</code>
    </td>
    <td>Boolean</td>
    <td>
      Forces guns dropped by enemies to be converted to the equivalent ammo
      if Lara already has the gun. See
      <a href="./1-levels/4-ITEM_DROPS.md">Item drops</a> for full details.
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
      <a name="fog-transparency"></a>
      <code>fog_transparency</code>
    </td>
    <td>Boolean</td>
    <td>
      Enables blending distant geometry into skybox rather than a solid color.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="fog-color"></a>
      <code>fog_color</code>
    </td>
    <td>Float array or hex string</td>
    <td>
      Fog color (R, G, B) or `#RRGGBB`. OG uses `#000000`. Will have no effect
      if `fog_transparency` is set to true.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="draw-distance-fade"></a>
      <code>fog_start</code>
    </td>
    <td>Double</td>
    <td>
      The distance (in tiles) at which objects and the world start to fade into
      blackness.
      <ul>
        <li>The default value in OG TR1 is hardcoded to 12.</li>
        <li>The default (disabled) value in TombATI is 72.</li>
      </ul>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="draw-distance-max"></a>
      <code>fog_end</code>
    </td>
    <td>Double</td>
    <td>
      The distance (in tiles) at which objects and the world are clipped away.
      <ul>
        <li>The default value in OG TR1 is hardcoded to 20.</li>
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
      <a href="./1-levels/4-ITEM_DROPS.md">Item drops</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><a name="enforced-config"></a>
    <code>enforced_config</code></td>
    <td>String-to-object map</td>
    <td>
      This allows <em>any</em> regular game config setting to be overriden. See
      <a href="./4-USER_CONFIGURATION.md">User configuration</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><a name="hidden-config"></a>
    <code>hidden_config</code></td>
    <td>String array</td>
    <td>
      This allows <em>any</em> regular game config setting to be hidden from
      the ingame settings dialogs. See <a href="./4-USER_CONFIGURATION.md">User
      configuration</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>injections</code></td>
    <td>String array</td>
    <td>
      Global data injection file paths. Individual levels will inherit these
      unless <code>inherit_injections</code> is set to <code>false</code> on
      those levels. See <a href="./5-INJECTIONS.md">Injections</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>levels</code></td>
    <td>Object array<strong>*</strong></td>
    <td>
      This is where the individual level details are defined - see
      <a href="./1-levels/0-REGULAR_LEVELS.md">Level properties</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>main_script</code></td>
    <td>String</td>
    <td>
      Path to a global Lua script to execute after game initialization, before
      the first level loads.
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
    <td>Float array or hex string</td>
    <td>
      Water color (R, G, B) or `#RRGGBB`. 1.0 or `FF` means pass-through, 0.0
      or `00` means completely black color.
      See <a href="../7-WATER_COLORS.md">this table</a> for reference values.</a>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="ambient-tracks"></a>
      <code>ambient_tracks</code>
    </td>
    <td>Integer array</td>
    <td>
      A list of music track IDs, which will be treated as ambient music. If
      Lara crosses a trigger for any of these, it will become the current looped
      track, and will persist on save/load.
    </td>
  </tr>
</table>

**\*** Required property.

### TR2

#### Example structure

<details>
<summary>Show snippet</summary>

```json5
{
    // NOTE: bad changes to this file may result in crashes.
    // Lines starting with double slashes are comments and are ignored.

    "main_menu_picture": "data/images/title_eu.webp",
    "savegame_fmt_legacy": "savegame.%d",
    "savegame_fmt_bson": "save_tr2_%02d.dat",

    "cmd_init":           {"action": "exit_to_title"},
    "cmd_title":          {"action": "noop"},
    "cmd_death_in_demo":  {"action": "exit_to_title"},
    "cmd_death_in_game":  {"action": "noop"},
    "cmd_demo_interrupt": {"action": "exit_to_title"},
    "cmd_demo_end":       {"action": "exit_to_title"},

    "cheat_keys": true,
    "load_save_disabled": false,
    "play_any_level": false,
    "lockout_option_ring": false,
    "demo_version": false,
    "single_level": -1,

    "demo_delay": 30,
    "secret_track": 43,

    "title": {
        "path": "data/title.tr2",
        "music_track": 60,
        "sequence": [
            {"type": "display_picture", "path": "data/images/legal_eu.webp", "legal": true},
            {"type": "play_fmv", "fmv_id": 0},
            {"type": "play_fmv", "fmv_id": 1},
            {"type": "exit_to_title"},
        ],
    },

    "sfx_path": "data/main.sfx",
    "injections": [
        "data/injections/pda_model.bin",
        "data/injections/winston_model.bin",
        "data/injections/font.bin",
    ],

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
    ],

    "enforced_config": {
        enable_3d_pickups": false,
    },
    "hidden_config": [
        "enable_save_crystals",
    ],
}
```

</details>

#### Reference

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
    <td><code>main_menu_picture</code></td>
    <td>String<strong>*</strong></td>
    <td>Path to the main menu background image.</td>
  </tr>
  <tr valign="top">
    <td><code>savegame_fmt_legacy</code></td>
    <td>String<strong>*</strong></td>
    <td>Path pattern to look for the original savegame files.</td>
  </tr>
  <tr valign="top">
    <td><code>secret_track</code></td>
    <td>Integer</td>
    <td>
      Music track to play when a secret is found. -1 to not play anything.
    </td>
  </tr>
  <tr valign="top">
    <td><code>sfx_path</code></td>
    <td>String</td>
    <td>
      The path to the sound effects (.sfx) file to use in the game.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="fog-transparency"></a>
      <code>fog_transparency</code>
    </td>
    <td>Boolean</td>
    <td>
      Enables blending distant geometry into skybox rather than a solid color.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="fog-color"></a>
      <code>fog_color</code>
    </td>
    <td>Float array or hex string</td>
    <td>
      Fog color (R, G, B) or `#RRGGBB`. OG uses `#000000`. Will have no effect
      if `fog_transparency` is set to true.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="draw-distance-fade"></a>
      <code>fog_start</code>
    </td>
    <td>Double</td>
    <td>
      The distance (in tiles) at which objects and the world start to fade into
      blackness. The default value in OG TR2 is hardcoded to 12.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <a name="draw-distance-max"></a>
      <code>fog_end</code>
    </td>
    <td>Double</td>
    <td>
      The distance (in tiles) at which objects and the world are clipped away.
      The default value in OG TR2 is hardcoded to 20.
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
      <code>water_color</code>
    </td>
    <td>Float array or hex string</td>
    <td>
      Water color (R, G, B) or `#RRGGBB`. 1.0 or `FF` means pass-through, 0.0
      or `00` means completely black color.
      See <a href="../7-WATER_COLORS.md">this table</a> for reference values.</a>
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>ambient_tracks</code>
    </td>
    <td>Integer array</td>
    <td>
      A list of music track IDs, which will be treated as ambient music. If
      Lara crosses a trigger for any of these, it will become the current looped
      track, and will persist on save/load.
    </td>
  </tr>
</table>
