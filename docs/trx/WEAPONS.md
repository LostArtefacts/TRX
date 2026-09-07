---
title: Weapons
order: 13
---

# Weapons

The file `cfg/weapons.json5` says which weapons a game has, and what each one
is. A weapon the file does not name is not in the game at all. The engine
carries the code a weapon is drawn, held and fired by, and the file names which
of it the weapon uses.

The weapons the games ship with are the pistols, the magnums, the automatic
pistols, the desert eagle, the revolver, the uzis, the shotgun, the M16, the
MP5, the grenade launcher, the rocket launcher, the harpoon gun, the crossbow,
the black skidoo, and the flare, which is not strictly a weapon but is treated
as one.

An entry is keyed by the weapon and holds what the weapon is. The numbers it
carries on its own sit at the top, and everything the engine treats as a group
of its own - the objects it is made of, its ammunition, its aim - is a group
here as well.

```json5
"uzis": {
    "kind": "dual_pistols",
    "damage": 1,
    "gun_height": 650,
    "equip_key": "equip_uzis",
    "is_remembered": true,
    "objects": {
        "pickup": "uzis_item",
        "ammo": "uzis_ammo_item",
        "anim": "lara_uzis",
    },
    "ammo": {
        "box_shots": 100,
        "icon": "\\{ammo uzis}",
    },
    "aim": {
        "speed": 10,
        "accuracy": 8,
        "lock": [-60, +60, -60, +60],
    },
    "sound": {
        "fire": "lara_uzi_fire",
        "alternating": true,
    },
    "save": {
        "ammo_key": "uzis",
        "required": true,
    },
}
```

A script states the same thing in the same words, and may add a weapon of its
own; see the [Weapon module](lua/reference/WEAPONS.md).

```lua
trx.weapons.patch("uzis", { damage = 2, ammo = { box_shots = 80 } })
```

A spec is written in the units a file is written in: an angle is in degrees,
and a distance in sectors. The weapon a script reads back holds the engine's
own units instead, so a spec that says `aim.speed = 10` reads back as
`weapon.aim_speed == 1820`, and `aim.target_dist = 8.0` as
`weapon.target_dist == 8192`. Write degrees and sectors in a spec, and
`trx.math.Angle` and `trx.math.Distance` everywhere else.

An XYZ value is an array of three numbers, or a group naming all three of
`x`, `y` and `z`. A weapon that leaves the key out keeps the offset it has.

## What a weapon holds

The keys a weapon states on its own:

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>kind</code></td>
    <td>String</td>
    <td>How the weapon is held, drawn and put away, which decides the arm animations it uses and the routines it is driven by. One of <code>dual_pistols</code>, <code>single_pistol</code>, <code>rifle</code>, <code>mounted</code> or <code>flare</code>. A kind the engine does not drive, such as <code>mounted</code>, leaves the weapon to the game code that owns it.</td>
  </tr>
  <tr valign="top">
    <td><code>base</code></td>
    <td>String</td>
    <td>The weapon this one starts from, named by its key. Everything the base holds is taken except its identity, the names it is saved under, and the key that draws it.</td>
  </tr>
  <tr valign="top">
    <td><code>damage</code></td>
    <td>Integer</td>
    <td>The HP damage value to subtract from targets when struck by this weapon type.</td>
  </tr>
  <tr valign="top">
    <td><code>gun_height</code></td>
    <td>Integer</td>
    <td>Used to determine the start Y position when firing a weapon, and to determine if Lara is too far submerged in water to be able to use a weapon (other than the harpoon).</td>
  </tr>
  <tr valign="top">
    <td><code>equip_key</code></td>
    <td>String</td>
    <td>The key that draws the weapon straight away. One key draws one weapon, so a weapon that claims a key takes it from whichever weapon held it. A weapon that names no key is reached through the inventory only.</td>
  </tr>
  <tr valign="top">
    <td><code>fire</code></td>
    <td>String</td>
    <td>What the weapon does when it is fired: <code>generic</code>, <code>m16</code>, <code>grenade</code>, <code>rocket</code> or <code>harpoon</code>. A script may state a function of its own here instead, or through <code>trx.weapons.on_fire</code>.</td>
  </tr>
  <tr valign="top">
    <td><code>is_available</code></td>
    <td>Boolean</td>
    <td>Determines if a weapon can be given to Lara when using item cheats. Pickups for unavailable weapons/flares will still work normally.</td>
  </tr>
  <tr valign="top">
    <td><code>is_default</code></td>
    <td>Boolean</td>
    <td>Whether Lara starts the game with the weapon, and reaches for it when what she holds runs dry.</td>
  </tr>
  <tr valign="top">
    <td><code>is_remembered</code></td>
    <td>Boolean</td>
    <td>Whether Lara returns to the weapon after she puts away what she holds now.</td>
  </tr>
  <tr valign="top">
    <td><code>is_launcher</code></td>
    <td>Boolean</td>
    <td>Whether the weapon throws an explosive that flies on its own.</td>
  </tr>
  <tr valign="top">
    <td><code>is_machine_gun</code></td>
    <td>Boolean</td>
    <td>Whether the weapon keeps firing while the trigger is held, which also lets Lara fire it on the move.</td>
  </tr>
  <tr valign="top">
    <td><code>is_usable_underwater</code></td>
    <td>Boolean</td>
    <td>Whether Lara may bring the weapon out under water.</td>
  </tr>
  <tr valign="top">
    <td><code>wants_combat_camera</code></td>
    <td>Boolean</td>
    <td>Whether drawing the weapon swings the camera to Lara's back.</td>
  </tr>
  <tr valign="top">
    <td><code>unaims_on_release</code></td>
    <td>Boolean</td>
    <td>Whether the weapon comes down as soon as Lara stops firing, rather than staying up until she puts it away.</td>
  </tr>
