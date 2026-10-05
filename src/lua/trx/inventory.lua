local raw = trxc.inventory
local h = require("trx.internal.helpers")

---@class trx
---@field inventory trx.inventory

---What Lara is carrying, and what goes into it.
---
---The module's functions ask about the inventory she holds now, so
---`trx.inventory.count(object)` asks about her. Any level's is a
---`trx.inventory.Inventory`, reached through `trx.game.Level.inventory`, which
---is what it will hand her when she arrives there rather than what she has
---this second.
---
---Every function takes either the pickup lying in the world or the inventory
---icon it goes into. The engine maps one to the other, so a script names
---whichever it has.
---@trx.module 4
---@class (exact) trx.inventory
local M = h.module("inventory")

---Where an entry sits in the ring, in the order they are drawn.
---@trx.base 1
---@alias trx.inventory.EntryNum integer

---One kind of thing an inventory holds, and how many of it.
---
---An entry stands for the icon rather than for where it sits, so it goes on
---naming the same thing as what is drawn around it changes. A box of
---ammunition is an entry like any other, counting what its rounds come to.
---@class (exact) trx.inventory.Entry
---@trx.readonly object
---@field object trx.catalog.objects The inventory icon this entry is drawn as.
---@field count integer How many of it there are. Writing 0 takes it away.
local Entry = h.handle("inventory.Entry", "INVENTORY_ENTRY", {
  fields = { object = "object_id", count = "qty" },
  writable = { "count" },
})

---An inventory: what is in it, and how much ammunition goes with it.
---
---`trx.inventory` is the one Lara is carrying. A level's, reached as
---`trx.game.Level.inventory`, is what she will arrive there with, and holds
---only what travels between levels - a key or a puzzle piece belongs to the
---level it was found in.
---
---Giving something to Lara's does what walking over it would: a weapon arrives
---with its rounds, her meshes change, and the level's own guns turn into
---ammunition for it. Giving it to a level's only says what she will arrive
---carrying.
---@class (exact) trx.inventory.Inventory
local Inventory = h.handle("inventory.Inventory", "INVENTORY_STATE", {})

---How many of something is in it. A box of ammunition counts what its rounds
---come to.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@return integer # 0 where there is none.
function Inventory:count(object_id) end

---Sets how many of it there are. Zero takes it away.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@param count integer How many. Below 0 raises.
function Inventory:set_count(object_id, count) end

---Whether there is any of it at all.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@return boolean # True for any count above 0.
function Inventory:has(object_id) end

---Puts a pickup in. Lara's inventory takes it as walking over it would, so a
---weapon arrives with the rounds a pickup carries and a flare box with its
---flares; a level's simply gains it.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@param count? integer How many. Defaults to 1; below 1 raises.
---@return integer # How many went in. 0 from Lara's means the level does not carry the icon for it - see `trx.inventory.Inventory:can_add`.
function Inventory:give(object_id, count) end

---Takes things back out, stopping when there are none left.
---
---This is not the exact opposite of `trx.inventory.Inventory:give`: a box of
---ammunition is rounds rather than an entry of its own, so taking one back
---takes the rounds a box is worth.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@param count? integer How many. Defaults to 1; below 1 raises.
---@return integer # How many came out.
function Inventory:take(object_id, count) end

---How many shots there are for the weapon. A shot is one pull of the trigger,
---which is what the counter shows the player; the shotgun spends six rounds on
---each.
---@param weapon trx.catalog.weapons Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
---@return integer # 0 where she carries no ammunition for it.
function Inventory:shots(weapon) end

---Sets how many shots there are for it.
---@param weapon trx.catalog.weapons Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
---@param count integer Shots. Below 0 raises.
function Inventory:set_shots(weapon, count) end

---Whether the weapon itself is in it, which is not the same as having
---ammunition for it.
---@param weapon trx.catalog.weapons Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
---@return boolean # True where the weapon itself is in it.
function Inventory:has_weapon(weapon) end

---The entry something is drawn as, or `nil` where there is none of it.
---
---Several pickups share one entry - the scion whether or not she holds it, a
---waterskin at each fill level - so this answers with the one thing they are
---drawn as.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@return trx.inventory.Entry? # The entry, or `nil` where there is none of it.
function Inventory:entry(object_id) end

---The entry at a position in the order they are drawn, or `nil` past the end.
---@param entry_num trx.inventory.EntryNum
---@return trx.inventory.Entry? # The entry, or `nil` past the last one.
function Inventory:entry_at(entry_num) end

