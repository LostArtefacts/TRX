-- Draws the passport entry of the inventory ring: its pages, the page turns,
-- and every list that the pages open. The page that the book rests on is one
-- layer, and each list opened from it is a layer above it. A choice that
-- leaves the passport runs as a game flow command once the ring has closed.
--
--   require("common.passport").setup()

local requester = require("common.ui.requester")

local M = {}

-- Each page sits five frames past the one the book opens on.
local FRAMES_PER_PAGE = 5

local Page = {
  LOAD_GAME = "load_game",
  SAVE_GAME = "save_game",
  NEW_GAME = "new_game",
  SELECT_LEVEL = "select_level",
  RESTART_LEVEL = "restart_level",
  EXIT_TO_TITLE = "exit_to_title",
  EXIT_GAME = "exit_game",
}

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

local CAPTIONS = {
  [Page.LOAD_GAME] = "general/passport/load_game",
  [Page.SAVE_GAME] = "general/passport/save_game",
  [Page.NEW_GAME] = "general/passport/new_game",
  [Page.SELECT_LEVEL] = "general/passport/select_level",
  [Page.RESTART_LEVEL] = "general/passport/restart_level",
  [Page.EXIT_TO_TITLE] = "general/passport/exit_to_title",
  [Page.EXIT_GAME] = "general/passport/exit_game",
}

-- Pages that act on confirm and open no list.
local FLAT = {
  [Page.RESTART_LEVEL] = true,
  [Page.EXIT_TO_TITLE] = true,
  [Page.EXIT_GAME] = true,
}

local caption = trx.signal.new("")
local caption_shown = trx.signal.new(false)
local arrow_left = trx.signal.new(false)
local arrow_right = trx.signal.new(false)

local state = {}

-------------------------------------------------------------------------------
-- What the game offers
-------------------------------------------------------------------------------

local function is_in_gym()
  local level = trx.game.current_level
  return level ~= nil and level.type == trx.game.LevelType.GYM
end

-- Returns false when Lara is dead, because the passport is then the only way
-- on.
local function can_back_out()
  return trx.inventory_ring.mode() ~= trx.inventory_ring.Mode.DEATH
end

local function is_tr1()
  return trx.game.tr_version == 1
end

-- Returns whether a page opens only after the player confirms it. TR1 does
-- this; the later games open a page as soon as the book stops on it.
local function browses_first(mode)
  return is_tr1()
    and (
      mode == trx.inventory_ring.Mode.TITLE
      or mode == trx.inventory_ring.Mode.GAME
      or mode == trx.inventory_ring.Mode.DEATH
    )
end

local function is_title()
  local level = trx.game.current_level
  return level == nil or level.type == trx.game.LevelType.TITLE
end

local function game_modes_available()
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
local function each_slot(fn)
  for _, pool in ipairs({ trx.savegame.Pool.QUICK, trx.savegame.Pool.NORMAL }) do
    for slot_num = 1, trx.savegame.slot_count(pool) do
      fn(slot_num, pool)
    end
  end
end

local function has_saves()
  local found = false
  each_slot(function(slot_num, pool)
    found = found or not trx.savegame.is_free(slot_num, pool)
  end)
  return found
end

