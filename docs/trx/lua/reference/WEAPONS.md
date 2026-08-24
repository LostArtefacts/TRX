---
title: Weapon
order: 5
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/weapons.lua. Edit it there.
-->

## <a id="weapons" name="weapons"></a>Weapon module

What a weapon is, rather than what Lara has of it.

None of this differs between the inventory she carries and the one a level
keeps for her, so it belongs to neither: what she holds and how many shots she
has are [`trx.inventory`](INVENTORY.md#inventory).

A weapon is shared by every copy of it. Changes last for the rest of the
session, so levels should restore any values they change when they end.

### Indexing

Indexing the module reaches a weapon definition, so [`trx.weapons.uzis`](#weapons) is the uzis. Keyed by weapon id or catalog name, not by position.

- <a id="weapons[]" name="weapons[]"></a>**`trx.weapons[key]`** (key: [trx.catalog.weapons](CATALOG.md#catalog.weapons) or string, value: [trx.weapons.Weapon](#weapons.Weapon) or `nil`). Weapon id, or its catalog name.

Example:
```lua
trx.weapons.uzis.damage = 5
trx.weapons.shotgun.ammo.box_shots = 12
trx.weapons.flare.glow.color = "33e5ff"
```

### Properties

- <a id="weapons.all" name="weapons.all"></a>**`trx.weapons.all`** (a list of [trx.weapons.Weapon](#weapons.Weapon)). Every weapon the engine knows, in the order it holds them. `UNARMED` is not one of them. *(read-only)*

### Enums

- <a id="weapons.Kind" name="weapons.Kind"></a>[lua]`trx.weapons.Kind`

    How the engine holds and fires a weapon, which decides which arm animations and firing routine it uses.

    - `trx.weapons.Kind.DUAL_PISTOLS` = `0`  
        One in each hand, each arm aiming and firing on its own.
    - `trx.weapons.Kind.SINGLE_PISTOL` = `1`  
        One in the right hand.
    - `trx.weapons.Kind.RIFLE` = `2`  
        Held in both hands, drawn from Lara's back.
    - `trx.weapons.Kind.MOUNTED` = `3`  
        Fixed to a vehicle rather than held.
    - `trx.weapons.Kind.FLARE` = `4`  
        Held in one hand and burning, rather than fired.

### Structures

- <a id="weapons.AimLimits" name="weapons.AimLimits"></a>[lua]`trx.weapons.AimLimits`

    How far off straight ahead an aim may go, as a pair of limits about each axis.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="weapons.AimLimits.max_pitch" name="weapons.AimLimits.max_pitch"></a>**`max_pitch`**: [trx.math.Angle](MATH.md#math.Angle). As far down as it reaches.
    - <a id="weapons.AimLimits.max_yaw" name="weapons.AimLimits.max_yaw"></a>**`max_yaw`**: [trx.math.Angle](MATH.md#math.Angle). As far to the right as it reaches.
    - <a id="weapons.AimLimits.min_pitch" name="weapons.AimLimits.min_pitch"></a>**`min_pitch`**: [trx.math.Angle](MATH.md#math.Angle). As far up as it reaches, which is a negative angle.
    - <a id="weapons.AimLimits.min_yaw" name="weapons.AimLimits.min_yaw"></a>**`min_yaw`**: [trx.math.Angle](MATH.md#math.Angle). As far to the left as the aim reaches, which is a negative angle.

- <a id="weapons.HandPos" name="weapons.HandPos"></a>[lua]`trx.weapons.HandPos`

    An offset in the frame of the hand that holds the weapon. A weapon held in one hand only uses the right.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="weapons.HandPos.left" name="weapons.HandPos.left"></a>**`left`**: [trx.math.Vec3](MATH.md#math.Vec3). Offset in the left hand.
    - <a id="weapons.HandPos.right" name="weapons.HandPos.right"></a>**`right`**: [trx.math.Vec3](MATH.md#math.Vec3). Offset in the right hand.

- <a id="weapons.Ammo" name="weapons.Ammo"></a>[lua]`trx.weapons.Ammo`

    What the weapon is fed. A shot is one pull of the trigger, which for the shotgun
    spends six rounds; the flare counts a flare where a weapon counts a shot.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="weapons.Ammo.box_label_qty" name="weapons.Ammo.box_label_qty"></a>**`box_label_qty`**: integer. What a box shows on its inventory icon, which follows nothing else.
    - <a id="weapons.Ammo.box_shots" name="weapons.Ammo.box_shots"></a>**`box_shots`**: integer. What one box of ammunition is worth.
    - <a id="weapons.Ammo.infinite" name="weapons.Ammo.infinite"></a>**`infinite`**: boolean. Whether firing spends nothing, so the weapon never runs out and carries no counter.
    - <a id="weapons.Ammo.initial_shots" name="weapons.Ammo.initial_shots"></a>**`initial_shots`**: integer. What the weapon arrives with the first time Lara picks it up.

- <a id="weapons.Flash" name="weapons.Flash"></a>[lua]`trx.weapons.Flash`

    The muzzle flash a shot draws.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="weapons.Flash.color" name="weapons.Flash.color"></a>**`color`**: [trx.math.Color](MATH.md#math.Color). What color it lights with, in TR3 and later.
    - <a id="weapons.Flash.shade" name="weapons.Flash.shade"></a>**`shade`**: integer. How brightly it lights the model around it, in TR1 and TR2. Later games light it by [`color`](#weapons.Flash.color) instead.
    - <a id="weapons.Flash.time" name="weapons.Flash.time"></a>**`time`**: integer. How many frames the flash stays on screen for.

    Computed properties (derived, not stored on the object):
    - <a id="weapons.Flash.pos" name="weapons.Flash.pos"></a>**`pos`**: [trx.weapons.HandPos](#weapons.HandPos). Where the flash is drawn, in each hand.

- <a id="weapons.Glow" name="weapons.Glow"></a>[lua]`trx.weapons.Glow`

    The glow sprite drawn where the weapon burns: a gun's muzzle, or a lit flare.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="weapons.Glow.color" name="weapons.Glow.color"></a>**`color`**: [trx.math.Color](MATH.md#math.Color). What color the glow is drawn in.
    - <a id="weapons.Glow.flicker" name="weapons.Glow.flicker"></a>**`flicker`**: boolean. Whether the brightness is randomized every frame, the way a flare burns.
    - <a id="weapons.Glow.pos" name="weapons.Glow.pos"></a>**`pos`**: [trx.math.Vec3](MATH.md#math.Vec3). Where it sits, in the frame of the mesh it follows.
    - <a id="weapons.Glow.scale" name="weapons.Glow.scale"></a>**`scale`**: number. Multiplies the sprite's own size. `0` turns the glow off.

- <a id="weapons.Anim" name="weapons.Anim"></a>[lua]`trx.weapons.Anim`

    The animation numbers a rifle is drawn, put away and fired by. They count the animations and frames of the weapon's own object, not Lara's.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="weapons.Anim.draw_frame" name="weapons.Anim.draw_frame"></a>**`draw_frame`**: integer. The frame the weapon appears in her hands on.
    - <a id="weapons.Anim.equip_anim" name="weapons.Anim.equip_anim"></a>**`equip_anim`**: integer. The animation the weapon starts on as Lara reaches for it. One the object does not have raises.
    - <a id="weapons.Anim.recoil_frame" name="weapons.Anim.recoil_frame"></a>**`recoil_frame`**: integer. The frame a pistol kicks on.
    - <a id="weapons.Anim.undraw_frame" name="weapons.Anim.undraw_frame"></a>**`undraw_frame`**: integer. The frame it leaves her hands on.

- <a id="weapons.Weapon" name="weapons.Weapon"></a>[lua]`trx.weapons.Weapon`

    A weapon definition, reached as [`trx.weapons.uzis`](#weapons) or by id.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="weapons.Weapon.aim_speed" name="weapons.Weapon.aim_speed"></a>**`aim_speed`**: [trx.math.Angle](MATH.md#math.Angle). How far the arms swing towards the target each frame.
    - <a id="weapons.Weapon.damage" name="weapons.Weapon.damage"></a>**`damage`**: integer. Hit points one shot takes off what it hits.
    - <a id="weapons.Weapon.fire_overlay_pitch" name="weapons.Weapon.fire_overlay_pitch"></a>**`fire_overlay_pitch`**: integer. The pitch at which to play the overlay sample.
    - <a id="weapons.Weapon.fire_overlay_sample" name="weapons.Weapon.fire_overlay_sample"></a>**`fire_overlay_sample`**: [trx.catalog.samples](CATALOG.md#catalog.samples). The overlay sample a shot plays. One this game has no sound for is silent.
    - <a id="weapons.Weapon.fire_sample" name="weapons.Weapon.fire_sample"></a>**`fire_sample`**: [trx.catalog.samples](CATALOG.md#catalog.samples). The sample a shot plays. One this game has no sound for is silent.
    - <a id="weapons.Weapon.gun_height" name="weapons.Weapon.gun_height"></a>**`gun_height`**: [trx.math.Distance](MATH.md#math.Distance). How far above Lara's feet the shot leaves the barrel. It also decides how deep she can wade and still fire.
    - <a id="weapons.Weapon.id" name="weapons.Weapon.id"></a>**`id`**: [trx.catalog.weapons](CATALOG.md#catalog.weapons). Which weapon this is, for the calls that take one: `trx.inventory:set_shots(weapon.id, 100)`. *(read-only)*
    - <a id="weapons.Weapon.is_available" name="weapons.Weapon.is_available"></a>**`is_available`**: boolean. Whether the game allows the weapon at all. Turning one off keeps it out of the cheats and off the controls list, and a save that carries it arrives without it.
    - <a id="weapons.Weapon.kind" name="weapons.Weapon.kind"></a>**`kind`**: [trx.weapons.Kind](#weapons.Kind). How the engine holds and fires it.
    - <a id="weapons.Weapon.shot_accuracy" name="weapons.Weapon.shot_accuracy"></a>**`shot_accuracy`**: [trx.math.Angle](MATH.md#math.Angle). How wide a cone a shot may stray into. `0` never misses.
    - <a id="weapons.Weapon.smoke_count" name="weapons.Weapon.smoke_count"></a>**`smoke_count`**: integer. How many puffs of smoke a shot leaves at the muzzle, in TR3. `0` for none.
    - <a id="weapons.Weapon.target_dist" name="weapons.Weapon.target_dist"></a>**`target_dist`**: [trx.math.Distance](MATH.md#math.Distance). How far the weapon reaches, both for auto-aim and for the shot itself.

    Computed properties (derived, not stored on the object):
    - <a id="weapons.Weapon.ammo" name="weapons.Weapon.ammo"></a>**`ammo`**: [trx.weapons.Ammo](#weapons.Ammo). What the weapon is fed.
    - <a id="weapons.Weapon.ammo_icon" name="weapons.Weapon.ammo_icon"></a>**`ammo_icon`**: string. The markup drawn beside the ammunition count in TR1. Later games count without one, and a weapon that carries no icon answers with `nil`.
    - <a id="weapons.Weapon.ammo_object" name="weapons.Weapon.ammo_object"></a>**`ammo_object`**: [trx.catalog.objects](CATALOG.md#catalog.objects). The box of ammunition it takes, or `nil` where it takes none.
    - <a id="weapons.Weapon.anim" name="weapons.Weapon.anim"></a>**`anim`**: [trx.weapons.Anim](#weapons.Anim). The animation numbers it is drawn and fired by.
    - <a id="weapons.Weapon.flash" name="weapons.Weapon.flash"></a>**`flash`**: [trx.weapons.Flash](#weapons.Flash). The muzzle flash a shot draws.
    - <a id="weapons.Weapon.glow" name="weapons.Weapon.glow"></a>**`glow`**: [trx.weapons.Glow](#weapons.Glow). The glow drawn where it burns.
    - <a id="weapons.Weapon.has_infinite_ammo" name="weapons.Weapon.has_infinite_ammo"></a>**`has_infinite_ammo`**: boolean. Whether the weapon never runs dry. The pistols do in most games, and a level or a script may say so of any weapon. A count of shots left means nothing in this context.
    - <a id="weapons.Weapon.left_arm" name="weapons.Weapon.left_arm"></a>**`left_arm`**: [trx.weapons.AimLimits](#weapons.AimLimits). How far the left arm may follow a target it has locked onto. A dual-wielded weapon drops the lock on the arm that cannot reach.
    - <a id="weapons.Weapon.lock" name="weapons.Weapon.lock"></a>**`lock`**: [trx.weapons.AimLimits](#weapons.AimLimits). Where auto-aim may lock on, measured from where Lara faces.
    - <a id="weapons.Weapon.muzzle_pos" name="weapons.Weapon.muzzle_pos"></a>**`muzzle_pos`**: [trx.weapons.HandPos](#weapons.HandPos). Where the barrel ends, which is where smoke and sparks come from.
    - <a id="weapons.Weapon.object" name="weapons.Weapon.object"></a>**`object`**: [trx.catalog.objects](CATALOG.md#catalog.objects). The pickup the weapon is, for handing it to [`trx.inventory:give`](INVENTORY.md#inventory.Inventory.give). `nil` where this game has no such weapon.
    - <a id="weapons.Weapon.right_arm" name="weapons.Weapon.right_arm"></a>**`right_arm`**: [trx.weapons.AimLimits](#weapons.AimLimits). How far the right arm may follow a target.
    - <a id="weapons.Weapon.rounds_per_shot" name="weapons.Weapon.rounds_per_shot"></a>**`rounds_per_shot`**: integer. How many rounds one pull of the trigger spends: six for the shotgun, one for everything else. What a box is worth in shots is [`trx.weapons.Ammo.box_shots`](#weapons.Ammo.box_shots).
    - <a id="weapons.Weapon.shell_pos" name="weapons.Weapon.shell_pos"></a>**`shell_pos`**: [trx.weapons.HandPos](#weapons.HandPos). Where a spent shell is thrown from. A weapon that leaves no shells has this at the origin.

### Functions

- <a id="weapons.get" name="weapons.get"></a>[lua]`trx.weapons.get(key)`  
  Retrieves a weapon definition by id or by name.

  Parameters:
  - <a id="weapons.get.key" name="weapons.get.key"></a>**`key`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons) or string). Weapon id, or its catalog name: `trx.weapons["uzis"]`.

  Returns: [trx.weapons.Weapon](#weapons.Weapon) or `nil`. `nil` if this game has no such weapon.

  Example:
  ```lua
  local uzis = trx.weapons.get(trx.catalog.weapons.UZIS)
  uzis.damage = 5
  ```

- <a id="weapons.is_available" name="weapons.is_available"></a>[lua]`trx.weapons.is_available(weapon)`  
  **Deprecated.** Read [`trx.weapons.Weapon.is_available`](#weapons.Weapon.is_available) instead.

  Whether the game allows this weapon at all. The game flow can keep one out, and
  a cheat that hands it over anyway leaves Lara with a gun the level was built
  without.

  Parameters:
  - <a id="weapons.is_available.weapon" name="weapons.is_available.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN`, `UNARMED`, and out-of-range values raise.

  Returns: boolean. True where this game has the weapon at all.

- <a id="weapons.object" name="weapons.object"></a>[lua]`trx.weapons.object(weapon)`  
  **Deprecated.** Read [`trx.weapons.Weapon.object`](#weapons.Weapon.object) instead.

  The pickup the weapon is, for handing it to [`trx.inventory:give`](INVENTORY.md#inventory.Inventory.give).

  Parameters:
  - <a id="weapons.object.weapon" name="weapons.object.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN`, `UNARMED`, and out-of-range values raise.

  Returns: [trx.catalog.objects](CATALOG.md#catalog.objects) or `nil`. The object id, or `nil` if this game has no such weapon.

  Example:
  ```lua
  trx.inventory:give(trx.weapons.object(trx.catalog.weapons.SHOTGUN))
  ```

- <a id="weapons.ammo_object" name="weapons.ammo_object"></a>[lua]`trx.weapons.ammo_object(weapon)`  
  **Deprecated.** Read [`trx.weapons.Weapon.ammo_object`](#weapons.Weapon.ammo_object) instead.

  The box of ammunition the weapon takes.

  Parameters:
  - <a id="weapons.ammo_object.weapon" name="weapons.ammo_object.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN`, `UNARMED`, and out-of-range values raise.

  Returns: [trx.catalog.objects](CATALOG.md#catalog.objects) or `nil`. The object id, or `nil` where the weapon takes no ammunition.

- <a id="weapons.rounds_per_shot" name="weapons.rounds_per_shot"></a>[lua]`trx.weapons.rounds_per_shot(weapon)`  
  **Deprecated.** Read [`trx.weapons.Weapon.rounds_per_shot`](#weapons.Weapon.rounds_per_shot) instead.

  How many rounds one pull of the trigger spends. Six for the shotgun, one for everything else.

  Parameters:
  - <a id="weapons.rounds_per_shot.weapon" name="weapons.rounds_per_shot.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN`, `UNARMED`, and out-of-range values raise.

  Returns: integer. Rounds, not shots.

- <a id="weapons.shots_per_box" name="weapons.shots_per_box"></a>[lua]`trx.weapons.shots_per_box(weapon)`  
  **Deprecated.** Read [`trx.weapons.Ammo.box_shots`](#weapons.Ammo.box_shots) instead, which is the same number.

  How many shots one box of ammunition for it is worth.

  Parameters:
  - <a id="weapons.shots_per_box.weapon" name="weapons.shots_per_box.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN`, `UNARMED`, and out-of-range values raise.

  Returns: integer. Shots, not rounds.
