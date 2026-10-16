local raw = trxc.inventory_ring
local raw_events = trxc.events
local h = require("trx.internal.helpers")
local lasting_polled = require("trx.signal").lasting_polled

---@class (partial) trx
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

---What the inventory ring tells a script, for the screens that a script draws
---over it.
---@class (exact) trx.inventory_ring.signals
---@trx.readonly count, hint, rest_frame
---@field hint trx.signal.Signal Holds the key hints that the ring shows under the name of the selected entry, as a list of texts that stand apart from one another, empty while it shows none or no ring is open. `trx.inventory_ring.set_input_hints` sets them.
---@field count trx.signal.Signal Holds the count that the ring shows for an entry Lara carries more than one of, or nil while it shows none or no ring is open.
---@field rest_frame trx.signal.Signal Says when the entry that the player picked comes to rest, with the frame of its animation that it rests on, and when it moves again, goes back into the ring or the ring closes, with `nil`. A screen of an entry opens once the entry rests, and the passport reads the page it rests on.
M.signals = h.namespace("inventory_ring.signals")

-- The engine says when the selected entry comes to rest and when it moves
-- again, with -1 for the latter. The handler is attached as the module loads,
-- so it belongs to no level: one that a level script attached would go with
-- the level.
local resting_on = nil
---@type trx.signal.Signal?
local rest_frame = nil
if raw_events ~= nil then
  raw_events.attach(h.constants("LUA_EVENT_TYPE").RING_REST, function(frame)
    resting_on = frame >= 0 and frame or nil
    if rest_frame ~= nil then
      rest_frame:set(resting_on)
    end
  end)
end

local function rest_frame_signal()
  if rest_frame == nil then
    rest_frame = trx.signal.new(resting_on)
  end
  return rest_frame
end

---@type trx.signal.Signal?
local hint = nil
---@type trx.signal.Signal?
local count = nil

h.properties(M.signals, "inventory_ring.signals", {
  rest_frame = { get = rest_frame_signal },
  hint = {
    get = function()
      if hint == nil then
        hint = lasting_polled(raw.hint_revision):map(function()
          return { raw.hint() }
        end)
      end
      return hint
    end,
  },
  count = {
    get = function()
      if count == nil then
        count = lasting_polled(raw.count)
      end
      return count
    end,
  },
})

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

---Turns the selected entry by hand, as examining it does: first about the
---vertical axis, then about the horizontal one. The turns add up until the
---player stops examining the entry.
---
---```lua
---trx.inventory_ring.turn_selection(trx.math.degrees(3), 0)
---```
---@param yaw trx.math.Angle How far to turn it left.
---@param pitch trx.math.Angle How far to tip it towards the player.
---@type fun(yaw: trx.math.Angle, pitch: trx.math.Angle)
M.turn_selection = raw.turn_selection

---A key hint the ring shows: the keys bound to an input role and what they
---do.
---@class (exact) trx.inventory_ring.InputHint
---@field input trx.ui.Inputs The role, or the roles shown side by side.
---@field text? string What the keys do.

---Shows hints for the keys that work now where the ring shows its button
---hints, under the name of the selected entry, set apart from one another as
---the ring sets apart its own. Given nothing, takes them down.
---
---```lua
---trx.inventory_ring.set_input_hints({
---  { input = trx.input.Role.MENU_CONFIRM, text = "Use" },
---  { input = trx.input.Role.MENU_BACK, text = "Cancel" },
---})
---```
---@param hints? trx.inventory_ring.InputHint[] The hints, up to four.
function M.set_input_hints(hints)
  local parts = {}
  for i, hint in ipairs(hints or {}) do
    parts[i] = trx.ui.input_hint_text(hint.input, hint.text)
  end
  raw.set_hint(table.unpack(parts))
end

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
