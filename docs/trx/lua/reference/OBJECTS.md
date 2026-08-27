---
title: Object
order: 7
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/objects.lua. Edit it there.
-->

## <a id="objects" name="objects"></a>Object module

Module for the object definitions a level is built from.

An object is the pattern every item of that type is cut from: a wolf's radius, not this wolf's. Per-item state lives on the item - see [`trx.items`](ITEMS.md#items).

### Indexing

Indexing the module reaches an object definition, so [`trx.objects.wolf`](#objects) is the wolf. Keyed by object id or catalog name, not by position.

- <a id="objects[]" name="objects[]"></a>**`trx.objects[key]`** (key: [trx.catalog.objects](CATALOG.md#catalog.objects) or string, value: [trx.objects.Object](#objects.Object) or `nil`). Object id, or its catalog name.

Example:
```lua
trx.objects.wolf.properties.max_hit_points = 30
```

### Properties

- <a id="objects.query" name="objects.query"></a>**`trx.objects.query`** ([trx.objects.ObjectQuery](#objects.ObjectQuery)). The identity query over every object definition. Narrow it and read it. *(read-only)*

### Structures

- <a id="objects.MeshNum" name="objects.MeshNum"></a>[lua]`trx.objects.MeshNum`

    The mesh's number within the object it belongs to. Counted from 0.

- <a id="objects.Object" name="objects.Object"></a>[lua]`trx.objects.Object`

    An object definition.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="objects.Object.anim_count" name="objects.Object.anim_count"></a>**`anim_count`**: integer. How many animations it has. *(read-only)*
    - <a id="objects.Object.is_intelligent" name="objects.Object.is_intelligent"></a>**`is_intelligent`**: boolean. Whether the object thinks - a creature rather than a door. *(read-only)*
    - <a id="objects.Object.loaded" name="objects.Object.loaded"></a>**`loaded`**: boolean. Whether the current level has this object at all. An object it never loaded still has a definition; this is how a script tells. *(read-only)*
    - <a id="objects.Object.mesh_count" name="objects.Object.mesh_count"></a>**`mesh_count`**: integer. How many meshes the object is built from. *(read-only)*
    - <a id="objects.Object.pivot_length" name="objects.Object.pivot_length"></a>**`pivot_length`**: integer. How far in front of itself the object turns about.
    - <a id="objects.Object.radius" name="objects.Object.radius"></a>**`radius`**: integer. Collision radius.
    - <a id="objects.Object.semi_transparent" name="objects.Object.semi_transparent"></a>**`semi_transparent`**: boolean. Whether the object is drawn see-through.
    - <a id="objects.Object.shadow_size" name="objects.Object.shadow_size"></a>**`shadow_size`**: integer. Size of the blob shadow drawn under it, and 0 for none.
    - <a id="objects.Object.smartness" name="objects.Object.smartness"></a>**`smartness`**: integer. How readily a creature of this type finds its way to Lara.

    Computed properties (derived, not stored on the object):
    - <a id="objects.Object.default_names" name="objects.Object.default_names"></a>**`default_names`**: table. The compile-time English names. A lookup tries these when the player's language has no matching name, so an English name still reaches the object in a translated install.
    - <a id="objects.Object.name" name="objects.Object.name"></a>**`name`**: string. The name the game shows for the object. It is the first value in [`names`](#objects.Object.names), or `nil` where the object has no name.
    - <a id="objects.Object.names" name="objects.Object.names"></a>**`names`**: table. Every name the object answers to, in the player's language. An object has more than one: a large medipack is also a `medipack` and a `big medi`.
    - <a id="objects.Object.properties" name="objects.Object.properties"></a>**`properties`**: table. The object's own typed properties, which every item of the type inherits. Writing here changes the default for all of them; write to [`trx.items.Item.properties`](ITEMS.md#items.Item.properties) to change one item only. Iterable with `pairs()`. See [Objects](../../OBJECTS.md).

    Methods:

    - <a id="objects.Object.get_default_names" name="objects.Object.get_default_names"></a>[lua]`object:get_default_names()`  
      The compile-time English names. A lookup tries these when the player's language has no matching name. Prefer [`default_names`](#objects.Object.default_names).

      Returns: a list of string.

    - <a id="objects.Object.get_names" name="objects.Object.get_names"></a>[lua]`object:get_names()`  
      Every name the object answers to, in the player's language. Prefer [`names`](#objects.Object.names).

      Returns: a list of string.

    - <a id="objects.Object.get_property" name="objects.Object.get_property"></a>[lua]`object:get_property(name)`  
      Reads one of the object's properties. Prefer `object.properties.<name>`.

      Parameters:
      - <a id="objects.Object.get_property.name" name="objects.Object.get_property.name"></a>**`name`** (string). Which property, as the object declares it.

      Returns: any or `nil`. The value, of the type the property is declared with.

    - <a id="objects.Object.get_property_names" name="objects.Object.get_property_names"></a>[lua]`object:get_property_names()`  
      Names of every property this object declares.

      Returns: a list of string.

    - <a id="objects.Object.set_property" name="objects.Object.set_property"></a>[lua]`object:set_property(name, value)`  
      Writes one of the object's properties. Prefer `object.properties.<name> = ...`.

      Parameters:
      - <a id="objects.Object.set_property.name" name="objects.Object.set_property.name"></a>**`name`** (string). Which property, as the object declares it.
      - <a id="objects.Object.set_property.value" name="objects.Object.set_property.value"></a>**`value`** (any). What to write, of the type the property is declared with.

- <a id="objects.ObjectQuery" name="objects.ObjectQuery"></a>[lua]`trx.objects.ObjectQuery`

    A [`trx.query.Query`](QUERY.md#query.Query) over every object the engine knows, with the narrowings below on top of the ones every query has. Objects answer to names, so it carries the name layer too - see [`trx.query.NamedQuery`](QUERY.md#query.NamedQuery).

    The families do not cover [`pickup`](#objects.ObjectQuery.pickup) between them: a second state of something Lara already carries, such as a part-full waterskin, is in none of them.

    Methods:

    - <a id="objects.ObjectQuery.ammo" name="objects.ObjectQuery.ammo"></a>[lua]`objectquery:ammo()`  
      Clips for a weapon.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.animation" name="objects.ObjectQuery.animation"></a>[lua]`objectquery:animation()`  
      An animation an object borrows rather than a thing of its own.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.boss" name="objects.ObjectQuery.boss"></a>[lua]`objectquery:boss()`  
      A creature the game treats as a boss, which the enemy health bar can be held to.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.collectible" name="objects.ObjectQuery.collectible"></a>[lua]`objectquery:collectible()`  
      A collectible, by the slot it fills.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.creature" name="objects.ObjectQuery.creature"></a>[lua]`objectquery:creature()`  
      The object is a creature.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.door" name="objects.ObjectQuery.door"></a>[lua]`objectquery:door()`  
      A door.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.enemy" name="objects.ObjectQuery.enemy"></a>[lua]`objectquery:enemy()`  
      A creature that fights Lara rather than for her.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.examine" name="objects.ObjectQuery.examine"></a>[lua]`objectquery:examine()`  
      An examine item, by the slot it fills.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.gun" name="objects.ObjectQuery.gun"></a>[lua]`objectquery:gun()`  
      A weapon.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.inventory_item" name="objects.ObjectQuery.inventory_item"></a>[lua]`objectquery:inventory_item()`  
      An icon in the inventory rather than a thing in the world.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.key" name="objects.ObjectQuery.key"></a>[lua]`objectquery:key()`  
      A key, by the slot it fills.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.loaded" name="objects.ObjectQuery.loaded"></a>[lua]`objectquery:loaded()`  
      The level loaded the object, so items of it exist.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.loyal" name="objects.ObjectQuery.loyal"></a>[lua]`objectquery:loyal()`  
      One of Lara's own: the butler, and Lara herself.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.null_object" name="objects.ObjectQuery.null_object"></a>[lua]`objectquery:null_object()`  
      A placeholder that is never drawn.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.pickup" name="objects.ObjectQuery.pickup"></a>[lua]`objectquery:pickup()`  
      Something Lara can pick up.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.pushable" name="objects.ObjectQuery.pushable"></a>[lua]`objectquery:pushable()`  
      A block Lara pushes and pulls.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.puzzle" name="objects.ObjectQuery.puzzle"></a>[lua]`objectquery:puzzle()`  
      A puzzle item, by the slot it fills.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.quest" name="objects.ObjectQuery.quest"></a>[lua]`objectquery:quest()`  
      A quest item, by the slot it fills. This is what carries the scion.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.receptacle" name="objects.ObjectQuery.receptacle"></a>[lua]`objectquery:receptacle()`  
      A slot a puzzle item goes into.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.secret" name="objects.ObjectQuery.secret"></a>[lua]`objectquery:secret()`  
      The trinket a secret trigger sits under.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.spawnable" name="objects.ObjectQuery.spawnable"></a>[lua]`objectquery:spawnable()`  
      The object is a thing in the world at all, rather than an inventory icon, an animation, or a null placeholder.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.supply" name="objects.ObjectQuery.supply"></a>[lua]`objectquery:supply()`  
      A pickup Lara spends rather than keeps.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.switch" name="objects.ObjectQuery.switch"></a>[lua]`objectquery:switch()`  
      A switch Lara throws.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

    - <a id="objects.ObjectQuery.tool" name="objects.ObjectQuery.tool"></a>[lua]`objectquery:tool()`  
      A pickup named for itself rather than filling a numbered slot: the crowbar, the lasersight, the binoculars, the waterskins, the leadbar.

      Returns: [trx.query.Query](QUERY.md#query.Query). The narrowed query.

### Functions

- <a id="objects.get" name="objects.get"></a>[lua]`trx.objects.get(key)`  
  Retrieves an object definition by id or by name.

  Parameters:
  - <a id="objects.get.key" name="objects.get.key"></a>**`key`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). Object id, or its catalog name: `trx.objects["wolf"]`.

  Returns: [trx.objects.Object](#objects.Object) or `nil`. `nil` if no such object exists.

  Example:
  ```lua
  local wolf = trx.objects.wolf
  wolf.properties.max_hit_points = 30
  ```

- <a id="objects.swap_mesh" name="objects.swap_mesh"></a>[lua]`trx.objects.swap_mesh(object_id1, object_id2, [mesh_num1], [mesh_num2])`  
  Swaps meshes between two objects. With no mesh numbers, swaps all of them; with both, swaps just those two. One without the other raises.

  Parameters:
  - <a id="objects.swap_mesh.object_id1" name="objects.swap_mesh.object_id1"></a>**`object_id1`** ([trx.catalog.objects](CATALOG.md#catalog.objects)).
  - <a id="objects.swap_mesh.object_id2" name="objects.swap_mesh.object_id2"></a>**`object_id2`** ([trx.catalog.objects](CATALOG.md#catalog.objects)).
  - <a id="objects.swap_mesh.mesh_num1" name="objects.swap_mesh.mesh_num1"></a>**`mesh_num1`** ([trx.objects.MeshNum](#objects.MeshNum), optional). Mesh of the first.
  - <a id="objects.swap_mesh.mesh_num2" name="objects.swap_mesh.mesh_num2"></a>**`mesh_num2`** ([trx.objects.MeshNum](#objects.MeshNum), optional). Mesh of the second.

- <a id="objects.swap_sprite" name="objects.swap_sprite"></a>[lua]`trx.objects.swap_sprite(object_id1, object_id2)`  
  Swaps the sprites of two objects, which is how a pickup looks when 3D pickups
  are turned off. Raises if either object is drawn from meshes rather than a
  sprite.

  Parameters:
  - <a id="objects.swap_sprite.object_id1" name="objects.swap_sprite.object_id1"></a>**`object_id1`** ([trx.catalog.objects](CATALOG.md#catalog.objects)).
  - <a id="objects.swap_sprite.object_id2" name="objects.swap_sprite.object_id2"></a>**`object_id2`** ([trx.catalog.objects](CATALOG.md#catalog.objects)).
