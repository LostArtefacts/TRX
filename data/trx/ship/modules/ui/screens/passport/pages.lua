-- The pages of the passport, and which of them the book offers in each mode
-- of the inventory ring.

local ring = require("common.ui.ring")

local M = {}

M.Page = {
  LOAD_GAME = "load_game",
  SAVE_GAME = "save_game",
  NEW_GAME = "new_game",
  SELECT_LEVEL = "select_level",
  RESTART_LEVEL = "restart_level",
  EXIT_TO_TITLE = "exit_to_title",
  EXIT_GAME = "exit_game",
}

local Page = M.Page

M.CAPTIONS = {
  [Page.LOAD_GAME] = "general/passport/load_game",
  [Page.SAVE_GAME] = "general/passport/save_game",
  [Page.NEW_GAME] = "general/passport/new_game",
  [Page.SELECT_LEVEL] = "general/passport/select_level",
  [Page.RESTART_LEVEL] = "general/passport/restart_level",
  [Page.EXIT_TO_TITLE] = "general/passport/exit_to_title",
  [Page.EXIT_GAME] = "general/passport/exit_game",
}

-- Pages that act on confirm and open no list.
M.FLAT = {
  [Page.RESTART_LEVEL] = true,
  [Page.EXIT_TO_TITLE] = true,
  [Page.EXIT_GAME] = true,
}

-------------------------------------------------------------------------------
-- What the game offers
-------------------------------------------------------------------------------

local function is_in_gym()
  local level = trx.game.current_level
  return level ~= nil and level.type == trx.game.LevelType.GYM
end

-- Returns false when Lara is dead, because the passport is then the only way
-- on.
function M.can_back_out()
  return ring.mode() ~= trx.ui.MenuMode.DEATH
end

-- Returns whether a page opens only after the player confirms it. TR1 does
-- this; the later games open a page as soon as the book stops on it.
function M.browses_first(mode)
  return trx.game.tr_version == 1
    and (
      mode == trx.ui.MenuMode.TITLE
      or mode == trx.ui.MenuMode.GAME
      or mode == trx.ui.MenuMode.DEATH
    )
end

function M.is_title()
  local level = trx.game.current_level
  return level == nil or level.type == trx.game.LevelType.TITLE
end

function M.game_modes_available()
  local policy = trx.config.get("gameplay.game_modes_policy")
  if policy == "never" then
    return false
  end
  if policy == "always" then
    return true
  end
  return trx.config.get("profile.new_game_plus_unlock")
end

-- Visits the quick saves first, then the numbered slots.
function M.each_slot(fn)
  for _, pool in ipairs({ trx.savegame.Pool.QUICK, trx.savegame.Pool.NORMAL }) do
    for slot_num = 1, trx.savegame.slot_count(pool) do
      fn(slot_num, pool)
    end
  end
end

local function has_saves()
  local found = false
  M.each_slot(function(slot_num, pool)
    found = found or not trx.savegame.is_free(slot_num, pool)
  end)
  return found
end

function M.switchable_mods()
  local mods = {}
  for _, mod in ipairs(trx.mod.list) do
    if trx.mod.can_switch(mod) then
      mods[#mods + 1] = mod
    end
  end
  return mods
end

function M.saving_enabled()
  return trx.savegame.slot_count(trx.savegame.Pool.NORMAL) > 0
    and not trx.config.get("flow.load_save_disabled")
end

-------------------------------------------------------------------------------
-- The pages
--
-- The book has three pages. A page with nothing on it stays empty, so that
-- every page keeps its place in the book.
-------------------------------------------------------------------------------

function M.first_available(pages)
  for i, page in ipairs(pages) do
    if page.available then
      return i
    end
  end
  return nil
end

---@class common.ui.passport.BookPage
---@field role? string
---@field available? boolean

-- Puts an exit on the last page when no page is available, so that the player
-- always has a way out.
local function ensure_way_out(pages, mode)
  if M.first_available(pages) ~= nil then
    return
  end
  pages[3] = {
    role = mode == trx.ui.MenuMode.TITLE and Page.EXIT_GAME
      or Page.EXIT_TO_TITLE,
    available = true,
  }
end

function M.determine_pages(mode)
  ---@type common.ui.passport.BookPage[]
  local pages = { {}, {}, {} }
  local function set(slot, role, available)
    pages[slot] = { role = role, available = available }
  end

  local saves = has_saves() and M.saving_enabled()
  local can_restart = trx.game.can_restart_level()

  if mode == trx.ui.MenuMode.TITLE then
    set(1, Page.LOAD_GAME, saves)
    set(2, Page.NEW_GAME, true)
    set(3, Page.EXIT_GAME, true)
  elseif mode == trx.ui.MenuMode.GAME then
    if not M.saving_enabled() then
      set(2, Page.RESTART_LEVEL, can_restart)
    else
      set(1, Page.LOAD_GAME, saves)
      set(2, Page.SAVE_GAME, true)
    end
    set(3, Page.EXIT_TO_TITLE, true)
  elseif mode == trx.ui.MenuMode.LOAD then
    if not M.saving_enabled() then
      set(2, Page.RESTART_LEVEL, can_restart)
    elseif saves then
      set(1, Page.LOAD_GAME, true)
    else
      set(2, Page.SAVE_GAME, true)
    end
  elseif
    mode == trx.ui.MenuMode.SAVE or mode == trx.ui.MenuMode.SAVE_CRYSTAL
  then
    if not M.saving_enabled() then
      set(2, Page.RESTART_LEVEL, can_restart)
    else
      set(2, Page.SAVE_GAME, true)
    end
  elseif mode == trx.ui.MenuMode.DEATH then
    set(1, Page.LOAD_GAME, saves)
    set(2, Page.RESTART_LEVEL, can_restart)
    set(3, Page.EXIT_TO_TITLE, true)
  end

  -- The gym offers a new game in place of a save. A level that forbids manual
  -- saves offers a restart, or nothing.
  for _, page in ipairs(pages) do
    if page.role == Page.SAVE_GAME then
      if is_in_gym() then
        page.role = Page.NEW_GAME
      elseif
        not trx.savegame.manual_allowed
        and mode ~= trx.ui.MenuMode.SAVE_CRYSTAL
      then
        if can_restart then
          page.role = Page.RESTART_LEVEL
        else
          page.available = false
        end
      end
    end
  end

  if trx.config.get("flow.play_any_level") then
    for _, page in ipairs(pages) do
      if page.role == Page.NEW_GAME then
        page.role = Page.SELECT_LEVEL
      end
    end
  end

  ensure_way_out(pages, mode)
  return pages
end

return M
