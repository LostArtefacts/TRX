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

## <a id="inventory" name="inventory"></a>Inventory module

What Lara is carrying, and what goes into it.

The module is the inventory she holds now, so `trx.inventory:count(object)`
asks about her. Any level's is reached the same way through
[`trx.game.Level.inventory`](GAME.md#game.Level.inventory), which is what it will hand her when she arrives
there rather than what she has this second.

Every function takes either the pickup lying in the world or the inventory icon
it goes into. The engine maps one to the other, so a script names whichever it
has.

### Indexing

Indexing the module reaches an entry of Lara's inventory, and `#trx.inventory` is how many kinds of thing she carries. Entries are keyed by the order they are drawn in, and are built one at a time as they are asked for. `pairs()` walks them.

- <a id="inventory[]" name="inventory[]"></a>**`trx.inventory[key]`** ([trx.inventory.Entry](#inventory.Entry) or `nil`).
- **`#trx.inventory`** (integer). How many there are.

Example:
```lua
for _, entry in pairs(trx.inventory) do
  trx.log.info(("%d x %s"):format(entry.count, trx.catalog.objects[entry.object]))
end
```

### Structures

- <a id="inventory.EntryNum" name="inventory.EntryNum"></a>[lua]`trx.inventory.EntryNum`

    Where an entry sits in the ring, in the order they are drawn. Counted from 1.

- <a id="inventory.Entry" name="inventory.Entry"></a>[lua]`trx.inventory.Entry`

    One kind of thing an inventory holds, and how many of it.

    An entry stands for the icon rather than for where it sits, so it goes on
    naming the same thing as what is drawn around it changes. A box of ammunition
    is an entry like any other, counting what its rounds come to.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="inventory.Entry.count" name="inventory.Entry.count"></a>**`count`**: integer. How many of it there are. Writing 0 takes it away.
    - <a id="inventory.Entry.object" name="inventory.Entry.object"></a>**`object`**: [trx.catalog.objects](CATALOG.md#catalog.objects). The inventory icon this entry is drawn as. *(read-only)*

- <a id="inventory.Inventory" name="inventory.Inventory"></a>[lua]`trx.inventory.Inventory`

    An inventory: what is in it, and how much ammunition goes with it.

    [`trx.inventory`](#inventory) is the one Lara is carrying. A level's, reached as
    [`trx.game.Level.inventory`](GAME.md#game.Level.inventory), is what she will arrive there with, and holds only
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

    - <a id="inventory.Inventory.can_add" name="inventory.Inventory.can_add"></a>[lua]`inventory:can_add(object_id)`  
      Whether [`give`](#inventory.Inventory.give) would do anything in the level being played. The level has to
      carry the inventory model, which is not the same as the pickup being in it: a
      level with no shotgun lying about still draws one in the ring, which is what
      lets a cheat hand one over.

      This asks about the level being played whichever inventory it is called on.

      Parameters:
      - <a id="inventory.Inventory.can_add.object_id" name="inventory.Inventory.can_add.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

      Returns: boolean. True where the level carries the model to draw it with.

    - <a id="inventory.Inventory.count" name="inventory.Inventory.count"></a>[lua]`inventory:count(object_id)`  
      How many of something is in it. A box of ammunition counts what its rounds come to.

      Parameters:
      - <a id="inventory.Inventory.count.object_id" name="inventory.Inventory.count.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

      Returns: integer. 0 where there is none.

    - <a id="inventory.Inventory.entry" name="inventory.Inventory.entry"></a>[lua]`inventory:entry(object_id)`  
      The entry something is drawn as, or `nil` where there is none of it.

      Several pickups share one entry - the scion whether or not she holds it, a
      waterskin at each fill level - so this answers with the one thing they are
      drawn as.

      Parameters:
      - <a id="inventory.Inventory.entry.object_id" name="inventory.Inventory.entry.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

      Returns: [trx.inventory.Entry](#inventory.Entry) or `nil`. The entry, or `nil` where there is none of it.

    - <a id="inventory.Inventory.entry_at" name="inventory.Inventory.entry_at"></a>[lua]`inventory:entry_at(entry_num)`  
      The entry at a position in the order they are drawn, or `nil` past the end.

      Parameters:
      - <a id="inventory.Inventory.entry_at.entry_num" name="inventory.Inventory.entry_at.entry_num"></a>**`entry_num`** ([trx.inventory.EntryNum](#inventory.EntryNum)).

      Returns: [trx.inventory.Entry](#inventory.Entry) or `nil`. The entry, or `nil` past the last one.

    - <a id="inventory.Inventory.entry_count" name="inventory.Inventory.entry_count"></a>[lua]`inventory:entry_count()`  
      How many entries there are. `#trx.inventory` is the same number for the one Lara carries.

      Returns: integer. Kinds of thing, not counts.

    - <a id="inventory.Inventory.give" name="inventory.Inventory.give"></a>[lua]`inventory:give(object_id, [count])`  
      Puts a pickup in. Lara's inventory takes it as walking over it would, so a
      weapon arrives with the rounds a pickup carries and a flare box with its
      flares; a level's simply gains it.

      Parameters:
      - <a id="inventory.Inventory.give.object_id" name="inventory.Inventory.give.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.
      - <a id="inventory.Inventory.give.count" name="inventory.Inventory.give.count"></a>**`count`** (integer, optional). How many. Defaults to 1; below 1 raises.

      Returns: integer. How many went in. 0 from Lara's means the level does not carry the icon for it - see [`can_add`](#inventory.Inventory.can_add).

      Example:
      ```lua
      trx.inventory:give(trx.catalog.objects.uzi_item, 2)
      ```

    - <a id="inventory.Inventory.has" name="inventory.Inventory.has"></a>[lua]`inventory:has(object_id)`  
      Whether there is any of it at all.

      Parameters:
      - <a id="inventory.Inventory.has.object_id" name="inventory.Inventory.has.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

      Returns: boolean. True for any count above 0.

    - <a id="inventory.Inventory.has_weapon" name="inventory.Inventory.has_weapon"></a>[lua]`inventory:has_weapon(weapon)`  
      Whether the weapon itself is in it, which is not the same as having ammunition for it.

      Parameters:
      - <a id="inventory.Inventory.has_weapon.weapon" name="inventory.Inventory.has_weapon.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.

      Returns: boolean. True where the weapon itself is in it.

    - <a id="inventory.Inventory.icon_of" name="inventory.Inventory.icon_of"></a>[lua]`inventory:icon_of(object_id)`  
      Which inventory icon a pickup is drawn as, whether or not there is any of it.

      Several pickups share one icon - the scion whether or not Lara holds it, a
      waterskin at each fill level - so this is what tells two spellings of one thing
      from two things. It answers with an object id rather than an entry; [`entry`](#inventory.Inventory.entry) is
      what hands back the entry itself.

      Parameters:
      - <a id="inventory.Inventory.icon_of.object_id" name="inventory.Inventory.icon_of.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

      Returns: [trx.catalog.objects](CATALOG.md#catalog.objects) or `nil`. The icon's object id, or `nil` for a pickup that has none.

    - <a id="inventory.Inventory.set_count" name="inventory.Inventory.set_count"></a>[lua]`inventory:set_count(object_id, count)`  
      Sets how many of it there are. Zero takes it away.

      Parameters:
      - <a id="inventory.Inventory.set_count.object_id" name="inventory.Inventory.set_count.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.
      - <a id="inventory.Inventory.set_count.count" name="inventory.Inventory.set_count.count"></a>**`count`** (integer). How many. Below 0 raises.

    - <a id="inventory.Inventory.set_shots" name="inventory.Inventory.set_shots"></a>[lua]`inventory:set_shots(weapon, count)`  
      Sets how many shots there are for it.

      Parameters:
      - <a id="inventory.Inventory.set_shots.weapon" name="inventory.Inventory.set_shots.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
      - <a id="inventory.Inventory.set_shots.count" name="inventory.Inventory.set_shots.count"></a>**`count`** (integer). Shots. Below 0 raises.

      Example:
      ```lua
      trx.inventory:set_shots(trx.catalog.weapons.UZIS, 2000)
      ```

    - <a id="inventory.Inventory.shots" name="inventory.Inventory.shots"></a>[lua]`inventory:shots(weapon)`  
      How many shots there are for the weapon. A shot is one pull of the trigger, which is what the counter shows the player; the shotgun spends six rounds on each.

      Parameters:
      - <a id="inventory.Inventory.shots.weapon" name="inventory.Inventory.shots.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.

      Returns: integer. 0 where she carries no ammunition for it.

    - <a id="inventory.Inventory.take" name="inventory.Inventory.take"></a>[lua]`inventory:take(object_id, [count])`  
      Takes things back out, stopping when there are none left.

      This is not the exact opposite of [`give`](#inventory.Inventory.give): a box of ammunition is rounds rather
      than an entry of its own, so taking one back takes the rounds a box is worth.

      Parameters:
      - <a id="inventory.Inventory.take.object_id" name="inventory.Inventory.take.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.
      - <a id="inventory.Inventory.take.count" name="inventory.Inventory.take.count"></a>**`count`** (integer, optional). How many. Defaults to 1; below 1 raises.

      Returns: integer. How many came out.