</table>

### `objects`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>weapon</code></td>
    <td>String</td>
    <td>What Lara picks the weapon up as.</td>
  </tr>
  <tr valign="top">
    <td><code>ammo</code></td>
    <td>String</td>
    <td>What its ammunition arrives as.</td>
  </tr>
  <tr valign="top">
    <td><code>anim</code></td>
    <td>String</td>
    <td>The object holding the animations she carries it with.</td>
  </tr>
  <tr valign="top">
    <td><code>shell</code></td>
    <td>String</td>
    <td>What it throws out as it fires, which is an ordinary shell where the weapon names nothing.</td>
  </tr>
  <tr valign="top">
    <td><code>projectile</code></td>
    <td>String</td>
    <td>What it sends on its way, for a weapon that throws something rather than sending a round straight at what Lara aims at.</td>
  </tr>
</table>

### `ammo`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>initial_shots</code></td>
    <td>Integer</td>
    <td>The amount of ammo given when the weapon itself is collected.</td>
  </tr>
  <tr valign="top">
    <td><code>box_shots</code></td>
    <td>Integer</td>
    <td>The amount of ammo given when the equivalent ammo object is picked up.</td>
  </tr>
  <tr valign="top">
    <td><code>box_label_qty</code></td>
    <td>Integer</td>
    <td>Multiplier used in the inventory ring for each loose ammo pickup.</td>
  </tr>
  <tr valign="top">
    <td><code>rounds_per_shot</code></td>
    <td>Integer</td>
    <td>What one pull of the trigger spends, which is one round unless the weapon fires more at once, as the shotgun does.</td>
  </tr>
  <tr valign="top">
    <td><code>infinite</code></td>
    <td>Boolean</td>
    <td>Whether firing spends nothing, so that the weapon never runs out and shows no count in the inventory ring or the overlay. The pistols and the skidoo's guns have it; taking it away from the pistols makes pistol clips worth collecting. The flare answers to it as well, and a bonus game overrides it for everything.</td>
  </tr>
  <tr valign="top">
    <td><code>icon</code></td>
    <td>String</td>
    <td>The icon the ammunition counter shows beside the number of shots left.</td>
  </tr>
</table>

### `aim`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>speed</code></td>
    <td>Integer</td>
    <td>Determines how quickly Lara's arms rotate into position when aiming at a target.</td>
  </tr>
  <tr valign="top">
    <td><code>accuracy</code></td>
    <td>Integer</td>
    <td>Adds a random factor to angles used when firing a weapon. Higher values mean less accuracy.</td>
  </tr>
  <tr valign="top">
    <td><code>target_dist</code></td>
    <td>Float</td>
    <td>The maximum distance (in world sectors) that a target can be from Lara in order for her to lock on.</td>
  </tr>
  <tr valign="top">
    <td><code>lock</code></td>
    <td>Integer array (length 4)</td>
    <td>These values are used to test if Lara is able to lock on to a target.</td>
  </tr>
  <tr valign="top">
    <td><code>left</code></td>
    <td>Integer array (length 4)</td>
    <td>These values determine if Lara has lost target on her left arm.</td>
  </tr>
  <tr valign="top">
    <td><code>right</code></td>
    <td>Integer array (length 4)</td>
    <td>These values determine if Lara has lost target on her right arm.</td>
  </tr>
</table>

