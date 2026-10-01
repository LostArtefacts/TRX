---
title: Inventory ring
order: 5
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/inventory_ring.lua. Edit it there.
-->

## <a id="inventory_ring" name="inventory_ring"></a>Inventory ring module

The rings the player browses, and the entries drawn on them.

This is the front of the inventory: which entries a ring holds, and how each is
drawn and turned. What Lara is carrying belongs to [`trx.inventory`](INVENTORY.md#inventory).

### Functions

- <a id="inventory_ring.icon_of" name="inventory_ring.icon_of"></a>[lua]`trx.inventory_ring.icon_of(object)`  
  Returns the inventory icon for a pickup, whether or not Lara has one.

  Parameters:
  - <a id="inventory_ring.icon_of.object" name="inventory_ring.icon_of.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup to check.

  Returns: [trx.catalog.objects](CATALOG.md#catalog.objects) or `nil`. The icon's object id, or `nil` for a pickup that has none.

- <a id="inventory_ring.item" name="inventory_ring.item"></a>[lua]`trx.inventory_ring.item(object)`  
  Returns the ring entry for an object.

  Returns `nil` when no entry names the object.

  Parameters:
  - <a id="inventory_ring.item.object" name="inventory_ring.item.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The inventory icon to read.

  Returns: table. The entry's `object_id`, frame counts, rotations, offsets,
    `scale`, and `draws_at_pivot`, or `nil`.

- <a id="inventory_ring.declare_item" name="inventory_ring.declare_item"></a>[lua]`trx.inventory_ring.declare_item(spec)`  
  Adds an object to the inventory ring and sets its display properties.

  Declaring an existing object replaces its entry. Use the pickup object for an
  entry that represents itself.

  Parameters:
  - <a id="inventory_ring.declare_item.spec" name="inventory_ring.declare_item.spec"></a>**`spec`** (table). The entry's `object_id`, frame counts, rotations, offsets,
    `scale`, and `draws_at_pivot`. An omitted value keeps the ring's default.

  Example:
  ```lua
  trx.inventory_ring.declare_item({
    object_id = "mymod:lantern_item",
    frames_total = 1,
    anim_direction = 1,
    anim_speed = 1,
    scale = 1.0,
    meshes_sel = -1,
    meshes_drawn = -1,
    inv_pos = 20,
  })
  ```
