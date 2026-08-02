---
title: Weapons
order: 5
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/weapons.lua. Edit it there.
-->

## <a id="weapons" name="weapons"></a>Weapons module

What a weapon is, rather than what Lara has of it.

None of this differs between the inventory she carries and the one a level
keeps for her, so it belongs to neither: what she holds and how many shots she
has are [`trx.inventory`](INVENTORY.md#inventory).

### Functions

- <a id="weapons.is_available" name="weapons.is_available"></a>[lua]`trx.weapons.is_available(weapon)`  
  Whether the game allows this weapon at all. The game flow can keep one out, and
  a cheat that hands it over anyway leaves Lara with a gun the level was built
  without.

  Parameters:
  - <a id="weapons.is_available.weapon" name="weapons.is_available.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.

  Returns: boolean. True where this game has the weapon at all.

- <a id="weapons.object" name="weapons.object"></a>[lua]`trx.weapons.object(weapon)`  
  The pickup the weapon is, for handing it to [`trx.inventory:give`](INVENTORY.md#inventory.Inventory.give).

  Parameters:
  - <a id="weapons.object.weapon" name="weapons.object.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.

  Returns: [trx.catalog.objects](CATALOG.md#catalog.objects). The object id, or `nil` if this game has no such weapon.

  Example:
  ```lua
  trx.inventory:give(trx.weapons.object(trx.catalog.weapons.SHOTGUN))
  ```

- <a id="weapons.shots_per_box" name="weapons.shots_per_box"></a>[lua]`trx.weapons.shots_per_box(weapon)`  
  How many shots one box of ammunition for it is worth.

  Parameters:
  - <a id="weapons.shots_per_box.weapon" name="weapons.shots_per_box.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.

  Returns: integer. Shots, not rounds.