---How many entries there are. `#trx.inventory` is the same number for the one
---Lara carries.
---@return integer # Kinds of thing, not counts.
function Inventory:entry_count() end

---Whether `trx.inventory.Inventory:give` would do anything in the level being
---played. The level has to carry the inventory model, which is not the same as
---the pickup being in it: a level with no shotgun lying about still draws one
---in the ring, which is what lets a cheat hand one over.
---
---This asks about the level being played whichever inventory it is called on.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@return boolean # True where the level carries the model to draw it with.
function Inventory:can_add(object_id) end

---Asks `trx.inventory.Inventory:count` of what Lara carries.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@return integer # 0 where there is none.
function M.count(object_id)
  return raw.get_current():count(object_id)
end

---Asks `trx.inventory.Inventory:set_count` of what Lara carries.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@param count integer How many. Below 0 raises.
function M.set_count(object_id, count)
  return raw.get_current():set_count(object_id, count)
end

---Asks `trx.inventory.Inventory:has` of what Lara carries.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@return boolean # True for any count above 0.
function M.has(object_id)
  return raw.get_current():has(object_id)
end

---Asks `trx.inventory.Inventory:give` of what Lara carries.
---
---```lua
---trx.inventory.give(trx.catalog.objects.uzi_item, 2)
---```
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@param count? integer How many. Defaults to 1; below 1 raises.
---@return integer # How many went in. 0 from Lara's means the level does not carry the icon for it - see `trx.inventory.Inventory:can_add`.
function M.give(object_id, count)
  return raw.get_current():give(object_id, count)
end

---Asks `trx.inventory.Inventory:take` of what Lara carries.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@param count? integer How many. Defaults to 1; below 1 raises.
---@return integer # How many came out.
function M.take(object_id, count)
  return raw.get_current():take(object_id, count)
end

---Asks `trx.inventory.Inventory:shots` of what Lara carries.
---@param weapon trx.catalog.weapons Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
---@return integer # 0 where she carries no ammunition for it.
function M.shots(weapon)
  return raw.get_current():shots(weapon)
end

---Asks `trx.inventory.Inventory:set_shots` of what Lara carries.
---
---```lua
---trx.inventory.set_shots(trx.catalog.weapons.UZIS, 2000)
---```
---@param weapon trx.catalog.weapons Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
---@param count integer Shots. Below 0 raises.
function M.set_shots(weapon, count)
  return raw.get_current():set_shots(weapon, count)
end

---Asks `trx.inventory.Inventory:has_weapon` of what Lara carries.
---@param weapon trx.catalog.weapons Which weapon. `UNKNOWN` and `UNARMED` raise, and so does anything outside the table; `FLARE` and `SKIDOO` are taken, being held the way a weapon is.
---@return boolean # True where the weapon itself is in it.
function M.has_weapon(weapon)
  return raw.get_current():has_weapon(weapon)
end

---Asks `trx.inventory.Inventory:entry` of what Lara carries.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@return trx.inventory.Entry? # The entry, or `nil` where there is none of it.
function M.entry(object_id)
  return raw.get_current():entry(object_id)
end

---Asks `trx.inventory.Inventory:entry_at` of what Lara carries.
---@param entry_num trx.inventory.EntryNum
---@return trx.inventory.Entry? # The entry, or `nil` past the last one.
function M.entry_at(entry_num)
  return raw.get_current():entry_at(entry_num)
end

---Asks `trx.inventory.Inventory:entry_count` of what Lara carries.
---@return integer # Kinds of thing, not counts.
function M.entry_count()
  return raw.get_current():entry_count()
end

---Asks `trx.inventory.Inventory:can_add` of what Lara carries.
---@param object_id trx.catalog.objects The pickup, or the inventory icon it goes into.
---@return boolean # True where the level carries the model to draw it with.
function M.can_add(object_id)
  return raw.get_current():can_add(object_id)
end

---Indexing the module reaches an entry of Lara's inventory, and
---`#trx.inventory` is how many kinds of thing she carries. Entries are keyed
---by the order they are drawn in, and are built one at a time as they are
---asked for. `pairs()` walks them.
---
---```lua
---for _, entry in pairs(trx.inventory) do
---  trx.log.info(("%d x %s"):format(entry.count, trx.catalog.objects[entry.object]))
---end
---```
---@type table<trx.inventory.EntryNum, trx.inventory.Entry?>
h.container("inventory", {
  base = 1,
  get = function(n)
    return raw.get_current():entry_at(n)
  end,
  count = function()
    return raw.get_current():entry_count()
  end,
}, M)
