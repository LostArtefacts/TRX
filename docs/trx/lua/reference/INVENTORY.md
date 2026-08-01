---
title: Inventory
order: 4
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/inventory.lua. Edit it there.
-->

## Inventory module

What Lara is carrying, and what goes into it.

The module is the inventory she holds now, so `trx.inventory:count(object)`
asks about her. Any level's is reached the same way through
`trx.game.Level.inventory`, which is what it will hand her when she arrives
there rather than what she has this second.

Every function takes either the pickup lying in the world or the inventory icon
it goes into. The engine maps one to the other, so a script names whichever it
has.

### Indexing

Indexing the module reaches an entry of Lara's inventory, and `#trx.inventory` is how many kinds of thing she carries. Entries count from one, in the order they are drawn, and are built one at a time as they are asked for. `pairs()` walks them.

- **`trx.inventory[key]`** (Entry or `nil`). 1-based position.
- **`#trx.inventory`** (integer). How many there are.

Example:
```lua
for _, entry in pairs(trx.inventory) do
  trx.log.info(("%d x %s"):format(entry.count, trx.catalog.objects[entry.object]))
end
```

### Structures

- [lua]`trx.inventory.Entry`

    One kind of thing an inventory holds, and how many of it.

    An entry stands for the icon rather than for where it sits, so it goes on
    naming the same thing as what is drawn around it changes. A box of ammunition
    is an entry like any other, counting what its rounds come to.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`count`**: integer. How many of it there are. Writing 0 takes it away.
    - **`object`**: integer. The inventory icon this entry is drawn as. Compare against `trx.catalog.objects`. *(read-only)*

- [lua]`trx.inventory.Inventory`

    An inventory: what is in it, and how much ammunition goes with it.

    `trx.inventory` is the one Lara is carrying. A level's, reached as
    `trx.game.Level.inventory`, is what she will arrive there with, and holds only
    what travels between levels - a key or a puzzle piece belongs to the level it
    was found in.

    Giving something to Lara's does what walking over it would: a weapon arrives
    with its rounds, her meshes change, and the level's own guns turn into
    ammunition for it. Giving it to a level's only says what she will arrive
    carrying.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Methods:

    - [lua]`inventory:can_add(object_id)`  
      Whether `give` would do anything in the level being played. The level has to
      carry the inventory model, which is not the same as the pickup being in it: a
      level with no shotgun lying about still draws one in the ring, which is what
      lets a cheat hand one over.

      This asks about the level being played whichever inventory it is called on.

      Parameters:
      - **`object_id`** (integer). The pickup, or the inventory icon it goes into. Compare against `trx.catalog.objects`.

      Returns: boolean.

    - [lua]`inventory:count(object_id)`  
      How many of something is in it. A box of ammunition counts what its rounds come to.

      Parameters:
      - **`object_id`** (integer). The pickup, or the inventory icon it goes into. Compare against `trx.catalog.objects`.

      Returns: integer.

    - [lua]`inventory:entry(object_id)`  
      The entry something is drawn as, or `nil` where there is none of it.

      Several pickups share one entry - the scion whether or not she holds it, a
      waterskin at each fill level - so this answers with the one thing they are
      drawn as.

      Parameters:
      - **`object_id`** (integer). The pickup, or the inventory icon it goes into. Compare against `trx.catalog.objects`.

      Returns: Entry or `nil`.

    - [lua]`inventory:entry_at(entry_num)`  
      The entry at a position, counted from one in the order they are drawn, or `nil` past the end.

      Parameters:
      - **`entry_num`** (integer). 1-based position.

      Returns: Entry or `nil`.

    - [lua]`inventory:entry_count()`  
      How many entries there are. `#trx.inventory` is the same number for the one Lara carries.

      Returns: integer.

    - [lua]`inventory:give(object_id, [count])`  
      Puts a pickup in. Lara's inventory takes it as walking over it would, so a
      weapon arrives with the rounds a pickup carries and a flare box with its
      flares; a level's simply gains it.

      Parameters:
      - **`object_id`** (integer). The pickup, or the inventory icon it goes into. Compare against `trx.catalog.objects`.
      - **`count`** (integer, optional). How many. Defaults to 1; below 1 raises.

      Returns: integer. How many went in. 0 from Lara's means the level does not carry the icon for it - see `can_add`.

      Example:
      ```lua
      trx.inventory:give(trx.catalog.objects.uzi_item, 2)
      ```

    - [lua]`inventory:has(object_id)`  
      Whether there is any of it at all.

      Parameters:
      - **`object_id`** (integer). The pickup, or the inventory icon it goes into. Compare against `trx.catalog.objects`.

      Returns: boolean.

    - [lua]`inventory:has_weapon(weapon)`  
      Whether the weapon itself is in it, which is not the same as having ammunition for it.

      Parameters:
      - **`weapon`** (integer). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is. Compare against `trx.catalog.weapons`.

      Returns: boolean.

    - [lua]`inventory:icon_of(object_id)`  
      Which inventory icon a pickup is drawn as, whether or not there is any of it.

      Several pickups share one icon - the scion whether or not Lara holds it, a
      waterskin at each fill level - so this is what tells two spellings of one thing
      from two things. It answers with an object id rather than an entry; `entry` is
      what hands back the entry itself.

      Parameters:
      - **`object_id`** (integer). The pickup, or the inventory icon it goes into. Compare against `trx.catalog.objects`.

      Returns: integer or `nil`. The icon's object id, or `nil` for a pickup that has none. Compare against `trx.catalog.objects`.

    - [lua]`inventory:set_count(object_id, count)`  
      Sets how many of it there are. Zero takes it away.

      Parameters:
      - **`object_id`** (integer). The pickup, or the inventory icon it goes into. Compare against `trx.catalog.objects`.
      - **`count`** (integer). How many. Below 0 raises.

    - [lua]`inventory:set_shots(weapon, count)`  
      Sets how many shots there are for it.

      Parameters:
      - **`weapon`** (integer). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is. Compare against `trx.catalog.weapons`.
      - **`count`** (integer). Shots. Below 0 raises.

      Example:
      ```lua
      trx.inventory:set_shots(trx.catalog.weapons.UZIS, 2000)
      ```

    - [lua]`inventory:shots(weapon)`  
      How many shots there are for the weapon. A shot is one pull of the trigger, which is what the counter shows the player; the shotgun spends six rounds on each.

      Parameters:
      - **`weapon`** (integer). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is. Compare against `trx.catalog.weapons`.

      Returns: integer.

    - [lua]`inventory:take(object_id, [count])`  
      Takes things back out, stopping when there are none left.

      This is not the exact opposite of `give`: a box of ammunition is rounds rather
      than an entry of its own, so taking one back takes the rounds a box is worth.

      Parameters:
      - **`object_id`** (integer). The pickup, or the inventory icon it goes into. Compare against `trx.catalog.objects`.
      - **`count`** (integer, optional). How many. Defaults to 1; below 1 raises.

      Returns: integer. How many came out.