local function switchable_mods()
  local mods = {}
  for _, mod in ipairs(trx.mod.list) do
    if trx.mod.can_switch(mod) then
      mods[#mods + 1] = mod
    end
  end
  return mods
end

local function saving_enabled()
  return trx.savegame.slot_count(trx.savegame.Pool.NORMAL) > 0
    and not trx.config.get("flow.load_save_disabled")
end

-------------------------------------------------------------------------------
-- The pages
--
-- The book has three pages. A page with nothing on it stays empty, so that
-- every page keeps its place in the book.
-------------------------------------------------------------------------------

local function first_available(pages)
  for i, page in ipairs(pages) do
    if page.available then
      return i
    end
  end
  return nil
end

-- Puts an exit on the last page when no page is available, so that the player
-- always has a way out.
local function ensure_way_out(pages, mode)
  if first_available(pages) ~= nil then
    return
  end
  pages[3] = {
    role = mode == trx.inventory_ring.Mode.TITLE and Page.EXIT_GAME
      or Page.EXIT_TO_TITLE,
    available = true,
  }
end

local function determine_pages(mode)
  local pages = { {}, {}, {} }
  local function set(slot, role, available)
    pages[slot] = { role = role, available = available }
  end

  local saves = has_saves() and saving_enabled()
  local can_restart = trx.game.can_restart_level()

  if mode == trx.inventory_ring.Mode.TITLE then
    set(1, Page.LOAD_GAME, saves)
    set(2, Page.NEW_GAME, true)
    set(3, Page.EXIT_GAME, true)
  elseif mode == trx.inventory_ring.Mode.GAME then
    if not saving_enabled() then
      set(2, Page.RESTART_LEVEL, can_restart)
    else
      set(1, Page.LOAD_GAME, saves)
      set(2, Page.SAVE_GAME, true)
    end
    set(3, Page.EXIT_TO_TITLE, true)
  elseif mode == trx.inventory_ring.Mode.LOAD then
    if not saving_enabled() then
      set(2, Page.RESTART_LEVEL, can_restart)
    elseif saves then
      set(1, Page.LOAD_GAME, true)
    else
      set(2, Page.SAVE_GAME, true)
    end
  elseif
    mode == trx.inventory_ring.Mode.SAVE
    or mode == trx.inventory_ring.Mode.SAVE_CRYSTAL
  then
    if not saving_enabled() then
      set(2, Page.RESTART_LEVEL, can_restart)
    else
      set(2, Page.SAVE_GAME, true)
    end
  elseif mode == trx.inventory_ring.Mode.DEATH then
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
        and mode ~= trx.inventory_ring.Mode.SAVE_CRYSTAL
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

-- Returns the next available page in the given direction, or nil.
local function neighbour(step)
  local i = state.index + step
  while i >= 1 and i <= #state.pages do
    if state.pages[i].available then
      return i
    end
    i = i + step
  end
  return nil
end

-------------------------------------------------------------------------------
-- The book
-------------------------------------------------------------------------------

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

-- Returns the page that the book rests on, or nil while the book turns.
local function page_on_show()
  local anim = trx.inventory_ring.selection_anim()
  if anim == nil then
    return nil
  end
  local offset = anim.frame - anim.open_frame
  if offset % FRAMES_PER_PAGE ~= 0 or anim.frame ~= anim.goal_frame then
    return nil
  end
  return offset // FRAMES_PER_PAGE + 1
end

local function set_caption(key)
  caption:set(
    trx.locale.format(
      "general/inventory_ring/object_name_fmt",
      trx.locale.get(key)
    )
  )
end

-- Shuts the book towards the nearer cover. A confirmed close leaves the ring;
-- otherwise the entry goes back into the ring. The quick save and load screens
-- have no book to shut, and take only a cancel.
local function close(confirmed)
  local anim = not state.standalone and trx.inventory_ring.selection_anim()
    or nil
  if anim ~= nil then
    if state.index == #state.pages then
      trx.inventory_ring.animate_selection(anim.frame_count - 1, 1)
    else
      trx.inventory_ring.animate_selection(0, -1)
    end
  end
  if confirmed and not state.standalone then
    state.ctx:confirm()
  else
    state.ctx:cancel()
  end
end

-------------------------------------------------------------------------------
-- Deleting a save
--
-- The delete button must be held, so that one stray key press cannot delete a
-- save. The button keeps its space while it is hidden, so that the list above
-- it does not move.
-------------------------------------------------------------------------------

local DELETE_TEXT_SCALE = 0.85
local DELETE_PAD_X = 6.0
local DELETE_PAD_Y = 3.0
local DELETE_SPACING = 2.0
local DELETE_BAR_HEIGHT = 4.0
local DELETE_HOLD_DEBUFF = 10
local DELETE_HOLD_MAX = 30

-- In canvas units at the default text size.
local DELETE_HEIGHT = 2.0 * DELETE_PAD_Y
  + 15.0 * DELETE_TEXT_SCALE
  + DELETE_SPACING
  + DELETE_BAR_HEIGHT

-- Includes the gap between the button and the list.
local DELETE_FOOTER_HEIGHT = 3.0 + DELETE_HEIGHT

local function delete_label()
  local key = trx.input.key_name(trx.input.Role.UNBIND_KEY)
  if key == nil then
    return nil
  end
  return string.format(
    "%s: %s",
    trx.locale.get("general/passport/delete_save"),
    trx.locale.format("general/misc/hold_fmt", key)
  )
end

local function is_deletable(page)
  if
    page == nil
    or page.role ~= Page.LOAD_GAME and page.role ~= Page.SAVE_GAME
  then
    return false
  end
  local row = page.req.selected_row()
  return row ~= nil
    and row.slot_num ~= nil
    and trx.savegame.info(row.slot_num, row.pool) ~= nil
end

local function sync_delete(page)
  if page == nil or page.delete_text == nil then
    return
  end
  local text = delete_label()
  page.delete_text:set(text or "")
  page.delete_shown:set(text ~= nil and is_deletable(page))
  page.delete_progress:set(
    (state.delete_hold - DELETE_HOLD_DEBUFF) / DELETE_HOLD_MAX
  )
end

-- Builds the button: its text over a sleek bar, against the right edge of the
-- list.
local function delete_button(page)
  page.delete_text = trx.signal.new(delete_label() or "")
  page.delete_shown = trx.signal.new(false)
  page.delete_progress = trx.signal.new(-1)
  local bar = trx.ui.widgets.SleekBar({ progress = page.delete_progress })
  bar.hidden = page.delete_progress:map(function(progress)
    return progress < 0
  end)
  return trx.ui.widgets.Stack({
    align = trx.ui.HAlign.RIGHT,
    children = {
      trx.ui.widgets.Pad({
        x = DELETE_PAD_X,
        y = DELETE_PAD_Y,
        hidden = ~page.delete_shown,
        child = trx.ui.widgets.Stack({
          spacing = DELETE_SPACING,
          align = trx.ui.HAlign.SPAN,
          children = {
            trx.ui.widgets.Label({
              text = page.delete_text,
              scale = DELETE_TEXT_SCALE,
            }),
            bar,
          },
        }),
      }),
    },
  })
end

-------------------------------------------------------------------------------
-- Rows
-------------------------------------------------------------------------------

local PASSPORT_WIDTH = 300.0

-- Lists every quick save and every numbered slot, empty or not. TR1 centers
-- the save number after the level name; the later games set it against the
-- right edge.
local function slot_rows(filter)
  local rows = {}
  each_slot(function(slot_num, pool)
    local info = trx.savegame.info(slot_num, pool)
    if filter ~= nil and not filter(info) then
      return
    end
    local row = { slot_num = slot_num, pool = pool }
    if info == nil then
      row.text = trx.locale.format("general/misc/empty_slot_fmt", slot_num)
    elseif info.counter <= 0 then
      row.text = info.level_title
    elseif is_tr1() then
      row.text = info.level_title .. " " .. info.counter
      if info.is_quick then
        row.text = row.text .. " (QS)"
      end
    else
      row.text = info.level_title
      row.right = info.is_quick and ("QS " .. info.counter)
        or tostring(info.counter)
    end
    rows[#rows + 1] = row
  end)
  return rows
end

-- Lists every level except the gym and the placeholder entries that only old
-- saves refer to.
local function all_level_rows()
  local rows = {}
  for _, level in ipairs(trx.game.levels) do
    if
      level.type ~= trx.game.LevelType.GYM
      and level.type ~= trx.game.LevelType.DUMMY
      and level.type ~= trx.game.LevelType.CURRENT
    then
      rows[#rows + 1] = { text = level.title, level = level }
    end
  end
  return rows
end

local function reached_level_rows(reached_nums)
  local reached = {}
  for _, num in ipairs(reached_nums) do
    reached[num] = true
  end
  local rows = {}
  for _, level in ipairs(trx.game.levels) do
    if level.type ~= trx.game.LevelType.GYM and reached[level.num] then
      rows[#rows + 1] = { text = level.title, level = level }
    end
  end
  return rows
end

local function mode_rows()
  local rows = {
    {
      text = trx.locale.get("general/passport/mode_new_game"),
      choice = "new_game",
    },
  }
  if game_modes_available() then
    rows[#rows + 1] = {
      text = trx.locale.get("general/passport/mode_new_game_plus"),
      choice = "new_game_plus",
    }
  end
  return rows
end

local function new_game_rows()
  local rows = mode_rows()

  -- One setting turns both of the save choices on or off.
  local play_prev = false
  local story = false
  if
    not trx.config.get("flow.load_save_disabled")
    and trx.config.get("gameplay.enable_play_previous_levels")
  then
    each_slot(function(slot_num, pool)
      local info = trx.savegame.info(slot_num, pool)
      if info == nil then
        return
      end
      play_prev = play_prev or info.can_select_level
      story = story or info.has_story
    end)
  end

  -- A rule separates these choices from the game modes.
  local extras = {}
  if play_prev then
    extras[#extras + 1] = {
      text = trx.locale.get("general/passport/play_previous_levels"),
      choice = "play_prev",
    }
  end
  if story then
    extras[#extras + 1] = {
      text = trx.locale.get("general/passport/story_so_far"),
      choice = "story",
    }
  end
  if #switchable_mods() > 1 then
    extras[#extras + 1] = {
      text = trx.locale.get("general/passport/switch_mod"),
      choice = "switch_mod",
    }
  end
  for index, row in ipairs(extras) do
    row.rule = index == 1
    rows[#rows + 1] = row
  end
  return rows
end

-------------------------------------------------------------------------------
-- Lists
-------------------------------------------------------------------------------

local function page_anchor()
  return is_title() and 0.98 or 0.67
end

-- Creates a list of fixed width that keeps room for every row it can show.
-- Every list rests on the same line, whatever its height.
local function list_req(rows, title, footer)
  local req = requester.new({
    title = title,
    rows = rows,
    width = PASSPORT_WIDTH,
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
    row_pad = 20.0,
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

-- The caption of each list that names one. A list without one keeps the
-- caption of the list or page under it.
local LIST_CAPTIONS = {
  play_prev_slot = "general/passport/play_previous_levels",
  play_prev_level = "general/passport/play_previous_levels",
  story_slot = "general/passport/story_so_far",
  switch_mod = "general/passport/switch_mod",
  select_level_mode = "general/passport/select_level",
}

local choose
local control_list

-- Shows only the top list. A message stays over the list that it is about,
-- and that list stays in view.
local function sync_page()
  state.page_shown:set(#state.lists == 0)
  local key = CAPTIONS[state.pages[state.index].role]
  for i, entry in ipairs(state.lists) do
    entry.shown:set(i == #state.lists)
    key = LIST_CAPTIONS[entry.role] or key
  end
  set_caption(key)
end

-- Pushes a list over the page. The list reads the input until it closes.
local function push_list(role, req, extra)
  local entry = { role = role, req = req, shown = trx.signal.new(true) }
  for key, value in pairs(extra or {}) do
    entry[key] = value
  end
  entry.layer = state.ctx:push({
    root = trx.ui.widgets.Stack({
      shown = entry.shown,
      children = { req.root },
    }),
    place = req.place,
    on_input = function(_, keys)
      control_list(entry, keys)
    end,
    on_close = function()
      for i, other in ipairs(state.lists) do
        if other == entry then
          table.remove(state.lists, i)
          break
        end
      end
      sync_page()
    end,
  })
  state.lists[#state.lists + 1] = entry
  arrow_left:set(false)
  arrow_right:set(false)
  sync_page()
  return entry
end

local function show_message(text)
  state.message = state.ctx:push({
    root = requester.message(text),
    place = requester.anchored(page_anchor()),
    on_input = function(layer, keys)
      if
        keys:pressed(trx.input.Role.MENU_CONFIRM)
        or keys:pressed(trx.input.Role.MENU_BACK)
      then
        layer:close()
      end
    end,
    on_close = function()
      state.message = nil
      sync_page()
    end,
  })
  sync_page()
end

local function push_confirm_delete(row)
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
  })
  req.list:select(2)
  req.place = requester.anchored(is_title() and 0.69 or 0.55)
  push_list("confirm_delete", req, { slot = row })
end

-- Builds the list of the page that the book rests on. Returns false when the
-- page has no list.
local function open_page(role)
  state.bare = false
  state.page = nil
  local req
  local page = { role = role }
  if role == Page.NEW_GAME then
    local rows = new_game_rows()
    -- With only one choice, the page shows no list and starts a new game on
    -- confirm.
    if #rows == 1 then
      state.bare = true
      return true
    end
    req = modes_req(rows, trx.locale.get("general/passport/select_mode"))
  elseif role == Page.LOAD_GAME or role == Page.SAVE_GAME then
    req = list_req(
      slot_rows(),
      trx.locale.get(CAPTIONS[role]),
      delete_button(page)
    )
    select_recent_slot(req)
  elseif role == Page.SELECT_LEVEL then
    req = list_req(
      all_level_rows(),
      trx.locale.get("general/passport/select_level")
    )
  else
    return false
  end
  page.req = req
  state.page = page
  sync_delete(page)
  state.place = req.place
  state.book:set_root(trx.ui.widgets.Stack({
    shown = state.page_shown,
    children = { req.root },
  }))
  return true
end

local function clear_page()
  state.page = nil
  state.opened = nil
  state.bare = false
  state.book:set_root(trx.ui.widgets.Custom({
    measure = function()
      return 0, 0
    end,
    paint = function() end,
  }))
end

local function flip(step)
  local next_index = neighbour(step)
  if next_index == nil then
    return false
  end
  state.index = next_index
  clear_page()
  -- Hides the caption before it takes the new page's name, so that no frame
  -- names a page the book has not turned to.
  caption_shown:set(false)
  arrow_left:set(false)
  arrow_right:set(false)
  set_caption(CAPTIONS[state.pages[state.index].role])
  return true
end

-- Returns whether the pages turn. TR1 turns them only while no page is open.
local function can_flip()
  return state.browse or not is_tr1()
end

local function flip_keys(keys)
  if not can_flip() then
    return
  end
  if keys:pressed(trx.input.Role.MENU_LEFT) then
    flip(-1)
  elseif keys:pressed(trx.input.Role.MENU_RIGHT) then
    flip(1)
  end
end

-------------------------------------------------------------------------------
-- Choices
-------------------------------------------------------------------------------

local function choose_new_game(row)
  if row.choice == "new_game" then
    trx.game.start_new_game()
  elseif row.choice == "new_game_plus" then
    trx.game.start_new_game({ ng_plus = true })
  elseif row.choice == "play_prev" then
    push_list(
      "play_prev_slot",
      list_req(
        slot_rows(function(info)
          return info ~= nil and info.can_select_level
        end),
        trx.locale.get("general/passport/select_save")
      )
    )
    return
  elseif row.choice == "story" then
    push_list(
      "story_slot",
      list_req(
        slot_rows(function(info)
          return info ~= nil and info.has_story
        end),
        trx.locale.get("general/passport/select_save")
      )
    )
    return
  elseif row.choice == "switch_mod" then
    local rows = {}
    local current = 1
    for _, mod in ipairs(switchable_mods()) do
      local key = "dynamic/mods/" .. mod.name .. "/title"
      local title = trx.locale.get(key)
      if title == key then
        title = mod.title or mod.name
      end
      rows[#rows + 1] = { text = title, mod = mod }
      if trx.mod.current ~= nil and mod.name == trx.mod.current.name then
        current = #rows
      end
    end
    local req = list_req(rows, trx.locale.get("general/passport/select_mod"))
    req.list:select(current)
    push_list("switch_mod", req)
    return
  end
  close(true)
end

choose = function(entry, row)
  local role = entry.role

  if role == Page.NEW_GAME then
    choose_new_game(row)
  elseif role == Page.LOAD_GAME then
    if trx.savegame.info(row.slot_num, row.pool) == nil then
      return
    end
    trx.savegame.load(row.slot_num, row.pool)
    close(true)
  elseif role == Page.SAVE_GAME then
    -- Quick saves are listed but are not save targets.
    if row.pool ~= trx.savegame.Pool.NORMAL then
      return
    end
    trx.savegame.save(row.slot_num, row.pool)
    close(true)
  elseif role == Page.SELECT_LEVEL then
    if game_modes_available() then
      state.chosen_level = row.level.num
      push_list(
        "select_level_mode",
        modes_req(mode_rows(), trx.locale.get("general/passport/select_mode"))
      )
      return
    end
    trx.game.play_level(row.level.num, { select = true, ng_plus = false })
    close(true)
  elseif role == "select_level_mode" then
    trx.game.play_level(state.chosen_level, {
      select = true,
      ng_plus = row.choice == "new_game_plus",
    })
    close(true)
  elseif role == "play_prev_slot" then
    local info = trx.savegame.info(row.slot_num, row.pool)
    local reached = info ~= nil
      and info.can_select_level
      and trx.savegame.reached_levels(row.slot_num, row.pool)
    local rows = reached and reached_level_rows(reached) or {}
    if #rows == 0 then
      show_message(trx.locale.get("general/passport/save_slot_unsupported"))
      return
    end
    push_list(
      "play_prev_level",
      list_req(rows, trx.locale.get("general/passport/select_level")),
      { slot = row }
    )
  elseif role == "play_prev_level" then
    local slot = entry.slot
    trx.game.play_level(row.level.num, {
      select = true,
      from_save = { slot_num = slot.slot_num, pool = slot.pool },
    })
    close(true)
  elseif role == "confirm_delete" then
    local slot = entry.slot
    entry.layer:close()
    if row.yes then
      if trx.savegame.delete(slot.slot_num, slot.pool) then
        if state.page ~= nil then
          state.page.req:set_rows(slot_rows())
          sync_delete(state.page)
        end
      else
        show_message(trx.locale.get("general/passport/delete_save_failed"))
      end
    end
  elseif role == "story_slot" then
    trx.savegame.play_story(row.slot_num, row.pool)
    close(true)
  elseif role == "switch_mod" then
    trx.mod.switch(row.mod)
    close(true)
  end
end

control_list = function(entry, keys)
  local picked = entry.req:control(keys)
  if picked == "cancel" then
    entry.layer:close()
  elseif picked ~= nil then
    choose(entry, entry.req.rows[picked])
  end
end

-------------------------------------------------------------------------------
-- The page that the book rests on
-------------------------------------------------------------------------------

-- Counts how long the delete button is held. After a hold asks for
-- confirmation, the count waits until the button is released.
local function control_delete(keys)
  if not is_deletable(state.page) then
    state.delete_hold = 0
    sync_delete(state.page)
    return false
  end
  local held = keys:held_for(trx.input.Role.UNBIND_KEY)
  if held == 0 then
    state.delete_armed = true
  end
  state.delete_hold = state.delete_armed and held or 0
  if state.delete_hold - DELETE_HOLD_DEBUFF > DELETE_HOLD_MAX then
    state.delete_armed = false
    state.delete_hold = 0
    sync_delete(state.page)
    push_confirm_delete(state.page.req.selected_row())
    return true
  end
  sync_delete(state.page)
  return false
end

-- Leaves an open page. TR1 goes back to turning the pages; the later games and
-- the quick save and load screens put the passport away, except when Lara is
-- dead.
local function back_out()
  if not state.standalone and browses_first(trx.inventory_ring.mode()) then
    state.browse = true
    clear_page()
  elseif can_back_out() then
    close(false)
  end
end

-- Reads the list on the page that the book rests on. Returns true when the
-- list used the press.
local function control_page(keys)
  if control_delete(keys) then
    return true
  end
  local picked = state.page.req:control(keys)
  if picked == "cancel" then
    back_out()
    return true
  elseif picked ~= nil then
    choose(state.page, state.page.req.rows[picked])
    return true
  end
  return false
end

local function control_book(keys)
  local page = state.pages[state.index]

  -- The caption and its arrows hide while the book turns. Input turns the
  -- pages during a turn only with the responsive passport setting.
  local showing = page_on_show()
  if showing ~= state.index then
    caption_shown:set(false)
    arrow_left:set(false)
    arrow_right:set(false)
  end
  if showing == nil then
    if trx.config.get("input.enable_responsive_passport") then
      flip_keys(keys)
    end
    return
  end
  if showing ~= state.index then
    turn_to(state.index)
    return
  end

  caption_shown:set(true)
  arrow_left:set(can_flip() and neighbour(-1) ~= nil)
  arrow_right:set(can_flip() and neighbour(1) ~= nil)

  -- A press reads as pressed only once per tick, so the press that leaves
  -- browsing is kept for a flat page to act on below.
  local confirmed = false
  if state.browse then
    if keys:pressed(trx.input.Role.MENU_CONFIRM) then
      confirmed = true
      state.browse = false
      -- A flat page has no list to open, so the same press acts on it below.
      if not FLAT[page.role] then
        return
      end
    else
      if keys:pressed(trx.input.Role.MENU_BACK) then
        if can_back_out() then
          close(false)
        end
      else
        flip_keys(keys)
      end
      return
    end
  end

  -- Builds the page once, when the book stops on it, because the page reads
  -- every save slot.
  if state.opened ~= page.role then
    state.opened = page.role
    if not FLAT[page.role] and not open_page(page.role) then
      close(false)
      return
    end
    -- On the TR1 title screen, a new game page with one choice starts the
    -- game on the press that opened the page.
    if
      state.bare
      and browses_first(trx.inventory_ring.mode())
      and is_title()
    then
      trx.game.start_new_game()
      close(true)
      return
    end
  end

  if FLAT[page.role] or state.bare then
    if confirmed or keys:pressed(trx.input.Role.MENU_CONFIRM) then
      if state.bare then
        trx.game.start_new_game()
      elseif page.role == Page.RESTART_LEVEL then
        trx.game.restart_level()
      elseif page.role == Page.EXIT_TO_TITLE then
        trx.game.exit_to_title()
      else
        trx.game.exit_game()
      end
      close(true)
      return
    end
    if state.bare and keys:pressed(trx.input.Role.MENU_BACK) then
      back_out()
      return
    end
  elseif state.page ~= nil and control_page(keys) then
    return
  end

  if keys:pressed(trx.input.Role.MENU_BACK) then
    if can_back_out() then
      close(false)
    end
  else
    flip_keys(keys)
  end
end

-------------------------------------------------------------------------------
-- Opening the book
-------------------------------------------------------------------------------

local function open(ctx)
  local mode = trx.inventory_ring.mode()
  local pages = determine_pages(mode)
  local index = first_available(pages)
  if index == nil then
    return nil
  end

  state = {
    ctx = ctx,
    pages = pages,
    index = index,
    browse = browses_first(mode),
    lists = {},
    page_shown = trx.signal.new(true),
    delete_hold = 0,
    delete_armed = true,
  }
  set_caption(CAPTIONS[pages[index].role])
  arrow_left:set(false)
  arrow_right:set(false)

  state.book = ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return 0, 0
      end,
      paint = function() end,
    }),
    -- The book's layer stays while its page changes, and each page sets its
    -- own position.
    place = function(w, h)
      if state.place ~= nil then
        return state.place(w, h)
      end
      return requester.anchored(0.5)(w, h)
    end,
    on_input = function(_, keys)
      control_book(keys)
    end,
    on_close = function()
      caption_shown:set(false)
      arrow_left:set(false)
      arrow_right:set(false)
    end,
  })
  ctx:push({
    root = trx.ui.widgets.Row({
      left = arrow_left,
      right = arrow_right,
      shown = caption_shown,
      child = trx.ui.widgets.Label({ text = caption }),
    }),
    region = trx.ui.Region.BOTTOM_CENTER,
    modal = false,
  })
  return state.book
end

-- The quick save and load screens hold one page alone, with no ring and no
-- book behind it. A page with no list leaves the screen to the engine.
local function open_save_load(ctx)
  local pages = determine_pages(ctx.mode)
  local index = first_available(pages)
  if index == nil or FLAT[pages[index].role] then
    return nil
  end

  state = {
    ctx = ctx,
    pages = pages,
    index = index,
    standalone = true,
    lists = {},
    page_shown = trx.signal.new(true),
    delete_hold = 0,
    delete_armed = true,
  }
  state.book = ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return 0, 0
      end,
      paint = function() end,
    }),
    place = function(w, h)
      return state.place(w, h)
    end,
    on_input = function(_, keys)
      control_page(keys)
    end,
  })
  if not open_page(pages[index].role) then
    return nil
  end
  return state.book
end

function M.setup()
  trx.ui.screens.define(
    trx.ui.Screen.RING_ENTRY,
    open,
    { object = trx.catalog.objects.PASSPORT_OPTION }
  )
  trx.ui.screens.define(trx.ui.Screen.SAVE_LOAD, open_save_load)
end

return M
