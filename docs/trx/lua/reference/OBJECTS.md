---
title: Object
order: 15
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/objects.lua. Edit it there.
-->

## Object module

Module for the object definitions a level is built from.

An object is the pattern every item of that type is cut from: a wolf's radius, not this wolf's. Per-item state lives on the item - see `trx.items`.

### Indexing

Indexing the module reaches an object definition, so `trx.objects.wolf` is the wolf.

- **`trx.objects[key]`** (Object or `nil`). Object id, or its catalog name.

Example:
```lua
trx.objects.wolf.properties.max_hit_points = 30
```

### Structures

- [lua]`trx.objects.Object`

    An object definition.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`anim_count`**: integer. How many animations it has. *(read-only)*
    - **`is_intelligent`**: boolean. Whether the object thinks - a creature rather than a door. *(read-only)*
    - **`loaded`**: boolean. Whether the current level has this object at all. An object it never loaded still has a definition; this is how a script tells. *(read-only)*
    - **`mesh_count`**: integer. How many meshes the object is built from. *(read-only)*
    - **`pivot_length`**: integer. How far in front of itself the object turns about.
    - **`radius`**: integer. Collision radius.
    - **`semi_transparent`**: boolean. Whether the object is drawn see-through.
    - **`shadow_size`**: integer. Size of the blob shadow drawn under it, and 0 for none.
    - **`smartness`**: integer. How readily a creature of this type finds its way to Lara.

    Computed properties (derived, not stored on the object):
    - **`properties`**: table. The object's own typed properties, which every item of the type inherits. Writing here changes the default for all of them; write to `item.properties` to change one item only. Iterable with `pairs()`. See [Objects](../../OBJECTS.md).

    Methods:

    - [lua]`object:get_property(name)`  
      Reads one of the object's properties. Prefer `object.properties.<name>`.

      Parameters:
      - **`name`** (string).

      Returns: any or `nil`.

    - [lua]`object:get_property_names()`  
      Names of every property this object declares.

      Returns: table.

    - [lua]`object:set_property(name, value)`  
      Writes one of the object's properties. Prefer `object.properties.<name> = ...`.

      Parameters:
      - **`name`** (string).
      - **`value`** (any).

### Functions

- [lua]`trx.objects.get(key)`  
  Retrieves an object definition by id or by name.

  Parameters:
  - **`key`** (any). Object id, or its catalog name: `trx.objects["wolf"]`. Compare against `trx.catalog.objects`.

  Returns: Object or `nil`. `nil` if no such object exists.

  Example:
  ```lua
  local wolf = trx.objects.wolf
  wolf.properties.max_hit_points = 30
  ```

- [lua]`trx.objects.swap_mesh(object_id1, object_id2, [mesh_num1], [mesh_num2])`  
  Swaps meshes between two objects. With no mesh numbers, swaps all of them; with both, swaps just those two. One without the other raises.

  Parameters:
  - **`object_id1`** (integer). Compare against `trx.catalog.objects`.
  - **`object_id2`** (integer). Compare against `trx.catalog.objects`.
  - **`mesh_num1`** (integer, optional). Mesh of the first.
  - **`mesh_num2`** (integer, optional). Mesh of the second.