### `anim`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>equip</code></td>
    <td>Integer</td>
    <td>For rifle type weapons, the relative equip animation index of the associated object e.g. <code>O_LARA_SHOTGUN</code>.</td>
  </tr>
  <tr valign="top">
    <td><code>draw_frame</code></td>
    <td>Integer</td>
    <td>For rifle type weapons, the relative frame number of the equip animation where the object mesh swap is performed e.g. removing the shotgun from Lara's back and putting it in her hand.</td>
  </tr>
  <tr valign="top">
    <td><code>undraw_frame</code></td>
    <td>Integer</td>
    <td>For rifle type weapons, the relative frame number of the unequip animation where the object mesh swap is performed e.g. removing the shotgun from Lara's hand and putting it on her back.</td>
  </tr>
  <tr valign="top">
    <td><code>recoil_frame</code></td>
    <td>Integer</td>
    <td>For pistol type weapons, this value determines when Lara should snap back to the aiming frame after the weapon is fired i.e. Uzis have a lower value than Pistols for faster fire rate.</td>
  </tr>
  <tr valign="top">
    <td><code>shell_frame</code></td>
    <td>Integer</td>
    <td>The frame a spent shell leaves the weapon at. A weapon that names no frame drops its shells as it fires instead.</td>
  </tr>
  <tr valign="top">
    <td><code>ready</code></td>
    <td>String</td>
    <td>The animation the weapon rests in once it is out: <code>grenade</code> or <code>harpoon</code>. A weapon that names none rests in the aim.</td>
  </tr>
</table>

### `flash`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>time</code></td>
    <td>Integer</td>
    <td>Determines the number of frames to show the weapon flash object (<code>O_GUN_FLASH</code> / <code>O_M16_FLASH</code>) after firing a weapon.</td>
  </tr>
  <tr valign="top">
    <td><code>shade</code></td>
    <td>Integer</td>
    <td>Specifies the shade applied when drawing the weapon flash object (<code>O_GUN_FLASH</code> / <code>O_M16_FLASH</code> / <code>O_FLARE_FIRE</code>).</td>
  </tr>
  <tr valign="top">
    <td><code>color</code></td>
    <td>Float array (length 3)</td>
    <td>Specifies the color applied when drawing the weapon flash object (<code>O_GUN_FLASH</code> / <code>O_M16_FLASH</code> / <code>O_FLARE_FIRE</code>), used in TR3 lighting system.</td>
  </tr>
  <tr valign="top">
    <td><code>pos</code></td>
    <td>XYZ</td>
    <td>Specifies the offset position where the weapon flash object (<code>O_GUN_FLASH</code> / <code>O_M16_FLASH</code> / <code>O_FLARE_FIRE</code>) will be drawn. <code>flash_pos_alt</code> is used only for discarded flares.</td>
  </tr>
  <tr valign="top">
    <td><code>pos_alt</code></td>
    <td>XYZ</td>
    <td>Specifies the offset position where the weapon flash object (<code>O_GUN_FLASH</code> / <code>O_M16_FLASH</code> / <code>O_FLARE_FIRE</code>) will be drawn. <code>flash_pos_alt</code> is used only for discarded flares.</td>
  </tr>
  <tr valign="top">
    <td><code>routine</code></td>
    <td>String</td>
    <td>The muzzle flash one shot shows: <code>m16</code>, <code>mp5</code> or <code>flare</code>. A weapon that names none shows the ordinary flash, upright at the barrel.</td>
  </tr>
  <tr valign="top">
    <td><code>lights_room</code></td>
    <td>Boolean</td>
    <td>Whether the flash lights the room around Lara.</td>
  </tr>
  <tr valign="top">
    <td><code>is_optional</code></td>
    <td>Boolean</td>
    <td>Whether the player may turn the flash off, which the shotgun flash setting governs.</td>
  </tr>
</table>

### `glow`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>color</code></td>
    <td>Float array (length 3)</td>
    <td>Specifies the color applied when drawing the weapon glow object (<code>O_GLOW</code>), used in TR3 lighting system.</td>
  </tr>
  <tr valign="top">
    <td><code>pos</code></td>
    <td>XYZ</td>
    <td>Specifies the additional offset to apply to the glow sprite position.</td>
  </tr>
  <tr valign="top">
    <td><code>scale</code></td>
    <td>Float</td>
    <td>Multiplies the glow sprite's own size. <code>0</code> turns the glow off.</td>
  </tr>
  <tr valign="top">
    <td><code>flicker</code></td>
    <td>Boolean</td>
    <td>Whether the brightness is randomized every frame, the way a flare burns.</td>
  </tr>
</table>

### `muzzle`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>pos</code></td>
    <td>XYZ</td>
    <td>Specifies the additional offset to apply to the muzzle for smoke effects (right hand).</td>
  </tr>
  <tr valign="top">
    <td><code>pos_alt</code></td>
    <td>XYZ</td>
    <td>Specifies the additional offset to apply to the muzzle for smoke effects (left hand for dual pistols).</td>
  </tr>
