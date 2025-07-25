---
title: LUA scripting
---

# Introduction

This is a highly experimental module used for internal test automation.
Over time we want to evolve it to support actual level scripting, but for now
it's not quite there yet.

# Functions

- `lara.get_item()`  
    Returns `TRX.ITEM` associated with Lara, or `nil` if the Lara object is not available.

- `console.log("string1", "string2")`  
    Logs a line to the developer console.

- `assert(condition)`  
    Shows an error if something is not true.

# Structures

## `TRX.ITEM`

Represents an item, also known as a moveable.

**Properties**:

- **`pos`**: A table with fields `x`, `y`, `z` representing position.
- **`rot`**: A table with fields `x`, `y`, `z` representing rotation.
- **`room`**: Integer representing the room number.
- **`status`**: Integer representing the item's status.
- **`hit_points`**: Integer representing the item's hit points.
