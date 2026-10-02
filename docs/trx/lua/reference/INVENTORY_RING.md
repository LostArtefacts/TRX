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

This is the front of the inventory: which entries a ring holds, how each is
drawn and turned, and what the player has picked. What Lara is carrying belongs
to [`trx.inventory`](INVENTORY.md#inventory).

A script draws what an entry opens by defining the
[`trx.ui.Screen.RING_ENTRY`](UI.md#ui.Screen) screen.

### Enums

- <a id="inventory_ring.Mode" name="inventory_ring.Mode"></a>[lua]`trx.inventory_ring.Mode`

    What the inventory ring was opened for.

    - `trx.inventory_ring.Mode.GAME` = `0`  
        Opened during play.
    - `trx.inventory_ring.Mode.TITLE` = `1`  
        The title screen's menu.
    - `trx.inventory_ring.Mode.KEYS` = `2`  
        The keys ring, opened against a locked door or receptacle.
    - `trx.inventory_ring.Mode.SAVE` = `3`  
        Opened to save, with the save list already on show.
    - `trx.inventory_ring.Mode.LOAD` = `4`  
        Opened to load, with the save list already on show.
    - `trx.inventory_ring.Mode.DEATH` = `5`  
        Opened because Lara died.
    - `trx.inventory_ring.Mode.SAVE_CRYSTAL` = `6`  
        Opened by a save crystal.
    - `trx.inventory_ring.Mode.GLOBE_SELECT` = `7`  
        The globe the player picks a destination from.

### Structures

- <a id="inventory_ring.EntryAnim" name="inventory_ring.EntryAnim"></a>[lua]`trx.inventory_ring.EntryAnim`

    The animation state of the selected entry.

    Properties:
    - <a id="inventory_ring.EntryAnim.direction" name="inventory_ring.EntryAnim.direction"></a>**`direction`**: integer. Which way the animation runs: `1` forwards, `-1` backwards.
    - <a id="inventory_ring.EntryAnim.frame" name="inventory_ring.EntryAnim.frame"></a>**`frame`**: integer. The frame on show.
    - <a id="inventory_ring.EntryAnim.frame_count" name="inventory_ring.EntryAnim.frame_count"></a>**`frame_count`**: integer. How many frames the entry's animation holds.
    - <a id="inventory_ring.EntryAnim.goal_frame" name="inventory_ring.EntryAnim.goal_frame"></a>**`goal_frame`**: integer. The frame the entry is animating towards.
    - <a id="inventory_ring.EntryAnim.open_frame" name="inventory_ring.EntryAnim.open_frame"></a>**`open_frame`**: integer. The frame the entry rests on once it has opened.

### Functions

- <a id="inventory_ring.mode" name="inventory_ring.mode"></a>[lua]`trx.inventory_ring.mode()`  
  What the open ring was opened for, or `nil` when no ring is open.

  Returns: [trx.inventory_ring.Mode](#inventory_ring.Mode) or `nil`. What the ring was opened for.

- <a id="inventory_ring.selection_anim" name="inventory_ring.selection_anim"></a>[lua]`trx.inventory_ring.selection_anim()`  
  Where the ring's selected entry is in its animation, or `nil` when no ring is open.

  Returns: [trx.inventory_ring.EntryAnim](#inventory_ring.EntryAnim) or `nil`. The entry's animation state.

- <a id="inventory_ring.animate_selection" name="inventory_ring.animate_selection"></a>[lua]`trx.inventory_ring.animate_selection(goal_frame, direction)`  
  Runs the ring's selected entry to a frame of its animation.

  Parameters:
  - <a id="inventory_ring.animate_selection.goal_frame" name="inventory_ring.animate_selection.goal_frame"></a>**`goal_frame`** (integer). The frame to stop on.
  - <a id="inventory_ring.animate_selection.direction" name="inventory_ring.animate_selection.direction"></a>**`direction`** (integer). Which way to run: `1` forwards, `-1` backwards.

  Example:
  ```lua
  -- turn the passport to its second page
  local anim = trx.inventory_ring.selection_anim()
  trx.inventory_ring.animate_selection(anim.open_frame + 5, 1)
  ```

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
