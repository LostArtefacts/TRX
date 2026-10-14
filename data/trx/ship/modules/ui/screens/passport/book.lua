-- The passport's book: the page that it rests on, the lists opened over that
-- page, and what each choice in them does. Each time the passport opens, it
-- gets a book of its own. The page is one layer, and each list opened from it
-- is a layer above it. A choice that leaves the passport runs as a game flow
-- command once the ring has closed.

local hold_button = require("common.ui.widgets.hold_button")
local metrics = require("common.ui.metrics")
local pages = require("common.ui.screens.passport.pages")
local requester = require("common.ui.widgets.requester")
local rows = require("common.ui.screens.passport.rows")

local M = {}

-- Each page sits five frames past the one the book opens on.
local FRAMES_PER_PAGE = 5

local PASSPORT_WIDTH = 300.0

-- The gap between the caption and each of its arrows.
local CAPTION_ARROW_GAP = 15

---@class common.ui.passport.Passport
---@field ctx trx.ui.ScreenContext
---@field pages common.ui.passport.BookPage[]
---@field index integer
---@field lists common.ui.passport.Sheet[]
---@field page_shown trx.signal.Signal
---@field caption trx.signal.Signal
---@field caption_shown? trx.signal.Signal
---@field arrow_left? trx.signal.Signal
---@field arrow_right? trx.signal.Signal
---@field target? trx.signal.Signal
---@field browsing? trx.signal.Signal
---@field resting? trx.signal.Signal
---@field browse? boolean
---@field standalone? boolean
---@field crystal? boolean
---@field book? trx.ui.StackLayer
---@field page? common.ui.passport.Sheet
---@field opened? string
---@field bare? boolean
---@field message? trx.ui.StackLayer
---@field chosen_level? trx.game.LevelNum
local Passport = {}
Passport.__index = Passport

-- Returns the next available page in the given direction, or nil.
function Passport:neighbour(step)
  local i = self.index + step
  while i >= 1 and i <= #self.pages do
    if self.pages[i].available then
      return i
    end
    i = i + step
  end
  return nil
end

-------------------------------------------------------------------------------
-- The book
-------------------------------------------------------------------------------

---@param index integer
local function turn_to(index)
  local anim = trx.inventory_ring.selection_anim()
  if anim == nil then
    return
  end
  local goal = anim.open_frame + FRAMES_PER_PAGE * (index - 1)
  if anim.goal_frame == goal then
    return
  end
  trx.inventory_ring.animate_selection(
    goal,
    goal > anim.goal_frame and 1 or -1
  )
  trx.sound.play(trx.catalog.samples.MENU_PASSPORT)
end

-- Returns the page that the book rests on at `frame`, or nil while the book
-- turns.
local function page_at(frame)
  local anim = trx.inventory_ring.selection_anim()
  if frame == nil or anim == nil then
    return nil
  end
  local offset = frame - anim.open_frame
  if offset % FRAMES_PER_PAGE ~= 0 then
    return nil
  end
  return offset // FRAMES_PER_PAGE + 1
end

function Passport:set_caption(key)
  self.caption:set(
    trx.locale.format(
      "general/inventory_ring/object_name_fmt",
      trx.locale.get(key)
    )
  )
end

-- Shuts the book towards the nearer cover. A confirmed close leaves the ring;
-- otherwise the entry goes back into the ring. The quick save and load screens
-- have no book to shut, and take only a cancel. The save crystal has no book
-- either, and leaves the ring once it saves.
function Passport:close(confirmed)
  local anim = not self.standalone and trx.inventory_ring.selection_anim()
    or nil
  if anim ~= nil then
    if self.index == #self.pages then
      trx.inventory_ring.animate_selection(anim.frame_count - 1, 1)
    else
      trx.inventory_ring.animate_selection(0, -1)
    end
  end
  if self.crystal then
    trx.overlay.set_caption(nil)
  end
  if confirmed and (not self.standalone or self.crystal) then
    self.ctx:confirm()
  else
    self.ctx:cancel()
  end
end

