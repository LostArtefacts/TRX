---
title: Object
order: 14
---

## Object module

Module for controlling game objects.

### Structures

- [lua]`trx.objects.Object`

    Represents a moveable object type.

    Properties:
    - **`properties`**: Table of typed object properties. Values written here
      become defaults for future item initialization and spawns; existing items
      are not changed retroactively.

    Properties are object-specific; see [Objects](../../OBJECTS.md) for the
    documented properties on each object. Property tables can be iterated with
    `pairs()`.

### Functions

- [lua]`trx.objects[object_id]`
- [lua]`trx.objects.object_name`

  Retrieves an object proxy for the given object ID or catalog object name.

  Example:
  ```lua
  trx.objects[trx.catalog.objects.wolf].properties.max_hit_points = 30
  trx.objects.flare_item.properties.burn_time = 1
  for name, value in pairs(trx.objects[trx.catalog.objects.wolf].properties) do
    trx.log.info(name .. " = " .. tostring(value))
  end
  ```

- [lua]`trx.objects.swap_mesh(obj1_id, obj2_id, mesh1_num, mesh2_num)`  
    Swaps the given meshes of the given objects.  
    Examples:
    - [lua]`trx.objects.swap_mesh(trx.catalog.objects.pierre, trx.catalog.objects.larson, 8, 8)`  
      Pierre now has Larson's head, and vice-versa

- [lua]`trx.objects.swap_mesh(obj1_id, obj2_id)`  
    Similar to above, but this will swap out all meshes rather than specific ones. This works best when both objects have the same mesh count; if one object has fewer meshes than the other, the minimum count will be used.  
    Examples:
    - [lua]`trx.objects.swap_mesh(trx.catalog.objects.pierre, trx.catalog.objects.larson)`  
      Pierre and Larson's meshes are fully swapped
    - [lua]`trx.objects.swap_mesh(trx.catalog.objects.pierre, trx.catalog.objects.warrior_1)`  
      Pierre's 15 meshes are now of mutant type; the mutant's first 15 meshes are Pierre's, the rest are default.
