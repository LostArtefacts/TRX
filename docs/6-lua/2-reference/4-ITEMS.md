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

    Writable properties:
    - `pos` (updating this also updates `room`)
    - `rot`
    - `hit_points` (updating this also may increase `max_hit_points`)
    - `max_hit_points`

### Functions

- [lua]`TRX.Items.Count()`  
  Returns the total number of allocated items.

- [lua]`TRX.Items.Get(index)`  
  Retrieves the [lua]`TRX.Items.ITEM` at the given zero-based index, or `nil` if out of range.