-------------------------------------------------------------------------------
-- Deleting a save
--
-- The delete button keeps its space while it is hidden, so that the list above
-- it does not move.
-------------------------------------------------------------------------------

-- Includes the gap between the button and the list.
local DELETE_FOOTER_HEIGHT = 3.0 + hold_button.HEIGHT

local function is_deletable(page)
  if
    page == nil
    or page.role ~= pages.Page.LOAD_GAME
      and page.role ~= pages.Page.SAVE_GAME
  then
    return false
  end
  local row = page.req:selected_row()
  return row ~= nil
    and row.slot_num ~= nil
    and trx.savegame.info(row.slot_num, row.pool) ~= nil
end

function Passport:sync_delete()
  local page = self.page
  if page ~= nil and page.delete ~= nil then
    page.delete:set_enabled(is_deletable(page))
  end
end

-------------------------------------------------------------------------------
-- Lists
-------------------------------------------------------------------------------

local function page_anchor()
  return pages.is_title() and 0.98 or 0.67
end

-- Creates a list of fixed width that keeps room for every row it can show.
-- Every list rests on the same line, whatever its height.
local function list_req(rows, title, footer)
  local req = requester.new({
    title = title,
    rows = rows,
    min_w = PASSPORT_WIDTH,
    reserve = true,
    footer = footer,
    footer_height = footer ~= nil and DELETE_FOOTER_HEIGHT or nil,
  })
  req.place = requester.on_page_line(req, page_anchor(), DELETE_FOOTER_HEIGHT)
  return req
end

-- Creates a list as wide as its rows, with wider row padding.
local function modes_req(rows, title)
  local req = requester.new({
    title = title,
    rows = rows,
    row_pad = metrics.CHOICE_ROW_PAD,
    arrows = false,
  })
  req.place = requester.on_page_line(req, page_anchor(), DELETE_FOOTER_HEIGHT)
  return req
end

-- Puts the cursor on the save that the game last used.
local function select_recent_slot(req)
  local slot_num, pool = trx.savegame.recent_slot()
  if slot_num == nil then
    return
  end
  for index, row in ipairs(req.rows) do
    if row.slot_num == slot_num and row.pool == pool then
      req.list:select(index)
      return
    end
  end
end

-- The lists that a page opens over itself.
local List = {
  PLAY_PREV_SLOT = "play_prev_slot",
  PLAY_PREV_LEVEL = "play_prev_level",
  STORY_SLOT = "story_slot",
  SWITCH_MOD = "switch_mod",
  SELECT_LEVEL_MODE = "select_level_mode",
  CONFIRM_DELETE = "confirm_delete",
}

-- The caption of each list that names one. A list without one keeps the
-- caption of the list or page under it.
local LIST_CAPTIONS = {
  [List.PLAY_PREV_SLOT] = "general/passport/play_previous_levels",
  [List.PLAY_PREV_LEVEL] = "general/passport/play_previous_levels",
  [List.STORY_SLOT] = "general/passport/story_so_far",
  [List.SWITCH_MOD] = "general/passport/switch_mod",
  [List.SELECT_LEVEL_MODE] = "general/passport/select_level",
}

