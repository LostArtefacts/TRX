---
title: Regular levels
---

## Regular levels

The `levels` section of the document defines how the game plays out. This is an
array of objects and can be defined in any order. The flow is controlled using
the correct [sequencing](../2-SEQUENCES.md) within each level itself.

Following are each of the properties available within a level.

<details>
<summary>Show snippet</summary>

```json5
{
    "path": "data/example.phd",
    // Optional level Lua script file
    "script": "data/scripts/level1.lua",
    "music_track": 57,
    "lara_type": 0,
    "water_color": [0.7, 0.5, 0.85],
    "cold_water": true,
    "fog_transparency": false,
    "fog_color": [0, 0, 0],
    "fog_start": 34.0,
    "fog_end": 50.0,
    "unobtainable_pickups": 1,
    "unobtainable_kills": 1,
    "inherit_injections": false,
    "injections": [
        "data/level_injection1.bin",
        "data/level_injection2.bin",
    ],
    "ambient_tracks": [30, 31, 32, 33],
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
    <td><code>script</code></td>
    <td>String</td>
    <td colspan="2">Path to a Lua script executed after loading this level.</td>
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
      <a href="./5-BONUS_LEVELS.md">Bonus levels</a> for full details.
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
      <a href="../2-SEQUENCES.md">Sequences</a> for full details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>music_track</code></td>
    <td>Integer<strong>*</strong></td>
    <td colspan="2">The ambient music track ID.</td>
  </tr>
  <tr valign="top">
    <td>
      <a name="cold-water"></a>
      <code>cold_water</code>
    </td>
    <td>Boolean</td>
    <td colspan="2">
      Can be customized per level. See <a href="../0-GLOBAL_PROPERTIES.md#cold-water">the global property</a>
      for details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>fog_transparency</code></td>
    <td>Boolean</td>
    <td colspan="2">
      Can be customized per level. See <a href="../0-GLOBAL_PROPERTIES.md#fog-transparency">the global property</a>
      for details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>fog_color</code></td>
    <td>Float array or hex string</td>
    <td colspan="2">
      Can be customized per level. See <a href="../0-GLOBAL_PROPERTIES.md#fog-color">the global property</a>
      for details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>fog_start</code></td>
    <td>Double</td>
    <td colspan="2">
      Can be customized per level. See <a href="../0-GLOBAL_PROPERTIES.md#draw-distance-fade">the global property</a>
      for details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>fog_end</code></td>
    <td>Double</td>
    <td colspan="2">
      Can be customized per level. See <a href="../0-GLOBAL_PROPERTIES.md#draw-distance-max">the global property</a>
      for details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>injections</code></td>
    <td>String array</td>
    <td colspan="2">
      Injection file paths. See <a href="../5-INJECTIONS.md">Injections</a> for full
      details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>inherit_injections</code></td>
    <td>Boolean</td>
    <td colspan="2">
      A flag to indicate whether or not the level should use the globally
      defined injections. See <a href="../5-INJECTIONS.md">Injections</a> for full
      details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>item_drops</code><strong>¹</strong></td>
    <td>Object array</td>
    <td colspan="2">
      Instructions to allocate items to enemies who will drop those items when
      killed. See <a href="./4-ITEM_DROPS.md">Item drops</a> for full details.
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
    <td><code>sfx_path</code><strong>²</strong></td>
    <td>String</td>
    <td colspan="2">
      The path to the sound effects (.sfx) file to use in this level. If this
      property is not defined, the default global file will be used.
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
    <td>Float array or hex string</td>
    <td colspan="2">
      Can be customized per level. See <a href="../0-GLOBAL_PROPERTIES.md#water-color">the global property</a> for
      details.
    </td>
  </tr>
  <tr valign="top">
    <td><code>ambient_tracks</code></td>
    <td>Integer array</td>
    <td colspan="2">
      Can be customized per level. See <a href="../0-GLOBAL_PROPERTIES.md#ambient-tracks">the global property</a> for
      details.
    </td>
  </tr>
</table>

**\*** Required property.  
**¹** Tomb Raider 1 only.
**²** Tomb Raider 2 only.
