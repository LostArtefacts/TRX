---
title: Rules
order: 12
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/rules.lua. Edit it there.
-->

## Rules module

Module for the numbers the engine plays by.

These are the game's rules: a mechanic that no single item owns. An object's own numbers live on
the object, as `trx.objects.<name>.properties`, and the player's own choices live in `trx.config`.

A rule lasts as long as the playthrough: it is saved with the game and restored with it, and a
new game starts from the defaults. A level script states what its level wants, and states it
again on every entry, so a level that wants the defaults back asks for them.

### Properties

- **`trx.rules.exposure.max`** (integer). How much warmth Lara holds, in frames, and what `trx.lara.exposure_bar` fills to. Warmth only moves in a room carrying the `damaging` flag, such as the cold water of Antarctica.
- **`trx.rules.exposure.drain_land`** (integer). Warmth lost each frame in the cold, on land or wading.
- **`trx.rules.exposure.drain_water`** (integer). Warmth lost each frame in the cold, underwater or at the surface.
- **`trx.rules.exposure.recovery`** (integer). Warmth regained each frame once out of the cold.
- **`trx.rules.exposure.damage`** (integer). Hit points lost each frame once the warmth has run out.
- **`trx.rules.corpse.fade_speed`** (integer). How much of a body's coverage goes each frame, out of 255. It is taken away once nothing is left. `0` leaves it where it lies.

### Functions

- [lua]`trx.rules.list()`  
  Every rule there is, as dotted `group.field` keys.

  Returns: table.

- [lua]`trx.rules.get(key)`  
  Reads a rule by its key, for code that does not know which one it wants.

  Parameters:
  - **`key`** (string). Dotted path, e.g. `exposure.damage`.

  Returns: any. Raises if no rule has that key.

- [lua]`trx.rules.set(key, value)`  
  Changes a rule by its key. A string is read as text, the way the console gives it; any other value is taken as the rule's own type.

  Parameters:
  - **`key`** (string). Dotted path, e.g. `exposure.damage`.
  - **`value`** (any).

- [lua]`trx.rules.reset([key])`  
  Puts a rule back to the value the engine ships with, or every rule when given no key. Happens on its own when a new game starts.

  Parameters:
  - **`key`** (string, optional). Dotted path.

- [lua]`trx.rules.format_value(key)`  
  How a rule's value reads as text, for showing it to the player.

  Parameters:
  - **`key`** (string). Dotted path.

  Returns: string.
