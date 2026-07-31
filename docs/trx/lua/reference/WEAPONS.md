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

## Weapons module

What a weapon is, rather than what Lara has of it.

None of this differs between the inventory she carries and the one a level
keeps for her, so it belongs to neither: what she holds and how many shots she
has are `trx.inventory`.

### Functions

- [lua]`trx.weapons.is_available(weapon)`  
  Whether the game allows this weapon at all. The game flow can keep one out, and
  a cheat that hands it over anyway leaves Lara with a gun the level was built
  without.

  Parameters:
  - **`weapon`** (integer). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is. Compare against `trx.catalog.weapons`.

  Returns: boolean.

- [lua]`trx.weapons.object(weapon)`  
  The pickup the weapon is, for handing it to `trx.inventory:give`.

  Parameters:
  - **`weapon`** (integer). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is. Compare against `trx.catalog.weapons`.

  Returns: integer. The object id, or `nil` if this game has no such weapon. Compare against `trx.catalog.objects`.

  Example:
  ```lua
  trx.inventory:give(trx.weapons.object(trx.catalog.weapons.SHOTGUN))
  ```

- [lua]`trx.weapons.shots_per_box(weapon)`  
  How many shots one box of ammunition for it is worth.

  Parameters:
  - **`weapon`** (integer). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is. Compare against `trx.catalog.weapons`.

  Returns: integer.
