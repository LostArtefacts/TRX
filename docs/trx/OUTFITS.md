---
title: Lara's outfits
order: 16
---

# Outfits

In TR1 and TR2 originally, Lara's meshes were taken from the `O_LARA` object,
with mesh swaps being performed as required at runtime using additional objects,
such as `O_LARA_PISTOL`, `O_LARA_SHOTGUN`, `O_LARA_EXTRA` etc. TR3 moved to a
dedicated `O_LARA_SKIN` object, but still depended on the additional gun and
extra mesh swaps, and these remained tightly coupled with the level's outfit.
For example, when putting a shotgun in Lara's hand, the relevant mesh would
include an entire copy of her hand, when in reality only the shotgun was
required. This meant if customizing Lara's gloves, the builder would need to do
so on several different objects.

TRX uses a different skin system, both to allow outfit swaps in-game for players
and to remove unnecessary mesh faces where applicable for a more streamlined
data setup. Custom level builders can define up to 32 outfits; following is a
guide to the data and JSON configuration, and some scenario/workflow examples.

## Data setup

The skin system uses the following objects. These are provided in the
`lara_outfits.bin` injection, and are available to download as a separate WAD
(see [injections](INJECTIONS.md#common-steps-for-importing-trx-assets-into-your-wad-wadtool)).

#### `O_LARA_SKIN_SWAP_1`...`O_LARA_SKIN_SWAP_32`
Each of these should contain a distinct Lara model, with the mesh count and bone
order conforming to the standard for Lara. Bone offsets are used (e.g. consider
Bacon Lara's different structure); animations are not used.

#### `O_LARA_SKIN_JOINTS_1`...`O_LARA_SKIN_JOINTS_32`
Each of these should contain a distinct Lara joints model. The data setup is
identical to TR4/5, so joint vertices should map to the corresponding meshes
that they stitch together.

#### `O_LARA_SKIN_SWAP_EXTRA`
This object contains various additional meshes for Lara, such as altered torsos
when the TR1 braid is in use, Lara's combat face, and meshes used in extra
animations, such as pulling the dagger in Dragon's Lair. It also contains both
the TR1 and TR2/3 braid.

#### `O_LARA_SKIN_SWAP_GUNS`
This object contains holsters - both empty and equipped with the various guns - 
as well as the guns themselves when they are in Lara's hands or on her back.

#### `O_LARA_SKIN_SWAP_LEGS`
This object contains copies of Lara's legs for each outfit, with holster strap
textures removed. This allows levels such as Home Sweet Home to swap out Lara's
legs when she has no holsters. It is not an essential object to include.

### Which slot holds which outfit

<!-- gen:outfit-slots -->
Each game gives these objects a different slot number. Import into
the slot listed for the game you are building for.

<table>
  <tr valign='top' align='left'>
    <th>Outfit</th>
    <th>Object</th>
    <th>TR1</th>
    <th>TR2</th>
    <th>TR3</th>
    <th>TR4</th>
  </tr>
  <tr valign='top'>
    <td><code>tr1_gym</code></td>
    <td><code>O_LARA_SKIN_SWAP_1</code></td>
    <td>258</td>
    <td>302</td>
    <td>393</td>
    <td>465</td>
  </tr>
  <tr valign='top'>
    <td><code>tr1_classic</code></td>
    <td><code>O_LARA_SKIN_SWAP_2</code></td>
    <td>259</td>
    <td>303</td>
    <td>394</td>
    <td>466</td>
  </tr>
  <tr valign='top'>
    <td><code>tr1_mauled</code></td>
    <td><code>O_LARA_SKIN_SWAP_3</code></td>
    <td>260</td>
    <td>304</td>
    <td>395</td>
    <td>467</td>
  </tr>
  <tr valign='top'>
    <td><code>tr1_combo</code></td>
    <td><code>O_LARA_SKIN_SWAP_4</code></td>
    <td>261</td>
    <td>305</td>
    <td>396</td>
    <td>468</td>
  </tr>
  <tr valign='top'>
    <td><code>tr1_bacon_lara</code></td>
    <td><code>O_LARA_SKIN_SWAP_5</code></td>
    <td>262</td>
    <td>306</td>
    <td>397</td>
    <td>469</td>
  </tr>
  <tr valign='top'>
    <td><code>tr2_gym</code></td>
    <td><code>O_LARA_SKIN_SWAP_6</code></td>
    <td>263</td>
    <td>307</td>
    <td>398</td>
    <td>470</td>
  </tr>
  <tr valign='top'>
    <td><code>tr2_classic</code></td>
    <td><code>O_LARA_SKIN_SWAP_7</code></td>
    <td>264</td>
    <td>308</td>
    <td>399</td>
    <td>471</td>
  </tr>
  <tr valign='top'>
    <td><code>tr2_diving_suit</code></td>
    <td><code>O_LARA_SKIN_SWAP_8</code></td>
    <td>265</td>
    <td>309</td>
    <td>400</td>
    <td>472</td>
  </tr>
  <tr valign='top'>
    <td><code>tr2_bomber_jacket</code></td>
    <td><code>O_LARA_SKIN_SWAP_9</code></td>
    <td>266</td>
    <td>310</td>
    <td>401</td>
    <td>473</td>
  </tr>
  <tr valign='top'>
    <td><code>tr2_robe</code></td>
    <td><code>O_LARA_SKIN_SWAP_10</code></td>
    <td>267</td>
    <td>311</td>
    <td>402</td>
    <td>474</td>
  </tr>
  <tr valign='top'>
    <td><code>tr2_vegas</code></td>
    <td><code>O_LARA_SKIN_SWAP_11</code></td>
    <td>268</td>
    <td>312</td>
    <td>403</td>
    <td>475</td>
  </tr>
  <tr valign='top'>
    <td><code>tr3_gym</code></td>
    <td><code>O_LARA_SKIN_SWAP_12</code></td>
    <td>269</td>
    <td>313</td>
    <td>404</td>
    <td>476</td>
  </tr>
  <tr valign='top'>
    <td><code>tr3_classic</code></td>
    <td><code>O_LARA_SKIN_SWAP_13</code></td>
    <td>270</td>
    <td>314</td>
    <td>405</td>
    <td>477</td>
  </tr>
  <tr valign='top'>
    <td><code>tr3_south_pacific</code></td>
    <td><code>O_LARA_SKIN_SWAP_14</code></td>
    <td>271</td>
    <td>315</td>
    <td>406</td>
    <td>478</td>
  </tr>
  <tr valign='top'>
    <td><code>tr3_catsuit</code></td>
    <td><code>O_LARA_SKIN_SWAP_15</code></td>
    <td>272</td>
    <td>316</td>
    <td>407</td>
    <td>479</td>
  </tr>
  <tr valign='top'>
    <td><code>tr3_nevada</code></td>
    <td><code>O_LARA_SKIN_SWAP_16</code></td>
    <td>273</td>
    <td>317</td>
    <td>408</td>
    <td>480</td>
  </tr>
  <tr valign='top'>
    <td><code>tr3_antarctica</code></td>
    <td><code>O_LARA_SKIN_SWAP_17</code></td>
    <td>274</td>
    <td>318</td>
    <td>409</td>
    <td>481</td>
  </tr>
  <tr valign='top'>
    <td><code>sophia</code></td>
    <td><code>O_LARA_SKIN_SWAP_18</code></td>
    <td>275</td>
    <td>319</td>
    <td>410</td>
    <td>482</td>
  </tr>
  <tr valign='top'>
    <td><code>tr2_diving_suit_alpha</code></td>
    <td><code>O_LARA_SKIN_SWAP_19</code></td>
    <td>276</td>
    <td>320</td>
    <td>411</td>
    <td>483</td>
  </tr>
  <tr valign='top'>
    <td><code>tr1_ngage</code></td>
    <td><code>O_LARA_SKIN_SWAP_20</code></td>
    <td>277</td>
    <td>321</td>
    <td>412</td>
    <td>484</td>
  </tr>
  <tr valign='top'>
    <td><code>tr3_antarctica_beta</code></td>
    <td><code>O_LARA_SKIN_SWAP_21</code></td>
    <td>278</td>
    <td>322</td>
    <td>413</td>
    <td>485</td>
  </tr>
  <tr valign='top'>
    <td><code>tr2_bomber_jacket_alpha</code></td>
    <td><code>O_LARA_SKIN_SWAP_22</code></td>
    <td>279</td>
    <td>323</td>
    <td>414</td>
    <td>486</td>
  </tr>
  <tr valign='top'>
    <td><code>tr4_young</code></td>
    <td><code>O_LARA_SKIN_SWAP_23</code></td>
    <td>280</td>
    <td>324</td>
    <td>415</td>
    <td>487</td>
  </tr>
  <tr valign='top'>
    <td><code>tr4_classic</code></td>
    <td><code>O_LARA_SKIN_SWAP_24</code></td>
    <td>281</td>
    <td>325</td>
    <td>416</td>
    <td>488</td>
  </tr>
  <tr valign='top'>
    <td><code>natla</code></td>
    <td><code>O_LARA_SKIN_SWAP_25</code></td>
    <td>282</td>
    <td>326</td>
    <td>417</td>
    <td>489</td>
  </tr>
</table>

Only the outfits below have a joints model. The rest need none.

<table>
  <tr valign='top' align='left'>
    <th>Outfit</th>
    <th>Object</th>
    <th>TR1</th>
    <th>TR2</th>
    <th>TR3</th>
    <th>TR4</th>
  </tr>
  <tr valign='top'>
    <td><code>tr4_young</code> joints</td>
    <td><code>O_LARA_SKIN_JOINTS_1</code></td>
    <td>296</td>
    <td>340</td>
    <td>433</td>
    <td>502</td>
  </tr>
  <tr valign='top'>
    <td><code>tr4_classic</code> joints</td>
    <td><code>O_LARA_SKIN_JOINTS_2</code></td>
    <td>297</td>
    <td>341</td>
    <td>434</td>
    <td>503</td>
  </tr>
</table>

The objects below are used by all outfits.

<table>
  <tr valign='top' align='left'>
    <th>Purpose</th>
    <th>Object</th>
    <th>TR1</th>
    <th>TR2</th>
    <th>TR3</th>
    <th>TR4</th>
  </tr>
  <tr valign='top'>
    <td>Braids, combat faces and extra animation meshes</td>
    <td><code>O_LARA_SKIN_SWAP_EXTRA</code></td>
    <td>290</td>
    <td>334</td>
    <td>425</td>
    <td>497</td>
  </tr>
  <tr valign='top'>
    <td>Holsters and the guns themselves</td>
    <td><code>O_LARA_SKIN_SWAP_GUNS</code></td>
    <td>291</td>
    <td>335</td>
    <td>426</td>
    <td>498</td>
  </tr>
  <tr valign='top'>
    <td>Legs without holster straps; optional</td>
    <td><code>O_LARA_SKIN_SWAP_LEGS</code></td>
    <td>292</td>
    <td>336</td>
    <td>427</td>
    <td>499</td>
  </tr>
</table>
<!-- /gen:outfit-slots -->

## JSON setup
The file `cfg/outfits.json5` sets up the available outfits, and how they should
behave. The structure of this file is described below.

#### Top-level overview
<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>outfits</code></td>
    <td>Object map</td>
    <td>
      The keys in this map define the available outfits, and these are the keys
      that should be used in the game-flow. The outfit object is described
      separately below.
    </td>
  </tr>
  <tr valign="top">
    <td><code>extra_meshes</code></td>
    <td>Integer map</td>
    <td>
      This map defines mesh offsets in <code>O_LARA_SKIN_SWAP_EXTRA</code>,
      which are required for various events/paths in the engine.
    </td>
  </tr>
  <tr valign="top">
    <td><code>gun_maps</code></td>
    <td>Object array</td>
    <td>
      These maps dictate which meshes to use in
      <code>O_LARA_SKIN_SWAP_GUNS</code> for a given outfit and gun combination.
    </td>
  </tr>
</table>

### Outfits

<details>
<summary>Show snippet</summary>

```json
"tr1_classic": {
  "name_gs": "dynamic/enums/lara_outfit/tr1_classic",
  "mesh_object": "O_LARA_SKIN_SWAP_2",
  "is_barefoot": false,
  "gun_map": 0,
  "combat_face_offset": 1,
  "supports_sunglasses": true,
  "braid": [
    {
      "mode": "BRAID_MODE_TR1_FULL",
      "mesh_offset": 10,
      "position": {
        "x": 0,
        "y": 20,
        "z": -45,
      },
    },
  ],
  "no_holster_offsets": {
    "thigh_r": 1,
    "thigh_l": 2,
  },
  "extra_outfits": {
    "LS_EXTRA_TREX_KILL": "tr1_mauled",
  },
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
    <td><code>name_gs</code></td>
    <td>String</td>
    <td colspan="2">
      The game string key used for localized UI labels for this outfit
      (for example, <code>dynamic/enums/lara_outfit/tr1_classic</code>).
    </td>
  </tr>
  <tr valign="top">
    <td><code>mesh_object</code></td>
    <td>String</td>
    <td colspan="2">
     Indicates which object contains the outfit's meshes and bones.
    </td>
  </tr>
  <tr valign="top">
    <td><code>joints_object</code></td>
    <td>String</td>
    <td colspan="2">
     Optional; indicates which object contains the joints for this outfit.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>extra_object</code><br/>
      <code>guns_object</code><br/>
      <code>legs_object</code>
    </td>
    <td>String</td>
    <td colspan="2">
     Optional; indicate which objects hold this outfit's extra, gun and leg
     meshes. They default to <code>O_LARA_SKIN_SWAP_EXTRA</code>,
     <code>O_LARA_SKIN_SWAP_GUNS</code> and <code>O_LARA_SKIN_SWAP_LEGS</code>,
     which is what an outfit sharing the shipped data wants.
    </td>
  </tr>
  <tr valign="top">
    <td><code>gold_color</code></td>
    <td>Color</td>
    <td colspan="2">
      The color Lara is cast in when she turns to gold, whether by the Midas
      hand or by the golden Lara setting. It defaults to the gold the shipped
      models are textured in.
    </td>
  </tr>
  <tr valign="top">
    <td rowspan="5"><code>is_barefoot</code></td>
    <td rowspan="5">Boolean</td>
    <td colspan="2">
      Indicates whether or not Lara is barefoot and is used to play different
      SFX for her footsteps. The following samples need to be defined in
      <code>catalog_samples.csv</code>.
    </td>
  </tr>
  <tr valign="top">
    <td><code>SFX_LARA_FOOTSTEP</code></td>
    <td>Regular step sound when walking, running etc.</td>
  </tr>
  <tr valign="top">
    <td><code>SFX_LARA_BAREFOOT</code></td>
    <td>Barefoot step sound when walking, running etc.</td>
  </tr>
  <tr valign="top">
    <td><code>SFX_LARA_LAND</code></td>
    <td>Regular sound when landing e.g. after a jump.</td>
  </tr>
  <tr valign="top">
    <td><code>SFX_LARA_BAREFOOT_LAND</code></td>
    <td>Barefoot sound when landing e.g. after a jump.</td>
  </tr>
  <tr valign="top">
    <td><code>gun_map</code></td>
    <td>Integer</td>
    <td colspan="2">
      The index into the <code>gun_maps</code> array to use for this outfit.
    </td>
  </tr>
  <tr valign="top">
    <td><code>braid</code></td>
    <td>Object array</td>
    <td colspan="2">
      The braid setup specific to this outfit. At most two braids can be
      defined. If omitted, no braid will be shown. See the braids section below.
    </td>
  </tr>
  <tr valign="top">
    <td><code>combat_face_offset</code></td>
    <td>Integer</td>
    <td colspan="2">
      The mesh offset in <code>O_LARA_SKIN_SWAP_EXTRA</code> for Lara's combat
      face. <code>-1</code> implies no combat face swap. This mesh is used when
      Lara is firing a weapon (traditionally, the <code>O_LARA_UZI</code> head
      mesh was used).
    </td>
  </tr>
  <tr valign="top">
    <td><code>supports_sunglasses</code></td>
    <td>Boolean</td>
    <td colspan="2">
      Defines whether or not sunglasses can be used with this outfit.
    </td>
  </tr>
  <tr valign="top">
    <td><code>no_holster_offsets</code></td>
    <td>Integer map</td>
    <td colspan="2">
      The mesh offsets in <code>O_LARA_SKIN_SWAP_LEGS</code> to use when Lara's
      holsters aren't visible. Omitting this property infers no mesh swaps.
    </td>
  </tr>
  <tr valign="top">
    <td rowspan="2"><code>extra_outfits</code></td>
    <td rowspan="2">String map</td>
    <td colspan="2">
      Pointers to alternative outfits to use for specific game events. One
      event is supported. If it is omitted, no swap will occur for it.
    </td>
  </tr>
  <tr valign="top">
    <td><code>LS_EXTRA_TREX_KILL</code></td>
    <td>When Lara is killed by the T-rex - instant full outfit swap.</td>
  </tr>
  <tr valign="top">
    <td><code>extra_mesh_positions</code></td>
    <td>XYZ map</td>
    <td colspan="2">
      Positional offsets for Lara's extra equipment meshes, allowing adjustments
      to be made per outfit. Used by default in the TR4 outfits to adjust Lara's
      sunglasses.
    </td>
  </tr>
</table>

### Braids

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th colspan="2">Description</th>
  </tr>
  <tr valign="top" >
    <td rowspan="5"><code>mode</code></td>
    <td rowspan="5">String</td>
    <td colspan="2">Indicates special handling when the braid is active.</td>
  </tr>
  <tr valign="top">
    <td><code>BRAID_MODE_NONE</code></td>
    <td>
      No special treatment (this mode is implied if <code>mode</code> is not
      specified).
    </td>
  </tr>
  <tr valign="top">
    <td><code>BRAID_MODE_TR1_HEAD_ONLY</code></td>
    <td>
      Replaces Lara's head with <code>EXTRA_MESH_TR1_BRAID_DEFAULT_HEAD</code>
      defined in the <code>O_LARA_SKIN_SWAP_EXTRA</code> object.
    </td>
  </tr>
  <tr valign="top">
    <td><code>BRAID_MODE_TR1_FULL</code></td>
    <td>
      As per <code>BRAID_MODE_TR1_HEAD_ONLY</code>, plus Lara's torso will be
      replaced with <code>EXTRA_MESH_TR1_BRAID_DEFAULT_TORSO</code>.
    </td>
  </tr>
  <tr valign="top">
    <td><code>BRAID_MODE_TR1_MAULED</code></td>
    <td>
      As per <code>BRAID_MODE_TR1_FULL</code>, but the torso swap mesh used here
      is <code>EXTRA_MESH_TR1_BRAID_MAULED_TORSO</code>.
    </td>
  </tr>
  <tr valign="top">
    <td><code>auto_enabled</code></td>
    <td>Boolean</td>
    <td colspan="2">
      Whether or not the braid is enabled by default when the player chooses
      <code>Auto</code> for Lara's braid status. Defaults to <code>true</code>;
      typically set to <code>false</code> for TR1 outfits.
    </td>
  </tr>
  <tr valign="top">
    <td><code>mesh_offset</code></td>
    <td>Integer</td>
    <td colspan="2">
      The starting offset in <code>O_LARA_SKIN_SWAP_EXTRA</code> for the regular
      braid meshes and bones.
    </td>
  </tr>
  <tr valign="top">
    <td><code>position</code></td>
    <td>XYZ</td>
    <td colspan="2">
      The position relative to Lara's head where the braid will be drawn.
    </td>
  </tr>
  <tr valign="top">
    <td><code>head_seam</code></td>
    <td>Array of [braid vertex, head vertex] pairs</td>
    <td colspan="2">
      The vertex mapping used to stitch the braid to Lara's head. This is only
      applicable when the outfit itself is jointed.
    </td>
  </tr>
</table>

### Guns

<details>
<summary>Show snippet</summary>

```json
{
  "LGT_DESERT_EAGLE": {
    "hand_r": 62,
    "thigh_r": 9,
  },
  "LGT_UZIS": {
    "hand_r": 63,
    "hand_l": 64,
    "thigh_r": 10,
    "thigh_l": 11,
  },
  "LGT_SHOTGUN": {
    "hand_r": 65,
    "torso": 72,
  },
}
```
</details>

The map keys must match known engine weapons. See [weapons](WEAPONS.md) and
`cfg/weapons.json5` for reference. Any entry may omit hand, thigh or torso, and
note that specific gun types will only look for particular entries. For example,
defining a `thigh_l` property for `LGT_SHOTGUN` is meaningless and will be
ignored. Any missing fields imply that no mesh is drawn for that slot.

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td>
      <code>hand_r</code><br/>
      <code>hand_l</code>
    </td>
    <td>Integer</td>
    <td>
      The mesh offset in <code>O_LARA_SKIN_SWAP_GUNS</code> for the gun to draw
      in Lara's hands.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>thigh_r</code><br/>
      <code>thigh_l</code>
    </td>
    <td>Integer</td>
    <td>
      The mesh offset in <code>O_LARA_SKIN_SWAP_GUNS</code> for the gun to draw
      against Lara's thighs. This expects the holster to be part of the mesh.
    </td>
  </tr>
  <tr valign="top">
    <td>
      <code>torso</code>
    </td>
    <td>Integer</td>
    <td>
      The mesh offset in <code>O_LARA_SKIN_SWAP_GUNS</code> for the gun to draw
      on Lara's back.
    </td>
  </tr>
</table>

## Custom level use cases

`lara_outfit` appears twice in a game flow, and the two are unrelated settings
that happen to share a name.

<table>
  <tr valign="top" align="left">
    <th>Where</th>
    <th>What it does</th>
  </tr>
  <tr valign="top">
    <td>On a level</td>
    <td>Names the outfit Lara wears in that level.</td>
  </tr>
  <tr valign="top">
    <td>Under <code>enforced_config</code></td>
    <td>
      Takes the choice of outfit away from the player. A value of
      <code>null</code> enforces no outfit of its own, which leaves each level
      free to name its own.
    </td>
  </tr>
</table>

Deleting one is not the same as deleting the other.

> I don't need to customize Lara's outfit, and I don't mind players freely
switching outfits.

In this scenario, simply ship your level with `lara_outfits.bin` and the default
`cfg/outfits.json5`.

***

> I don't need to customize Lara's outfit, nor do I want the player to change
it.

In this case, you can simply enforce the outfit option, and continue to ship the
standard files as above. Provided your game-flow level has an accurate
`lara_outfit` entry, you can enforce as follows.

```json
"enforced_config": {
  "lara_outfit": null,
},
```

Using null here means that the enforcement names no outfit of its own, so each
level's own `lara_outfit` is what Lara wears; a second level that names a
different one is still honoured. Naming an outfit instead of `null` dresses her
in that one everywhere, whatever the levels say.

***

> I don't need to customize Lara's outfits, but I want to restrict the ones that
can be selected by the player.

Ship the default injection file, but edit `cfg/outfits.json5` by removing the 
entries from the `outfits` section that you do not need.

***

> I want to customize Lara's outfits.

Download the provided TRX WAD to access the data included in the shipped
injection. Move each of the relevant objects to your WAD. You can then proceed
to edit the meshes as required. Ensure that you remove the `lara_outfits.bin`
from your game-flow, otherwise these will override the data in your level.

> I want to customize Lara's braid.

Braid meshes and bones are taken from the `O_LARA_SKIN_SWAP_EXTRA` object, so to
customize it you will need to follow the same steps as for customizing outfits
i.e. import the TRX WAD, edit the data and remove the injection.
