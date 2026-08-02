---
title: Creatures
order: 11
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/creatures.lua. Edit it there.
-->

## Creatures module

Module for controlling certain creature behavior.

### Properties

- <a name="creatures.hostile_allies"></a>**`trx.creatures.hostile_allies`** (boolean). Whether Lara's allies are hostile towards her.

### Functions

- <a name="creatures.add_ally"></a>[lua]`trx.creatures.add_ally(object_id)`  
  Marks an object as an ally of Lara. Every item of that type becomes an ally.

  Parameters:
  - **`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)).

  Example:
  ```lua
  trx.creatures.add_ally(trx.catalog.objects.monk_1)
  ```

- <a name="creatures.add_ally_target"></a>[lua]`trx.creatures.add_ally_target(object_id)`  
  Marks an object as one that will fight any of Lara's allies. Every item of that type will target them.

  Parameters:
  - **`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)).

  Example:
  ```lua
  trx.creatures.add_ally_target(trx.catalog.objects.bandit_1)
  ```
