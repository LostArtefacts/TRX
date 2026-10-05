local raw = trxc.savegame
local h = require("trx.internal.helpers")

---@class (partial) trx
---@field savegame trx.savegame

---Manage save slots and saved games.
---@trx.module 28
---@class (exact) trx.savegame
---@trx.readonly manual_allowed
---@field manual_allowed boolean Whether the current level allows manual saving.
local M = h.module("savegame")

---Which set of save slots a slot belongs to.
---@enum trx.savegame.Pool
local Pool = {
  ---The numbered save slots.
  NORMAL = h.IntegerConstant,
  ---The quick-save slots, counted and addressed by their on-screen order.
  QUICK = h.IntegerConstant,
}
M.Pool = h.enum("savegame.Pool", "SAVEGAME_SLOT_POOL", Pool)

---Slot number within the pool. For the quick pool this is the on-screen
---order.
---@trx.base 1
---@alias trx.savegame.SlotNum integer

---Counts the slots in a pool.
---
---The quick pool counts only slots that hold a save.
---@param pool? trx.savegame.Pool Which set of slots to look in. Defaults to `NORMAL`.
---@return integer # The number of slots.
function M.slot_count(pool)
  return raw.slot_count(pool or M.Pool.NORMAL)
end

---Whether a slot holds no save.
---@param slot_num trx.savegame.SlotNum
---@param pool? trx.savegame.Pool Which set of slots to look in. Defaults to `NORMAL`.
---@return boolean # Whether the slot is empty.
function M.is_free(slot_num, pool)
  return raw.is_free(slot_num, pool or M.Pool.NORMAL)
end

---Starts the saved game in a slot.
---
---The game flow loads it after this call returns. Raises when the slot holds
---no save.
---
---```lua
---trx.savegame.load(1)
---```
---@param slot_num trx.savegame.SlotNum
---@param pool? trx.savegame.Pool Which set of slots to look in. Defaults to `NORMAL`.
function M.load(slot_num, pool)
  raw.load(slot_num, pool or M.Pool.NORMAL)
end

---Writes a saved game to a slot.
---
---A quick save without a slot number uses the next slot in the rotation.
---Otherwise, it uses the named slot.
---
---```lua
---trx.savegame.save(1)
---```
---@param slot_num? trx.savegame.SlotNum The quick pool uses the next slot in its rotation when it is omitted.
---@param pool? trx.savegame.Pool Which set of slots to look in. Defaults to `NORMAL`.
---@return boolean # Whether the save was written. `false` means that the quick pool had no slot.
function M.save(slot_num, pool)
  return raw.save(slot_num, pool or M.Pool.NORMAL)
end

---What a slot holds, as the save list shows it.
---@class (exact) trx.savegame.SlotInfo
---@field level_title string The name of the level the save was made in.
---@field counter integer The save count when this save was written.
---@field level_num integer Where the level sits in the main level table.
---@field is_quick boolean Whether the save is a quick save.
---@field can_restart boolean Whether the level can be restarted from the save.
---@field can_select_level boolean Whether the save reaches an earlier level in the game.
---@field has_story boolean Whether story content runs before the saved level.
local SlotInfo = h.class("savegame.SlotInfo")

---Returns the slot contents, or `nil` if the slot is empty.
---
---```lua
---local info = trx.savegame.info(1)
---if info ~= nil then
---  trx.log.info(info.level_title)
---end
---```
---@param slot_num trx.savegame.SlotNum
---@param pool? trx.savegame.Pool Which set of slots to look in. Defaults to `NORMAL`.
---@return trx.savegame.SlotInfo? # What the slot holds.
function M.info(slot_num, pool)
  return raw.info(slot_num, pool or M.Pool.NORMAL)
end

---Removes the save from a slot and deletes its file.
---@param slot_num trx.savegame.SlotNum
---@param pool? trx.savegame.Pool Which set of slots to look in. Defaults to `NORMAL`.
---@return boolean # Whether a save was removed.
function M.delete(slot_num, pool)
  return raw.delete(slot_num, pool or M.Pool.NORMAL)
end

---Counts the saves in every pool.
---@return integer # The number of saves.
---@type fun(): integer
M.total_count = raw.total_count

---Reports whether the saved level can be restarted.
---
---With no slot, this uses the save the game is running from. A game that is
---not running from a save can always restart.
---@param slot_num? trx.savegame.SlotNum The slot to ask about. The running save answers when it is omitted.
---@param pool? trx.savegame.Pool Which set of slots to look in. Defaults to `NORMAL`.
---@return boolean # Whether the level can be restarted.
function M.restart_available(slot_num, pool)
  return raw.restart_available(
    slot_num,
    slot_num ~= nil and (pool or M.Pool.NORMAL) or nil
  )
end

---Returns the levels up to the saved one without starting it.
---@param slot_num trx.savegame.SlotNum
---@param pool? trx.savegame.Pool Which set of slots to look in. Defaults to `NORMAL`.
---@return trx.game.LevelNum[]? # The levels, or `nil` when the slot holds no save that can be read.
function M.reached_levels(slot_num, pool)
  return raw.reached_levels(slot_num, pool or M.Pool.NORMAL)
end

---Plays the story content that runs before the saved level.
---
---Raises when the slot holds no save, or when no story runs before it.
---@param slot_num trx.savegame.SlotNum
---@param pool? trx.savegame.Pool Which set of slots to look in. Defaults to `NORMAL`.
function M.play_story(slot_num, pool)
  raw.play_story(slot_num, pool or M.Pool.NORMAL)
end

---Returns the slot where a save list should open.
---
---This is the slot that the game last loaded or saved. If there is no such
---slot, it is the most recently written save, and then the first numbered
---slot.
---@return trx.savegame.SlotNum? # The slot number, or `nil` where the game keeps no slots.
---@return trx.savegame.Pool # Which pool it belongs to.
---@type fun(): trx.savegame.SlotNum?, trx.savegame.Pool
M.recent_slot = raw.recent_slot

h.properties(M, "savegame", {
  manual_allowed = { get = raw.manual_allowed },
})

local _ = SlotInfo
