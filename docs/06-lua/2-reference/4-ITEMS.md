---
title: Items
---

## Items module

Module for controlling all moveables behavior.

### Structures

- [lua]`trx.items.Item`

    Represents an item, also known as a moveable.

    Properties:

    - **`pos`**: A table with fields `x`, `y`, `z` representing position.
    - **`rot`**: A table with fields `x`, `y`, `z` representing rotation.
    - **`room`**: Integer representing the room number.
    - **`status`**: Integer representing the item's status.
    - **`hit_points`**: Integer representing the item's hit points.
    - **`max_hit_points`**: Integer representing the item's hit points.
    - **`object_id`**: Integer ID of the item's object type.
    - **`name`**: String name of the item, or `nil` if none.

    Writable properties:
    - `pos` (updating this also updates `room`)
    - `rot`
    - `hit_points` (updating this also may increase `max_hit_points`)
    - `max_hit_points`
    - `name` (string identifier; setting duplicates raises an error)

### Functions

-- Uses Lua length operator on the items table:
- [lua]`#trx.items`  
  Returns the total number of allocated items.

- [lua]`trx.items[num]`  
  [lua]`trx.items["name"]`  
  Retrieves the [lua]`trx.items.Item` at the given 1-based index or with the given `name`, or `nil` if out of range/not found.  

  Example:
  ```lua
  local item = trx.items[1]
  item.name = "lara"
  local lara = trx.items["lara"]
  ```

- [lua]`trx.items.fn.get(arg)`  
  Alias of `trx.items[arg]`.

  Example:
  ```lua
  local item_hp = trx.items.fn.get(17).hit_points
  local lara_hp = trx.items.fn.get("lara").hit_points
  ```
