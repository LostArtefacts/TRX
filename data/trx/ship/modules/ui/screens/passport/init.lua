-- Draws the passport entry of the inventory ring: its pages, the page turns,
-- and every list that the pages open. The quick save and load screens and the
-- save crystal entry show one page of the passport alone.

local book = require("common.ui.screens.passport.book")
local pages = require("common.ui.screens.passport.pages")
local ring = require("common.ui.ring")

local M = {}

-- Fallback text for the passport's own strings. The strings files and their
-- translations take precedence.
trx.locale.declare({
  ["general/passport/new_game"] = "New Game",
  ["general/passport/select_mod"] = "Select Game",
  ["general/passport/save_slot_unsupported"] = "This save does not support this feature.",
  ["general/passport/restart_level"] = "Restart Level",
  ["general/passport/exit_to_title"] = "Exit to Title",
  ["general/passport/exit_game"] = "Exit Game",
  ["general/passport/mode_new_game"] = "New Game",
  ["general/passport/mode_new_game_plus"] = "New Game+",
  ["general/passport/select_mode"] = "Select Mode",
  ["general/passport/select_save"] = "Select Save",
  ["general/passport/select_level"] = "Select Level",
  ["general/passport/story_so_far"] = "Story so far...",
  ["general/passport/switch_mod"] = "Switch Game",
  ["general/passport/play_previous_levels"] = "Play previous levels",
})

---@param ctx trx.ui.EntryContext
local function open(ctx)
  local mode = ring.mode()
  local book_pages = pages.determine_pages(mode)
  local index = pages.first_available(book_pages)
  if index == nil then
    return nil
  end
  return book.open(ctx, book_pages, index, mode)
end

-- Opens the page of the quick save and load screens. A page with no list
-- leaves the screen closed.
---@param ctx trx.ui.SaveLoadContext
local function open_save_load(ctx)
  local book_pages = pages.determine_pages(ctx.mode)
  local index = pages.first_available(book_pages)
  if index == nil or pages.FLAT[book_pages[index].role] then
    return nil
  end
  local opened = book.open_alone(ctx, book_pages, index, false)
  if opened == nil then
    return nil
  end
  if ctx.mode == trx.ui.MenuMode.LOAD then
    trx.overlay.set_caption(trx.locale.get("general/passport/load_game"))
  else
    trx.overlay.set_caption(trx.locale.get("general/passport/save_game"))
  end
  ctx:on_end(function()
    trx.overlay.set_caption()
  end)
  return opened
end

local function name_crystal()
  local crystal = trx.catalog.objects.SAVE_CRYSTAL_ITEM
  local name = trx.locale.get("objects/save_crystal_item/name"):match("^[^|]*")
  trx.overlay.set_caption(name)
  ring.show_count(trx.inventory.count(crystal))
end

-- Opens the save page of the save crystal entry. The crystal saves only in the
-- save pickup mode, and only from the ring that the player opened. The ring
-- leaves the caption to the page, so the page names the crystal.
---@param ctx trx.ui.EntryContext
local function open_crystal(ctx)
  if
    trx.config.get("gameplay.save_crystal_mode") ~= "save_pickup"
    or ring.mode() ~= trx.ui.MenuMode.GAME
  then
    return nil
  end
  local opened = book.open_alone(
    ctx,
    { { role = pages.Page.SAVE_GAME, available = true } },
    1,
    true
  )
  if opened == nil then
    return nil
  end
  name_crystal()
  return opened
end

function M.setup()
  trx.ui.screens.define(
    trx.ui.Screen.RING_ENTRY,
    open,
    { object = trx.catalog.objects.PASSPORT_OPTION }
  )
  trx.ui.screens.define(trx.ui.Screen.SAVE_LOAD, open_save_load)
  trx.ui.screens.define(
    trx.ui.Screen.RING_ENTRY,
    open_crystal,
    { object = trx.catalog.objects.SAVE_CRYSTAL_OPTION }
  )
end

return M