</table>

### `smoke`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>pos</code></td>
    <td>XYZ</td>
    <td>Where smoke leaves the weapon, which is the muzzle where the weapon gives no other place.</td>
  </tr>
  <tr valign="top">
    <td><code>pos_alt</code></td>
    <td>XYZ</td>
    <td>The same, for the left hand of a weapon held in both.</td>
  </tr>
  <tr valign="top">
    <td><code>tip</code></td>
    <td>XYZ</td>
    <td>The far end of the barrel. A weapon that names one drives its smoke along the barrel and throws sparks with it; one that does not lets the smoke drift.</td>
  </tr>
  <tr valign="top">
    <td><code>tip_alt</code></td>
    <td>XYZ</td>
    <td>The same, for the left hand.</td>
  </tr>
  <tr valign="top">
    <td><code>count</code></td>
    <td>Integer</td>
    <td>How many smoke effect instances to spawn upon shooting.</td>
  </tr>
  <tr valign="top">
    <td><code>size</code></td>
    <td>String</td>
    <td>How big the smoke one shot leaves is: <code>launcher</code>. A weapon that names none smokes as an ordinary round does.</td>
  </tr>
</table>

### `shell`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>pos</code></td>
    <td>XYZ</td>
    <td>Specifies the additional offset to apply to the gun for shells (right hand).</td>
  </tr>
  <tr valign="top">
    <td><code>pos_alt</code></td>
    <td>XYZ</td>
    <td>Specifies the additional offset to apply to the gun for shells (left hand for dual pistols).</td>
  </tr>
  <tr valign="top">
    <td><code>throws_forward</code></td>
    <td>Boolean</td>
    <td>Whether spent shells leave ahead of Lara rather than fall from the hand that fired.</td>
  </tr>
  <tr valign="top">
    <td><code>angle</code></td>
    <td>Integer</td>
    <td>The angle they leave at.</td>
  </tr>
  <tr valign="top">
    <td><code>min_speed</code></td>
    <td>Integer</td>
    <td>A shell slower than this carries the amount again, so that a weapon that throws them far does not drop one at Lara's feet.</td>
  </tr>
</table>

### `sound`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>fire</code></td>
    <td>String</td>
    <td>The sound effect to play when the weapon is fired (see ./SAMPLES.md).</td>
  </tr>
  <tr valign="top">
    <td><code>overlay</code></td>
    <td>String</td>
    <td>A second sample played over the first (see ./SAMPLES.md).</td>
  </tr>
  <tr valign="top">
    <td><code>overlay_pitch</code></td>
    <td>Integer</td>
    <td>The pitch that second sample plays at.</td>
  </tr>
  <tr valign="top">
    <td><code>rapid_fire</code></td>
    <td>String</td>
    <td>The sound a held trigger makes: <code>m16</code> or <code>mp5</code>. A weapon that names none falls silent between the shots its own sample marks.</td>
  </tr>
  <tr valign="top">
    <td><code>alternating</code></td>
    <td>Boolean</td>
    <td>Whether the weapon sounds every shot as it fires, alternating between its sample and the one beside it.</td>
  </tr>
</table>

### `stow`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>place</code></td>
    <td>String</td>
    <td>Where the weapon rides when it is put away: <code>none</code>, <code>holster</code> or <code>back</code>.</td>
  </tr>
  <tr valign="top">
    <td><code>order</code></td>
    <td>Integer</td>
    <td>Which weapon shows there when Lara carries several: the lowest order wins.</td>
  </tr>
</table>

### `save`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>ammo_key</code></td>
    <td>String</td>
    <td>The name a savegame gives the rounds Lara carries now.</td>
  </tr>
  <tr valign="top">
    <td><code>resume_has_key</code></td>
    <td>String</td>
    <td>The name a level keeps the weapon under for her return.</td>
  </tr>
  <tr valign="top">
    <td><code>resume_ammo_key</code></td>
    <td>String</td>
    <td>The name it keeps her rounds under.</td>
  </tr>
  <tr valign="top">
    <td><code>required</code></td>
    <td>Boolean</td>
    <td>Whether a savegame that lacks those names is broken, which is true of the weapons the first savegame format already held.</td>
  </tr>
</table>

### `cheat`

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Description</th>
  </tr>
  <tr valign="top">
    <td><code>ammo</code></td>
    <td>Integer</td>
    <td>What the item cheat hands out. A weapon that names none is left out of it.</td>
  </tr>
  <tr valign="top">
    <td><code>key_ammo</code></td>
    <td>Integer</td>
    <td>What the key cheat hands out, which is generous by a different amount.</td>
  </tr>
</table>
