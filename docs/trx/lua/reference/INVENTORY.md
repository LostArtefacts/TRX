---
title: Inventory
order: 4
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: tools/lint/gen/lua_docs
  The public API is declared next to its implementation, in
  src/lua/trx/inventory.lua. Edit it there.
-->

## <a id="inventory" name="inventory"></a>Inventory module

What Lara is carrying, and what goes into it.

The module's functions ask about the inventory she holds now, so
`trx.inventory.count(object)` asks about her. Any level's is a
[`trx.inventory.Inventory`](#inventory.Inventory), reached through [`trx.game.Level.inventory`](GAME.md#game.Level.inventory), which
is what it will hand her when she arrives there rather than what she has
this second.

Every function takes either the pickup lying in the world or the inventory
icon it goes into. The engine maps one to the other, so a script names
whichever it has.

### Indexing

Indexing the module reaches an entry of Lara's inventory, and
`#trx.inventory` is how many kinds of thing she carries. Entries are keyed
by the order they are drawn in, and are built one at a time as they are
asked for. `pairs()` walks them.

- <a id="inventory[]" name="inventory[]"></a>**`trx.inventory[key]`** (key: [trx.inventory.EntryNum](#inventory.EntryNum), value: [trx.inventory.Entry](#inventory.Entry) or `nil`).
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
    naming the same thing as what is drawn around it changes. A box of
    ammunition is an entry like any other, counting what its rounds come to.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="inventory.Entry.count" name="inventory.Entry.count"></a>**`count`**: integer. How many of it there are. Writing 0 takes it away.
    - <a id="inventory.Entry.object" name="inventory.Entry.object"></a>**`object`**: [trx.catalog.objects](CATALOG.md#catalog.objects). The inventory icon this entry is drawn as. *(read-only)*

- <a id="inventory.Inventory" name="inventory.Inventory"></a>[lua]`trx.inventory.Inventory`

    An inventory: what is in it, and how much ammunition goes with it.

    [`trx.inventory`](#inventory) is the one Lara is carrying. A level's, reached as
    [`trx.game.Level.inventory`](GAME.md#game.Level.inventory), is what she will arrive there with, and holds
    only what travels between levels - a key or a puzzle piece belongs to the
    level it was found in.

    Giving something to Lara's does what walking over it would: a weapon arrives
    with its rounds, her meshes change, and the level's own guns turn into
    ammunition for it. Giving it to a level's only says what she will arrive
    carrying.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Methods:

    - <a id="inventory.Inventory.ammo_kind" name="inventory.Inventory.ammo_kind"></a>[lua]`inventory:ammo_kind(weapon)`  
      The kind of ammunition the weapon fires, one of
      [`trx.weapons.Weapon.ammo_objects`](WEAPONS.md#weapons.Weapon.ammo_objects). A weapon fires its first kind until
      another is set.

      Parameters:
      - <a id="inventory.Inventory.ammo_kind.weapon" name="inventory.Inventory.ammo_kind.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.

      Returns: [trx.catalog.objects](CATALOG.md#catalog.objects) or `nil`. The kind, or `nil` where the weapon takes no ammunition.

    - <a id="inventory.Inventory.can_add" name="inventory.Inventory.can_add"></a>[lua]`inventory:can_add(object_id)`  
      Whether [`give`](#inventory.Inventory.give) would do anything in the level being
      played. The level has to carry the inventory model, which is not the same as
      the pickup being in it: a level with no shotgun lying about still draws one
      in the ring, which is what lets a cheat hand one over.

      This asks about the level being played whichever inventory it is called on.

      Parameters:
      - <a id="inventory.Inventory.can_add.object_id" name="inventory.Inventory.can_add.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

      Returns: boolean. True where the level carries the model to draw it with.

    - <a id="inventory.Inventory.count" name="inventory.Inventory.count"></a>[lua]`inventory:count(object_id)`  
      How many of something is in it. A box of ammunition counts what its rounds
      come to.

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
      How many entries there are. `#trx.inventory` is the same number for the one
      Lara carries.

      Returns: integer. Kinds of thing, not counts.

    - <a id="inventory.Inventory.give" name="inventory.Inventory.give"></a>[lua]`inventory:give(object_id, [count])`  
      Puts a pickup in. Lara's inventory takes it as walking over it would, so a
      weapon arrives with the rounds a pickup carries and a flare box with its
      flares; a level's simply gains it.

      Parameters:
      - <a id="inventory.Inventory.give.object_id" name="inventory.Inventory.give.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.
      - <a id="inventory.Inventory.give.count" name="inventory.Inventory.give.count"></a>**`count`** (integer, optional). How many. Defaults to 1; below 1 raises.

      Returns: integer. How many went in. 0 from Lara's means the level does not carry the icon for it - see [`can_add`](#inventory.Inventory.can_add).

    - <a id="inventory.Inventory.has" name="inventory.Inventory.has"></a>[lua]`inventory:has(object_id)`  
      Whether there is any of it at all.

      Parameters:
      - <a id="inventory.Inventory.has.object_id" name="inventory.Inventory.has.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

      Returns: boolean. True for any count above 0.

    - <a id="inventory.Inventory.has_attachment" name="inventory.Inventory.has_attachment"></a>[lua]`inventory:has_attachment(weapon)`  
      Whether the weapon carries its attachment, such as the laser sight on the
      TR4 revolver. [`trx.weapons.Weapon.attachment`](WEAPONS.md#weapons.Weapon.attachment) says what the weapon takes.

      Parameters:
      - <a id="inventory.Inventory.has_attachment.weapon" name="inventory.Inventory.has_attachment.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.

      Returns: boolean. True where the weapon carries its attachment.

    - <a id="inventory.Inventory.has_weapon" name="inventory.Inventory.has_weapon"></a>[lua]`inventory:has_weapon(weapon)`  
      Whether the weapon itself is in it, which is not the same as having
      ammunition for it.

      Parameters:
      - <a id="inventory.Inventory.has_weapon.weapon" name="inventory.Inventory.has_weapon.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.

      Returns: boolean. True where the weapon itself is in it.

    - <a id="inventory.Inventory.set_ammo_kind" name="inventory.Inventory.set_ammo_kind"></a>[lua]`inventory:set_ammo_kind(weapon, kind)`  
      Sets the kind of ammunition the weapon fires. The weapon keeps the kind
      when it runs out of it.

      Parameters:
      - <a id="inventory.Inventory.set_ammo_kind.weapon" name="inventory.Inventory.set_ammo_kind.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
      - <a id="inventory.Inventory.set_ammo_kind.kind" name="inventory.Inventory.set_ammo_kind.kind"></a>**`kind`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). One of [`trx.weapons.Weapon.ammo_objects`](WEAPONS.md#weapons.Weapon.ammo_objects). Any other object raises.

    - <a id="inventory.Inventory.set_count" name="inventory.Inventory.set_count"></a>[lua]`inventory:set_count(object_id, count)`  
      Sets how many of it there are. Zero takes it away.

      Parameters:
      - <a id="inventory.Inventory.set_count.object_id" name="inventory.Inventory.set_count.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.
      - <a id="inventory.Inventory.set_count.count" name="inventory.Inventory.set_count.count"></a>**`count`** (integer). How many. Below 0 raises.

    - <a id="inventory.Inventory.set_shots" name="inventory.Inventory.set_shots"></a>[lua]`inventory:set_shots(weapon, count, [kind])`  
      Sets how many shots there are for it.

      Parameters:
      - <a id="inventory.Inventory.set_shots.weapon" name="inventory.Inventory.set_shots.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
      - <a id="inventory.Inventory.set_shots.count" name="inventory.Inventory.set_shots.count"></a>**`count`** (integer). Shots. Below 0 raises.
      - <a id="inventory.Inventory.set_shots.kind" name="inventory.Inventory.set_shots.kind"></a>**`kind`** ([trx.catalog.objects](CATALOG.md#catalog.objects), optional). The kind of ammunition, one of [`trx.weapons.Weapon.ammo_objects`](WEAPONS.md#weapons.Weapon.ammo_objects). Defaults to the kind the weapon fires; any other object raises.

    - <a id="inventory.Inventory.shots" name="inventory.Inventory.shots"></a>[lua]`inventory:shots(weapon, [kind])`  
      How many shots there are for the weapon. A shot is one pull of the trigger,
      which is what the counter shows the player; the shotgun spends six rounds on
      each.

      Parameters:
      - <a id="inventory.Inventory.shots.weapon" name="inventory.Inventory.shots.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
      - <a id="inventory.Inventory.shots.kind" name="inventory.Inventory.shots.kind"></a>**`kind`** ([trx.catalog.objects](CATALOG.md#catalog.objects), optional). The kind of ammunition, one of [`trx.weapons.Weapon.ammo_objects`](WEAPONS.md#weapons.Weapon.ammo_objects). Defaults to the kind the weapon fires; any other object raises.

      Returns: integer. 0 where she carries no ammunition for it.

    - <a id="inventory.Inventory.take" name="inventory.Inventory.take"></a>[lua]`inventory:take(object_id, [count])`  
      Takes things back out, stopping when there are none left.

      This is not the exact opposite of [`give`](#inventory.Inventory.give): a box of
      ammunition is rounds rather than an entry of its own, so taking one back
      takes the rounds a box is worth.

      Parameters:
      - <a id="inventory.Inventory.take.object_id" name="inventory.Inventory.take.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.
      - <a id="inventory.Inventory.take.count" name="inventory.Inventory.take.count"></a>**`count`** (integer, optional). How many. Defaults to 1; below 1 raises.

      Returns: integer. How many came out.

- <a id="inventory.Actions" name="inventory.Actions"></a>[lua]`trx.inventory.Actions`

    What the inventory ring offers for an entry.

    Properties:
    - <a id="inventory.Actions.combine" name="inventory.Actions.combine"></a>**`combine`**: boolean. Whether Lara carries something it combines with, so that choosing it combines rather than uses it.
    - <a id="inventory.Actions.examine" name="inventory.Actions.examine"></a>**`examine`**: boolean. Whether it can be examined, which the player's settings and its description decide.
    - <a id="inventory.Actions.separate" name="inventory.Actions.separate"></a>**`separate`**: boolean. Whether it is a weapon that carries its attachment, which [`trx.inventory.separate`](#inventory.separate) takes off.
    - <a id="inventory.Actions.use" name="inventory.Actions.use"></a>**`use`**: boolean. Whether choosing it uses it.

### Functions

- <a id="inventory.count" name="inventory.count"></a>[lua]`trx.inventory.count(object_id)`  
  Asks [`trx.inventory.Inventory:count`](#inventory.Inventory.count) of what Lara carries.

  Parameters:
  - <a id="inventory.count.object_id" name="inventory.count.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

  Returns: integer. 0 where there is none.

- <a id="inventory.set_count" name="inventory.set_count"></a>[lua]`trx.inventory.set_count(object_id, count)`  
  Asks [`trx.inventory.Inventory:set_count`](#inventory.Inventory.set_count) of what Lara carries.

  Parameters:
  - <a id="inventory.set_count.object_id" name="inventory.set_count.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.
  - <a id="inventory.set_count.count" name="inventory.set_count.count"></a>**`count`** (integer). How many. Below 0 raises.

- <a id="inventory.has" name="inventory.has"></a>[lua]`trx.inventory.has(object_id)`  
  Asks [`trx.inventory.Inventory:has`](#inventory.Inventory.has) of what Lara carries.

  Parameters:
  - <a id="inventory.has.object_id" name="inventory.has.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

  Returns: boolean. True for any count above 0.

- <a id="inventory.give" name="inventory.give"></a>[lua]`trx.inventory.give(object_id, [count])`  
  Asks [`trx.inventory.Inventory:give`](#inventory.Inventory.give) of what Lara carries.

  Parameters:
  - <a id="inventory.give.object_id" name="inventory.give.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.
  - <a id="inventory.give.count" name="inventory.give.count"></a>**`count`** (integer, optional). How many. Defaults to 1; below 1 raises.

  Returns: integer. How many went in. 0 from Lara's means the level does not carry the icon for it - see [`trx.inventory.Inventory:can_add`](#inventory.Inventory.can_add).

  Example:
  ```lua
  trx.inventory.give(trx.catalog.objects.uzi_item, 2)
  ```

- <a id="inventory.take" name="inventory.take"></a>[lua]`trx.inventory.take(object_id, [count])`  
  Asks [`trx.inventory.Inventory:take`](#inventory.Inventory.take) of what Lara carries.

  Parameters:
  - <a id="inventory.take.object_id" name="inventory.take.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.
  - <a id="inventory.take.count" name="inventory.take.count"></a>**`count`** (integer, optional). How many. Defaults to 1; below 1 raises.

  Returns: integer. How many came out.

- <a id="inventory.shots" name="inventory.shots"></a>[lua]`trx.inventory.shots(weapon, [kind])`  
  Asks [`trx.inventory.Inventory:shots`](#inventory.Inventory.shots) of what Lara carries.

  Parameters:
  - <a id="inventory.shots.weapon" name="inventory.shots.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
  - <a id="inventory.shots.kind" name="inventory.shots.kind"></a>**`kind`** ([trx.catalog.objects](CATALOG.md#catalog.objects), optional). The kind of ammunition, one of [`trx.weapons.Weapon.ammo_objects`](WEAPONS.md#weapons.Weapon.ammo_objects). Defaults to the kind the weapon fires; any other object raises.

  Returns: integer. 0 where she carries no ammunition for it.

- <a id="inventory.ammo_kind" name="inventory.ammo_kind"></a>[lua]`trx.inventory.ammo_kind(weapon)`  
  Asks [`trx.inventory.Inventory:ammo_kind`](#inventory.Inventory.ammo_kind) of what Lara carries.

  Parameters:
  - <a id="inventory.ammo_kind.weapon" name="inventory.ammo_kind.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.

  Returns: [trx.catalog.objects](CATALOG.md#catalog.objects) or `nil`. The kind, or `nil` where the weapon takes no ammunition.

- <a id="inventory.set_ammo_kind" name="inventory.set_ammo_kind"></a>[lua]`trx.inventory.set_ammo_kind(weapon, kind)`  
  Asks [`trx.inventory.Inventory:set_ammo_kind`](#inventory.Inventory.set_ammo_kind) of what Lara carries.

  Parameters:
  - <a id="inventory.set_ammo_kind.weapon" name="inventory.set_ammo_kind.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
  - <a id="inventory.set_ammo_kind.kind" name="inventory.set_ammo_kind.kind"></a>**`kind`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). One of [`trx.weapons.Weapon.ammo_objects`](WEAPONS.md#weapons.Weapon.ammo_objects). Any other object raises.

  Example:
  ```lua
  trx.inventory.set_ammo_kind(
    trx.catalog.weapons.SHOTGUN,
    trx.catalog.objects.SHOTGUN_AMMO_2_ITEM
  )
  ```

- <a id="inventory.set_shots" name="inventory.set_shots"></a>[lua]`trx.inventory.set_shots(weapon, count, [kind])`  
  Asks [`trx.inventory.Inventory:set_shots`](#inventory.Inventory.set_shots) of what Lara carries.

  Parameters:
  - <a id="inventory.set_shots.weapon" name="inventory.set_shots.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
  - <a id="inventory.set_shots.count" name="inventory.set_shots.count"></a>**`count`** (integer). Shots. Below 0 raises.
  - <a id="inventory.set_shots.kind" name="inventory.set_shots.kind"></a>**`kind`** ([trx.catalog.objects](CATALOG.md#catalog.objects), optional). The kind of ammunition, one of [`trx.weapons.Weapon.ammo_objects`](WEAPONS.md#weapons.Weapon.ammo_objects). Defaults to the kind the weapon fires; any other object raises.

  Example:
  ```lua
  trx.inventory.set_shots(trx.catalog.weapons.UZIS, 2000)
  ```

- <a id="inventory.has_weapon" name="inventory.has_weapon"></a>[lua]`trx.inventory.has_weapon(weapon)`  
  Asks [`trx.inventory.Inventory:has_weapon`](#inventory.Inventory.has_weapon) of what Lara carries.

  Parameters:
  - <a id="inventory.has_weapon.weapon" name="inventory.has_weapon.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.

  Returns: boolean. True where the weapon itself is in it.

- <a id="inventory.has_attachment" name="inventory.has_attachment"></a>[lua]`trx.inventory.has_attachment(weapon)`  
  Asks [`trx.inventory.Inventory:has_attachment`](#inventory.Inventory.has_attachment) of what Lara carries.

  Parameters:
  - <a id="inventory.has_attachment.weapon" name="inventory.has_attachment.weapon"></a>**`weapon`** ([trx.catalog.weapons](CATALOG.md#catalog.weapons)). Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.

  Returns: boolean. True where the weapon carries its attachment.

- <a id="inventory.entry" name="inventory.entry"></a>[lua]`trx.inventory.entry(object_id)`  
  Asks [`trx.inventory.Inventory:entry`](#inventory.Inventory.entry) of what Lara carries.

  Parameters:
  - <a id="inventory.entry.object_id" name="inventory.entry.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

  Returns: [trx.inventory.Entry](#inventory.Entry) or `nil`. The entry, or `nil` where there is none of it.

- <a id="inventory.entry_at" name="inventory.entry_at"></a>[lua]`trx.inventory.entry_at(entry_num)`  
  Asks [`trx.inventory.Inventory:entry_at`](#inventory.Inventory.entry_at) of what Lara carries.

  Parameters:
  - <a id="inventory.entry_at.entry_num" name="inventory.entry_at.entry_num"></a>**`entry_num`** ([trx.inventory.EntryNum](#inventory.EntryNum)).

  Returns: [trx.inventory.Entry](#inventory.Entry) or `nil`. The entry, or `nil` past the last one.

- <a id="inventory.entry_count" name="inventory.entry_count"></a>[lua]`trx.inventory.entry_count()`  
  Asks [`trx.inventory.Inventory:entry_count`](#inventory.Inventory.entry_count) of what Lara carries.

  Returns: integer. Kinds of thing, not counts.

- <a id="inventory.can_add" name="inventory.can_add"></a>[lua]`trx.inventory.can_add(object_id)`  
  Asks [`trx.inventory.Inventory:can_add`](#inventory.Inventory.can_add) of what Lara carries.

  Parameters:
  - <a id="inventory.can_add.object_id" name="inventory.can_add.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

  Returns: boolean. True where the level carries the model to draw it with.

- <a id="inventory.combine_partners" name="inventory.combine_partners"></a>[lua]`trx.inventory.combine_partners(object_id)`  
  What an inventory icon combines with, among the things Lara carries.

  Parameters:
  - <a id="inventory.combine_partners.object_id" name="inventory.combine_partners.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The inventory icon to combine.

  Returns: a list of [trx.catalog.objects](CATALOG.md#catalog.objects). The icons of what it combines with.

  Example:
  ```lua
  for _, partner in ipairs(trx.inventory.combine_partners(item)) do
    trx.log.info(trx.catalog.objects[partner])
  end
  ```

- <a id="inventory.combine" name="inventory.combine"></a>[lua]`trx.inventory.combine(object_id, partner_id)`  
  Combines two things Lara carries into what they make, taking one of each,
  as combining them in the inventory ring does.

  Parameters:
  - <a id="inventory.combine.object_id" name="inventory.combine.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The inventory icon to combine.
  - <a id="inventory.combine.partner_id" name="inventory.combine.partner_id"></a>**`partner_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The inventory icon to combine it with.

  Returns: [trx.catalog.objects](CATALOG.md#catalog.objects) or `nil`. What they made, or `nil`, changing nothing, where the two do not combine or Lara lacks either.

  Example:
  ```lua
  local made = trx.inventory.combine(item, partner)
  ```

- <a id="inventory.separate" name="inventory.separate"></a>[lua]`trx.inventory.separate(object_id)`  
  Takes the attachment off a weapon Lara carries, such as the laser sight off
  the TR4 revolver, and gives it back to her.

  Parameters:
  - <a id="inventory.separate.object_id" name="inventory.separate.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The weapon's pickup, or the inventory icon it goes into.

  Returns: [trx.catalog.objects](CATALOG.md#catalog.objects) or `nil`. The icon the attachment went into, or `nil`, changing nothing, where the weapon carries none.

  Example:
  ```lua
  trx.inventory.separate(trx.catalog.objects.REVOLVER_OPTION)
  ```

- <a id="inventory.icon_of" name="inventory.icon_of"></a>[lua]`trx.inventory.icon_of(object)`  
  Returns the inventory icon for a pickup, whether or not Lara has one.

  Parameters:
  - <a id="inventory.icon_of.object" name="inventory.icon_of.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup to check.

  Returns: [trx.catalog.objects](CATALOG.md#catalog.objects) or `nil`. The icon's object id, or `nil` for a pickup that has none.

- <a id="inventory.use" name="inventory.use"></a>[lua]`trx.inventory.use(object_id)`  
  Uses something Lara carries, as choosing it in the inventory ring does:
  she draws a weapon, takes a medipack, lights a flare, or uses a key on what
  she stands at. Something that has no use does nothing.

  Parameters:
  - <a id="inventory.use.object_id" name="inventory.use.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

  Example:
  ```lua
  trx.inventory.use(trx.catalog.objects.SMALL_MEDIPACK_OPTION)
  ```

- <a id="inventory.actions" name="inventory.actions"></a>[lua]`trx.inventory.actions(object_id)`  
  What the inventory ring offers for an entry: using, examining, combining or
  separating it.

  Parameters:
  - <a id="inventory.actions.object_id" name="inventory.actions.object_id"></a>**`object_id`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The pickup, or the inventory icon it goes into.

  Returns: [trx.inventory.Actions](#inventory.Actions). What it offers.

  Example:
  ```lua
  if trx.inventory.actions(object).examine then
    trx.log.info("it can be examined")
  end
  ```
