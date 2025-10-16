---
title: Items
---

## Items module

Module for controlling all moveables behavior.

### Structures

- [lua]`TRX.Items.ITEM`

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

- [lua]`TRX.Items.Count()`  
  Returns the total number of allocated items.

- [lua]`TRX.Items.Get(arg)`  
  Retrieves the [lua]`TRX.Items.ITEM` at the given 1-based index or with the given `name`, or `nil` if out of range/not found.  
  Example:
  ```lua
  local item = TRX.Items.Get(1)
  item.name = "lara"
  local lara = TRX.Items.Get("lara")
  ```