-- Shows only the top list. A message stays over the list that it is about,
-- and that list stays in view.
function Passport:sync_page()
  self.page_shown:set(#self.lists == 0)
  local key = pages.CAPTIONS[self.pages[self.index].role]
  for i, entry in ipairs(self.lists) do
    entry.shown:set(i == #self.lists)
    key = LIST_CAPTIONS[entry.role] or key
  end
  self:set_caption(key)
end

-- Pushes a list over the page. The list reads the input until it closes.
function Passport:push_list(role, req, extra)
  ---@type common.ui.passport.Sheet
  local entry = { role = role, req = req, shown = trx.signal.new(true) }
  for key, value in pairs(extra or {}) do
    entry[key] = value
  end
  entry.layer = self.ctx:push({
    root = trx.ui.widgets.Stack({
      shown = entry.shown,
      children = { req.widget },
    }),
    place = req.place,
    on_input = function(_, keys)
      self:control_list(entry, keys)
    end,
    on_close = function()
      for i, other in ipairs(self.lists) do
        if other == entry then
          table.remove(self.lists, i)
          break
        end
      end
      self:sync_page()
    end,
  })
  self.lists[#self.lists + 1] = entry
  self:sync_page()
  return entry
end

function Passport:show_message(text)
  self.message = self.ctx:push({
    root = requester.message(text),
    place = { y = page_anchor() },
    on_input = function(layer, keys)
      if
        keys:pressed(trx.input.Role.MENU_CONFIRM)
        or keys:pressed(trx.input.Role.MENU_BACK)
      then
        layer:close()
      end
    end,
    on_close = function()
      self.message = nil
      self:sync_page()
    end,
  })
  self:sync_page()
end

function Passport:push_confirm_delete(row)
  local req = requester.new({
    title = trx.locale.get("general/passport/delete_save_confirm"),
    rows = {
      {
        text = trx.locale.get("general/passport/delete_save_yes"),
        yes = true,
      },
      { text = trx.locale.get("general/passport/delete_save_no") },
    },
    arrows = false,
    place = { y = pages.is_title() and 0.69 or 0.55 },
  })
  req.list:select(2)
  self:push_list(List.CONFIRM_DELETE, req, { slot = row })
end

-- Builds the list of the page that the book rests on. Returns false when the
-- page has no list.
function Passport:open_page(role)
  self.bare = false
  self.page = nil
  ---@type common.ui.Requester
  local req
  -- A page opened from the book, or a list pushed over it.
  ---@class common.ui.passport.Sheet
  ---@field role string
  ---@field req common.ui.Requester
  ---@field shown? trx.signal.Signal
  ---@field layer? trx.ui.StackLayer
  ---@field slot? table
  ---@field delete? common.ui.HoldButton
  local page = { role = role }
  if role == pages.Page.NEW_GAME then
    local choices = rows.new_game_rows()
    -- With only one choice, the page shows no list and starts a new game on
    -- confirm.
    if #choices == 1 then
      self.bare = true
      return true
    end
    req = modes_req(choices, trx.locale.get("general/passport/select_mode"))
  elseif role == pages.Page.LOAD_GAME or role == pages.Page.SAVE_GAME then
    page.delete = hold_button.new({
      words = trx.locale.get("general/passport/delete_save"),
      align = trx.ui.HAlign.RIGHT,
    })
    req = list_req(
      rows.slot_rows(),
      trx.locale.get(pages.CAPTIONS[role]),
      page.delete.widget
    )
    select_recent_slot(req)
  elseif role == pages.Page.SELECT_LEVEL then
    req = list_req(
      rows.all_level_rows(),
      trx.locale.get("general/passport/select_level")
    )
  else
    return false
  end
  page.req = req
  self.page = page
  self:sync_delete()
  self.book:set_root(
    trx.ui.widgets.Stack({
      shown = self.page_shown,
      children = { req.widget },
    }),
    req.place
  )
  return true
end

function Passport:clear_page()
  self.page = nil
  self.opened = nil
  self.bare = false
  self.book:set_root(trx.ui.widgets.Spacer())
end

function Passport:flip(step)
  local next_index = self:neighbour(step)
  if next_index == nil then
    return false
  end
  self.index = next_index
  self:clear_page()
  -- Hides the caption before it takes the new page's name, so that no frame
  -- names a page the book has not turned to.
  self.target:set(next_index)
  self:set_caption(pages.CAPTIONS[self.pages[self.index].role])
  return true
end

-- Starts or stops browsing the pages, which TR1 does before it opens one.
function Passport:set_browse(browse)
  self.browse = browse
  if self.browsing ~= nil then
    self.browsing:set(browse)
  end
end

-- Returns whether the pages turn. TR1 turns them only while no page is open.
function Passport:can_flip()
  return self.browse or trx.game.tr_version ~= 1
end

function Passport:flip_keys(keys)
  if not self:can_flip() then
    return
  end
  if keys:pressed(trx.input.Role.MENU_LEFT) then
    self:flip(-1)
  elseif keys:pressed(trx.input.Role.MENU_RIGHT) then
    self:flip(1)
  end
end

-------------------------------------------------------------------------------
-- Choices
-------------------------------------------------------------------------------

-- Spends a crystal before the save, so that the save does not hold it. A save
-- that fails gives the crystal back.
local function save_with_crystal(row)
  local crystal = trx.catalog.objects.SAVE_CRYSTAL_ITEM
  trx.inventory.take(crystal)
  if not trx.savegame.save(row.slot_num, row.pool) then
    trx.inventory.give(crystal)
  end
end

function Passport:choose_new_game(row)
  if row.choice == "new_game" then
    trx.game.start_new_game()
  elseif row.choice == "new_game_plus" then
    trx.game.start_new_game({ ng_plus = true })
  elseif row.choice == "play_prev" then
    self:push_list(
      List.PLAY_PREV_SLOT,
      list_req(
        rows.slot_rows(function(info)
          return info ~= nil and info.can_select_level
        end),
        trx.locale.get("general/passport/select_save")
      )
    )
    return
  elseif row.choice == "story" then
    self:push_list(
      List.STORY_SLOT,
      list_req(
        rows.slot_rows(function(info)
          return info ~= nil and info.has_story
        end),
        trx.locale.get("general/passport/select_save")
      )
    )
    return
  elseif row.choice == "switch_mod" then
    local mod_rows = {}
    local current = 1
    for _, mod in ipairs(pages.switchable_mods()) do
      local key = "dynamic/mods/" .. mod.name .. "/title"
      local title = trx.locale.get(key)
      if title == key then
        title = mod.title or mod.name
      end
      mod_rows[#mod_rows + 1] = { text = title, mod = mod }
      if trx.mod.current ~= nil and mod.name == trx.mod.current.name then
        current = #mod_rows
      end
    end
    local req =
      list_req(mod_rows, trx.locale.get("general/passport/select_mod"))
    req.list:select(current)
    self:push_list(List.SWITCH_MOD, req)
    return
  end
  self:close(true)
end

function Passport:choose(entry, row)
  local role = entry.role

  if role == pages.Page.NEW_GAME then
    self:choose_new_game(row)
  elseif role == pages.Page.LOAD_GAME then
    if trx.savegame.info(row.slot_num, row.pool) == nil then
      return
    end
    trx.savegame.load(row.slot_num, row.pool)
    self:close(true)
  elseif role == pages.Page.SAVE_GAME then
    -- Quick saves are listed but are not save targets.
    if row.pool ~= trx.savegame.Pool.NORMAL then
      return
    end
    if self.crystal then
      save_with_crystal(row)
    else
      trx.savegame.save(row.slot_num, row.pool)
    end
    self:close(true)
  elseif role == pages.Page.SELECT_LEVEL then
    if pages.game_modes_available() then
      self.chosen_level = row.level.num
      self:push_list(
        List.SELECT_LEVEL_MODE,
        modes_req(
          rows.mode_rows(),
          trx.locale.get("general/passport/select_mode")
        )
      )
      return
    end
    trx.game.play_level(row.level.num, { select = true, ng_plus = false })
    self:close(true)
  elseif role == List.SELECT_LEVEL_MODE then
    local level_num = self.chosen_level
    if level_num == nil then
      return
    end
    trx.game.play_level(level_num, {
      select = true,
      ng_plus = row.choice == "new_game_plus",
    })
    self:close(true)
  elseif role == List.PLAY_PREV_SLOT then
    local info = trx.savegame.info(row.slot_num, row.pool)
    local reached = info ~= nil
      and info.can_select_level
      and trx.savegame.reached_levels(row.slot_num, row.pool)
    local level_rows = reached and rows.reached_level_rows(reached) or {}
    if #level_rows == 0 then
      self:show_message(
        trx.locale.get("general/passport/save_slot_unsupported")
      )
      return
    end
    self:push_list(
      List.PLAY_PREV_LEVEL,
      list_req(level_rows, trx.locale.get("general/passport/select_level")),
      { slot = row }
    )
  elseif role == List.PLAY_PREV_LEVEL then
    local slot = entry.slot
    trx.game.play_level(row.level.num, {
      select = true,
      from_save = { slot_num = slot.slot_num, pool = slot.pool },
    })
    self:close(true)
  elseif role == List.CONFIRM_DELETE then
    local slot = entry.slot
    entry.layer:close()
    if row.yes then
      if trx.savegame.delete(slot.slot_num, slot.pool) then
        if self.page ~= nil then
          self.page.req:set_rows(rows.slot_rows())
          self:sync_delete()
        end
      else
        self:show_message(
          trx.locale.get("general/passport/delete_save_failed")
        )
      end
    end
  elseif role == List.STORY_SLOT then
    trx.savegame.play_story(row.slot_num, row.pool)
    self:close(true)
  elseif role == List.SWITCH_MOD then
    trx.mod.switch(row.mod)
    self:close(true)
  end
end

function Passport:control_list(entry, keys)
  local picked, cancelled = entry.req.list:control(keys)
  if cancelled then
    entry.layer:close()
  elseif picked ~= nil then
    self:choose(entry, entry.req.rows[picked])
  end
end

-------------------------------------------------------------------------------
-- The page that the book rests on
-------------------------------------------------------------------------------

-- Reads the delete button. A completed hold asks for confirmation.
function Passport:control_delete(keys)
  local delete = self.page.delete
  if delete == nil or not delete:control(keys, is_deletable(self.page)) then
    return false
  end
  self:push_confirm_delete(self.page.req:selected_row())
  return true
end

-- Leaves an open page. TR1 goes back to turning the pages; the later games and
-- the quick save and load screens put the passport away, except when Lara is
-- dead.
function Passport:back_out()
  if
    not self.standalone and pages.browses_first(trx.inventory_ring.mode())
  then
    self:set_browse(true)
    self:clear_page()
  elseif pages.can_back_out() then
    self:close(false)
  end
end

-- Reads the list on the page that the book rests on. Returns true when the
-- list used the press.
function Passport:control_page(keys)
  if self:control_delete(keys) then
    return true
  end
  local picked, cancelled = self.page.req.list:control(keys)
  if cancelled then
    self:back_out()
    return true
  elseif picked ~= nil then
    self:choose(self.page, self.page.req.rows[picked])
    return true
  end
  return false
end

function Passport:control_book(keys)
  local page = self.pages[self.index]

  -- Input turns the pages during a turn only with the responsive passport
  -- setting.
  local showing = self.resting:get()
  if showing == nil then
    if trx.config.get("input.enable_responsive_passport") then
      self:flip_keys(keys)
    end
    return
  end
  if showing ~= self.index then
    turn_to(self.index)
    return
  end

  -- A press reads as pressed only once per tick, so the press that leaves
  -- browsing is kept for a flat page to act on below.
  local confirmed = false
  if self.browse then
    if keys:pressed(trx.input.Role.MENU_CONFIRM) then
      confirmed = true
      self:set_browse(false)
      -- A flat page has no list to open, so the same press acts on it below.
      if not pages.FLAT[page.role] then
        return
      end
    else
      if keys:pressed(trx.input.Role.MENU_BACK) then
        if pages.can_back_out() then
          self:close(false)
        end
      else
        self:flip_keys(keys)
      end
      return
    end
  end

  -- Builds the page once, when the book stops on it, because the page reads
  -- every save slot.
  if self.opened ~= page.role then
    self.opened = page.role
    if not pages.FLAT[page.role] and not self:open_page(page.role) then
      self:close(false)
      return
    end
    -- On the TR1 title screen, a new game page with one choice starts the
    -- game on the press that opened the page.
    if
      self.bare
      and pages.browses_first(trx.inventory_ring.mode())
      and pages.is_title()
    then
      trx.game.start_new_game()
      self:close(true)
      return
    end
  end

  if pages.FLAT[page.role] or self.bare then
    if confirmed or keys:pressed(trx.input.Role.MENU_CONFIRM) then
      if self.bare then
        trx.game.start_new_game()
      elseif page.role == pages.Page.RESTART_LEVEL then
        trx.game.restart_level()
      elseif page.role == pages.Page.EXIT_TO_TITLE then
        trx.game.exit_to_title()
      else
        trx.game.exit_game()
      end
      self:close(true)
      return
    end
    if self.bare and keys:pressed(trx.input.Role.MENU_BACK) then
      self:back_out()
      return
    end
  elseif self.page ~= nil and self:control_page(keys) then
    return
  end

  if keys:pressed(trx.input.Role.MENU_BACK) then
    if pages.can_back_out() then
      self:close(false)
    end
  else
    self:flip_keys(keys)
  end
end

-------------------------------------------------------------------------------
-- Opening the book
-------------------------------------------------------------------------------

local function new(ctx, fields)
  ---@type common.ui.passport.Passport
  local self = setmetatable({
    ctx = ctx,
    lists = {},
    page_shown = trx.signal.new(true),
    caption = trx.signal.new(""),
  }, Passport)
  for key, value in pairs(fields) do
    self[key] = value
  end
  return self
end

-- Opens the book on the page at `index`, in the inventory ring opened in
-- `mode`.
function M.open(ctx, book_pages, index, mode)
  local self = new(ctx, {
    pages = book_pages,
    index = index,
    browse = pages.browses_first(mode),
  })
  self:set_caption(pages.CAPTIONS[book_pages[index].role])

  -- The caption names the page that the book rests on, and hides while the
  -- book turns. Its arrows show where the book turns to, while no list is
  -- open over the page.
  self.target = trx.signal.new(index)
  self.browsing = trx.signal.new(self.browse)
  self.resting = trx.inventory_ring.signals.rest_frame:map(page_at)
  self.caption_shown = trx.signal.combine(
    self.resting,
    self.target,
    function(resting, target)
      return resting == target
    end
  )
  local function arrow(step)
    return trx.signal.combine(
      self.caption_shown,
      self.target,
      self.browsing,
      self.page_shown,
      function(shown, _, _, alone)
        return shown
          and alone
          and self:can_flip()
          and self:neighbour(step) ~= nil
      end
    )
  end
  self.arrow_left = arrow(-1)
  self.arrow_right = arrow(1)

  self.book = ctx:push({
    root = trx.ui.widgets.Spacer(),
    on_input = function(_, keys)
      self:control_book(keys)
    end,
  })
  -- The book names its own pages, so the name the ring gave the entry comes
  -- down.
  trx.overlay.set_caption(nil)
  -- Unlit arrows keep their room, so the caption does not move as they come
  -- and go.
  local function arrow_label(glyph, lit)
    return trx.ui.widgets.Label({ text = glyph, hidden = ~lit })
  end
  local caption = trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    v_align = trx.ui.VAlign.CENTER,
    spacing = CAPTION_ARROW_GAP,
    shown = self.caption_shown,
    children = {
      arrow_label("\\{button left}", self.arrow_left),
      trx.ui.widgets.Label({ text = self.caption }),
      arrow_label("\\{button right}", self.arrow_right),
    },
  })
  trx.ui.regions.place(trx.ui.Region.BOTTOM_CENTER, caption)
  ctx:on_end(function()
    caption:release()
  end)
  return self.book
end

-- Holds one page alone, with no ring and no book behind it. Returns nil when
-- the page has no list.
function M.open_alone(ctx, book_pages, index, crystal)
  local self = new(ctx, {
    pages = book_pages,
    index = index,
    standalone = true,
    crystal = crystal,
  })
  self.book = ctx:push({
    root = trx.ui.widgets.Spacer(),
    on_input = function(_, keys)
      self:control_page(keys)
    end,
  })
  -- A new game page with one choice has no list to show, so the engine
  -- keeps the screen.
  if not self:open_page(book_pages[index].role) or self.bare then
    return nil
  end
  return self.book
end

return M
