-- Deleting a save, as the passport and the flat menu both offer it: a button
-- that the player holds over a save, a question that asks again with No picked
-- first, and the message that a failed delete shows.
--
-- Usage:
--   local delete_save = require("common.ui.delete_save")
--   local button = delete_save.button()
--   if button:control(keys, delete_save.is_deletable(slot)) then
--     local req = delete_save.confirm_req({ y = 0.5 })
--     ...
--     if delete_save.is_yes(req:selected_row()) then
--       delete_save.delete(slot.slot_num, slot.pool, show_message)
--     end
--   end

local hold_button = require("common.ui.widgets.hold_button")
local requester = require("common.ui.widgets.requester")

local M = {}

---@alias common.ui.delete_save.Slot { slot_num?: integer, pool?: integer }

-- Returns the height that the button takes, in canvas units at the default
-- text size.
function M.button_height()
  return hold_button.height()
end

-- Creates the button, set against the right of the width it is given. It
-- starts hidden.
---@return common.ui.HoldButton
function M.button()
  return hold_button.new({
    words = trx.locale.text("general/passport/delete_save"),
    align = trx.ui.HAlign.RIGHT,
  })
end

-- Returns whether the slot holds a save to delete.
---@param slot? common.ui.delete_save.Slot
---@return boolean
function M.is_deletable(slot)
  return slot ~= nil
    and slot.slot_num ~= nil
    and trx.savegame.info(slot.slot_num, slot.pool) ~= nil
end

-- Creates the dialog that asks whether to delete the save, with No picked.
---@param place trx.ui.Placement|fun(w: number, h: number): number, number
---@return common.ui.Requester
function M.confirm_req(place)
  local req = requester.new({
    title = trx.locale.text("general/passport/delete_save_confirm"),
    rows = {
      {
        text = trx.locale.text("general/passport/delete_save_yes"),
        yes = true,
      },
      { text = trx.locale.text("general/passport/delete_save_no") },
    },
    arrows = false,
    place = place,
  })
  req.list:select(2)
  return req
end

-- Returns whether the row of the dialog is Yes.
---@return boolean
function M.is_yes(row)
  return row ~= nil and row.yes == true
end

-- Deletes the save. Returns whether it is gone; when it is not, runs
-- `on_fail` with the text that says so.
---@param slot_num trx.savegame.SlotNum
---@param pool trx.savegame.Pool
---@param on_fail fun(text: string|trx.locale.Text)
---@return boolean
function M.delete(slot_num, pool, on_fail)
  if trx.savegame.delete(slot_num, pool) then
    return true
  end
  on_fail(trx.locale.text("general/passport/delete_save_failed"))
  return false
end

return M
