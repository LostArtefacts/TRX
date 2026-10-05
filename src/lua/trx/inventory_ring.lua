local raw = trxc.inventory_ring
local h = require("trx.internal.helpers")

---@class trx
---@field inventory_ring trx.inventory_ring

---The rings the player browses, and the entries drawn on them.
---
---This is the front of the inventory: which entries a ring holds, how each is
---drawn and turned, and what the player has picked. What Lara is carrying
---belongs to `trx.inventory`.
---
---A script draws what an entry opens by defining the
---`trx.ui.Screen.RING_ENTRY` screen.
---@trx.module 5 Inventory ring
---@class (exact) trx.inventory_ring
local M = h.module("inventory_ring")

---What the inventory ring was opened for.
---@enum trx.inventory_ring.Mode
local Mode = {
  ---Opened during play.
  GAME = h.IntegerConstant,
  ---The title screen's menu.
  TITLE = h.IntegerConstant,
  ---The keys ring, opened against a locked door or receptacle.
  KEYS = h.IntegerConstant,
  ---Opened to save, with the save list already on show.
  SAVE = h.IntegerConstant,
  ---Opened to load, with the save list already on show.
  LOAD = h.IntegerConstant,
  ---Opened because Lara died.
  DEATH = h.IntegerConstant,
  ---Opened by a save crystal.
  SAVE_CRYSTAL = h.IntegerConstant,
  ---The globe the player picks a destination from.
  GLOBE_SELECT = h.IntegerConstant,
}
M.Mode = h.enum("inventory_ring.Mode", "INVENTORY_MODE", Mode)

---What the open ring was opened for, or `nil` when no ring is open.
---@return trx.inventory_ring.Mode? # What the ring was opened for.
---@type fun(): trx.inventory_ring.Mode?
M.mode = raw.mode

---The animation state of the selected entry.
---@class (exact) trx.inventory_ring.EntryAnim
---@field frame integer The frame on show.
---@field goal_frame integer The frame the entry is animating towards.
---@field open_frame integer The frame the entry rests on once it has opened.
---@field frame_count integer How many frames the entry's animation holds.
---@field direction integer Which way the animation runs: `1` forwards, `-1` backwards.
local EntryAnim = h.class("inventory_ring.EntryAnim")

---Where the ring's selected entry is in its animation, or `nil` when no ring
---is open.
---@return trx.inventory_ring.EntryAnim? # The entry's animation state.
---@type fun(): trx.inventory_ring.EntryAnim?
M.selection_anim = raw.selection_anim

---Runs the ring's selected entry to a frame of its animation.
---
---```lua
----- turn the passport to its second page
---local anim = trx.inventory_ring.selection_anim()
---trx.inventory_ring.animate_selection(anim.open_frame + 5, 1)
---```
---@param goal_frame integer The frame to stop on.
---@param direction integer Which way to run: `1` forwards, `-1` backwards.
---@type fun(goal_frame: integer, direction: integer)
M.animate_selection = raw.animate_selection

---Returns the inventory icon for a pickup, whether or not Lara has one.
---@param object trx.catalog.objects The pickup to check.
---@return trx.catalog.objects? # The icon's object id, or `nil` for a pickup that has none.
---@type fun(object: trx.catalog.objects): trx.catalog.objects?
M.icon_of = raw.icon_of

---Returns the ring entry for an object.
---
---Returns `nil` when no entry names the object.
---@param object trx.catalog.objects The inventory icon to read.
---@return table # The entry's `object_id`, frame counts, rotations, offsets, `scale`, and `draws_at_pivot`, or `nil`. <!--noref: object_id--> <!--noref: scale, draws_at_pivot-->
---@type fun(object: trx.catalog.objects): table
M.item = raw.item

---Adds an object to the inventory ring and sets its display properties.
---
---Declaring an existing object replaces its entry. Use the pickup object for
---an entry that represents itself.
---
---```lua
---trx.inventory_ring.declare_item({
---  object_id = "mymod:lantern_item",
---  frames_total = 1,
---  anim_direction = 1,
---  anim_speed = 1,
---  scale = 1.0,
---  meshes_sel = -1,
---  meshes_drawn = -1,
---  inv_pos = 20,
---})
---```
---@param spec table The entry's `object_id`, frame counts, rotations, offsets, `scale`, and `draws_at_pivot`. An omitted value keeps the ring's default. <!--noref: object_id--> <!--noref: scale, draws_at_pivot-->
function M.declare_item(spec)
  assert(
    type(spec) == "table",
    "trx.inventory_ring.declare_item expects a table"
  )
  return raw.declare_item(spec)
end
