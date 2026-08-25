---
title: Rules
order: 14
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/rules.lua. Edit it there.
-->

## <a id="rules" name="rules"></a>Rules module

Module for the numbers the engine plays by.

These are the game's rules: a mechanic that no single item owns. An
object's own numbers live on the object, as
`trx.objects.<name>.properties`, and the player's own choices live in
[`trx.config`](CONFIG.md#config).

A rule lasts as long as the playthrough: it is saved with the game and
restored with it, and a new game starts from the defaults. A level script
states what its level wants, and states it again on every entry, so a level
that wants the defaults back asks for them.

### Properties

- <a id="rules.exposure.max" name="rules.exposure.max"></a>**`trx.rules.exposure.max`** ([trx.game.Frames](GAME.md#game.Frames)). How much warmth Lara holds, and what [`trx.lara.exposure_bar`](LARA.md#lara.Lara.exposure_bar) fills to. Warmth only moves in a room carrying the [`trx.rooms.Room.damaging`](ROOMS.md#rooms.Room.damaging) flag, such as the cold water of Antarctica.
- <a id="rules.exposure.drain_land" name="rules.exposure.drain_land"></a>**`trx.rules.exposure.drain_land`** (integer). Warmth lost each frame in the cold, on land or wading.
- <a id="rules.exposure.drain_water" name="rules.exposure.drain_water"></a>**`trx.rules.exposure.drain_water`** (integer). Warmth lost each frame in the cold, underwater or at the surface.
- <a id="rules.exposure.recovery" name="rules.exposure.recovery"></a>**`trx.rules.exposure.recovery`** (integer). Warmth regained each frame once out of the cold.
- <a id="rules.exposure.damage" name="rules.exposure.damage"></a>**`trx.rules.exposure.damage`** (integer). Hit points lost each frame once the warmth has run out.
- <a id="rules.corpse.fade_speed" name="rules.corpse.fade_speed"></a>**`trx.rules.corpse.fade_speed`** (integer). How much of a body's coverage goes each frame, out of 255. It is taken away once nothing is left. `0` leaves it where it lies.
- <a id="rules.carrier.snap_to_sector" name="rules.carrier.snap_to_sector"></a>**`trx.rules.carrier.snap_to_sector`** (boolean). Whether an item a defeated enemy carried lands in the middle of the sector
  the enemy stood on, rather than at its feet. Quest items are left where
  they fall either way.
- <a id="rules.carrier.inherit_facing" name="rules.carrier.inherit_facing"></a>**`trx.rules.carrier.inherit_facing`** (boolean). Whether an item a defeated enemy carried turns to face the way the enemy
  did, rather than keeping the rotation the level gave it. This only reaches
  drops the level data places on the enemy; a drop the gameflow names always
  takes the enemy's facing.
- <a id="rules.fx.rotate_debris" name="rules.fx.rotate_debris"></a>**`trx.rules.fx.rotate_debris`** (boolean). Whether debris pieces generated from shattered meshes should rotate in yaw
  and pitch while they are active. The original TR4 did not apply rotation.

### Functions

- <a id="rules.list" name="rules.list"></a>[lua]`trx.rules.list()`  
  Every rule there is, as dotted `group.field` keys, in no particular order.

  Returns: a list of string.

- <a id="rules.get" name="rules.get"></a>[lua]`trx.rules.get(key)`  
  Reads a rule by its key, for code that does not know which one it wants.

  Parameters:
  - <a id="rules.get.key" name="rules.get.key"></a>**`key`** (string). Dotted path, e.g. `exposure.damage`.

  Returns: any. Raises if no rule has that key.

- <a id="rules.set" name="rules.set"></a>[lua]`trx.rules.set(key, value)`  
  Changes a rule by its key. A string is read as text, the way the console gives it; any other value is taken as the rule's own type.

  Parameters:
  - <a id="rules.set.key" name="rules.set.key"></a>**`key`** (string). Dotted path, e.g. `exposure.damage`.
  - <a id="rules.set.value" name="rules.set.value"></a>**`value`** (any). The value to write, of the type the rule declares.

- <a id="rules.reset" name="rules.reset"></a>[lua]`trx.rules.reset([key])`  
  Puts a rule back to the value the engine ships with, or every rule when given no key. Happens on its own when a new game starts.

  Parameters:
  - <a id="rules.reset.key" name="rules.reset.key"></a>**`key`** (string, optional). Dotted path.

- <a id="rules.format_value" name="rules.format_value"></a>[lua]`trx.rules.format_value(key)`  
  How a rule's value reads as text, for showing it to the player.

  Parameters:
  - <a id="rules.format_value.key" name="rules.format_value.key"></a>**`key`** (string). Dotted path.

  Returns: string. The text, ready to print.
