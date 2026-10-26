-- The list of save slots that the original TR4 shows to save or to load a
-- game: a heading at the top, and a line for each slot with the number of the
-- save and the level it was made in. The quick saves come first, as the
-- passport lists them, and can be loaded but not saved over. A save can be
-- deleted as in the passport, with the button held under the list.

local accent = require("common.ui.accent")
local arrows = require("common.ui.flat_menu.arrows")
local delete_save = require("common.ui.delete_save")
local pages = require("common.ui.screens.passport.pages")
local requester = require("common.ui.widgets.requester")
local sound = require("common.ui.flat_menu.sound").play
local scroll = require("common.ui.scroll")
local TEXT_SCALE = require("common.ui.flat_menu.text").SCALE

local M = {}

trx.locale.declare({
  ["general/flat_menu/empty_slot"] = "Empty Slot",
})

local Role = trx.input.Role

-- The room between two rows of the list, in canvas units.
local ROW_GAP = 2
-- Where the number and the name of a save start, across the original's 4:3
-- screen of 640 pixels, from its middle.
local NUMBER_X = -310
local NAME_X = -270
-- Where the time of a save starts, from the middle of the screen, and how it
-- is written: the same in every language, as the format cannot reorder its
-- parts.
local DATE_X = 135
local DATE_FMT = "%04d-%02d-%02d %02d:%02d"

---@class common.ui.flat_menu.saves.Slot
---@field slot_num integer
---@field pool integer

---@class common.ui.flat_menu.saves.Pick
---@field heading string|trx.locale.Text
---@field filter fun(info: trx.savegame.SlotInfo?): boolean Says whether the list shows a save.
---@field on_pick fun(slot: common.ui.flat_menu.saves.Slot): boolean Runs with the save the player picks, and says whether the list closes.
---@field on_back fun() Runs once the player backs out of the list.

-- The slot the list was left on, which every list opens on again, as in the
-- original.
---@type common.ui.flat_menu.saves.Slot?
local left_on = nil

---@enum common.ui.flat_menu.saves.Kind
M.Kind = {
  SAVE = "save",
  LOAD = "load",
  PICK = "pick",
}

local function colored(role, text)
  return accent.text(text, role)
end

-- The x of a point the original gives across its 640-pixel screen.
local function across(x)
  local h = trx.ui.canvas.height
  return trx.ui.canvas.width / 2 + x * h * 4 / 3 / 640
end

-- Returns every slot the list shows: the quick saves, then the numbered
-- slots, or only those that pass `filter`.
---@param filter? fun(info: trx.savegame.SlotInfo?): boolean
---@return common.ui.flat_menu.saves.Slot[]
local function all_slots(filter)
  ---@type common.ui.flat_menu.saves.Slot[]
  local slots = {}
  pages.each_slot(function(slot_num, pool)
    if filter == nil or filter(trx.savegame.info(slot_num, pool)) then
      slots[#slots + 1] = { slot_num = slot_num, pool = pool }
    end
  end)
  return slots
end

-- Returns the number of a save, the level it was made in, and when it was
-- made, or no number and the text that says the slot is empty.
local function slot_text(slot)
  local info = trx.savegame.info(slot.slot_num, slot.pool)
  if info == nil then
    return nil, trx.locale.text("general/flat_menu/empty_slot"), nil
  end
  local date = nil
  local at = info.saved_at
  if at ~= nil then
    date = DATE_FMT:format(at.year, at.month, at.day, at.hour, at.minute)
  end
  local number = ("%03d"):format(math.max(info.counter, 0))
  if info.is_quick then
    number = "QS " .. number
  end
  return number, info.level_title or "", date
end

---@param ctx trx.ui.ScreenContext
---@param kind common.ui.flat_menu.saves.Kind
---@param pick? common.ui.flat_menu.saves.Pick
---@param on_done? fun(done: boolean)
---@param stays? boolean
---@return trx.ui.StackLayer
local function open(ctx, kind, pick, on_done, stays)
  local slots = all_slots(pick and pick.filter)
  local count = #slots
  -- A save cannot go into a quick save slot, so a save opens on the first
  -- numbered slot unless it was left on another.
  local function can_take(slot)
    return kind ~= M.Kind.SAVE or slot.pool == trx.savegame.Pool.NORMAL
  end
  local selected = nil
  for i, slot in ipairs(slots) do
    if
      left_on ~= nil
      and slot.slot_num == left_on.slot_num
      and slot.pool == left_on.pool
      and can_take(slot)
    then
      selected = i
    end
  end
  if selected == nil then
    for i, slot in ipairs(slots) do
      if selected == nil and can_take(slot) then
        selected = i
      end
    end
  end
  ---@type { selected: integer, first: number, rows: table[], is_asking: boolean, is_loaded: boolean }
  local state = {
    selected = selected or 1,
    first = 1,
    rows = {},
    is_asking = false,
    is_loaded = false,
  }
  local button = delete_save.button()

  local heading = colored(
    accent.Role.MENU_HEADING,
    pick ~= nil and pick.heading
      or kind == M.Kind.SAVE and trx.locale.text("general/passport/save_game")
      or trx.locale.text("general/passport/load_game")
  )

  -- The height of a line of text, which spaces the heading and the rows.
  local function line()
    local _, height = trx.ui.primitive.measure_text("Ag", TEXT_SCALE)
    return height + ROW_GAP
  end

  -- Returns where the first row stands: two lines under the heading, which
  -- stands a line under the top of the safe area.
  local function top()
    return trx.ui.safe_area.y + line() * 3
  end

  -- How many rows show at once: as many as fit between the first row and the
  -- delete button. A list that does not fit gives its last line to the arrow
  -- that says it goes on.
  local function visible()
    local safe = trx.ui.safe_area
    local bottom = safe.y
      + safe.height
      - delete_save.button_height() * trx.ui.text_scale
    local fit = math.floor((bottom - top()) / line())
    if count > fit then
      fit = fit - 1
    end
    return math.max(1, math.min(count, fit))
  end

  -- Builds the text of the rows that show, once the selection moves.
  local function follow()
    left_on = slots[state.selected]
    local shown = visible()
    if state.selected < state.first then
      state.first = state.selected
    elseif state.selected > state.first + shown - 1 then
      state.first = state.selected - shown + 1
    end
    state.rows = {}
    for i = 1, shown do
      local index = state.first + i - 1
      local color = index == state.selected and accent.Role.PICKED
        or accent.Role.MENU_ROW
      local number, name, date = slot_text(slots[index])
      state.rows[i] = {
        number = number ~= nil and colored(color, number) or nil,
        name = colored(color, name),
        date = date ~= nil and colored(color, date) or nil,
      }
    end
  end

  local function paint()
    if state.is_asking then
      return
    end
    local primitive = trx.ui.primitive
    local w = trx.ui.canvas.width
    local height = line()
    local heading_w = primitive.measure_text(heading, TEXT_SCALE)
    primitive.text(
      heading,
      (w - heading_w) / 2,
      top() - height * 2,
      TEXT_SCALE
    )
    -- The names start past the widest number, where the original's place for
    -- them is too close.
    local number_x = across(NUMBER_X)
    local name_x = math.max(
      across(NAME_X),
      number_x + primitive.measure_text("000 ", TEXT_SCALE)
    )
    for _, row in ipairs(state.rows) do
      if row.number ~= nil then
        local number_w = primitive.measure_text(row.number .. " ", TEXT_SCALE)
        name_x = math.max(name_x, number_x + number_w)
      end
    end
    if state.first > 1 then
      arrows.centred(arrows.UP, top() - height)
    end
    if state.first + #state.rows - 1 < count then
      arrows.centred(arrows.DOWN, top() + height * #state.rows)
    end
    for i, row in ipairs(state.rows) do
      local y = top() + height * (i - 1)
      if row.number ~= nil then
        primitive.text(row.number, number_x, y, TEXT_SCALE)
        primitive.text(row.name, name_x, y, TEXT_SCALE)
        if row.date ~= nil then
          primitive.text(row.date, across(DATE_X), y, TEXT_SCALE)
        end
      else
        local name_w = primitive.measure_text(row.name, TEXT_SCALE)
        primitive.text(row.name, (w - name_w) / 2, y, TEXT_SCALE)
      end
    end
  end

  -- The rows follow the room they show in, which grows as the title screen
  -- hides its logo.
  local layout = trx.ui.signals.layout:on(follow)

  -- Shows a dialog in place of the list until the player closes it.
  local function ask(root, place, on_input)
    state.is_asking = true
    button:set_enabled(false)
    ctx:push({
      root = root,
      place = place,
      on_input = on_input,
      on_close = function()
        state.is_asking = false
      end,
    })
  end

  local function show_message(text)
    ask(requester.message(text), { y = 0.5 }, function(message, keys)
      if keys:pressed(Role.MENU_CONFIRM) or keys:pressed(Role.MENU_BACK) then
        message:close()
      end
    end)
  end

  local function confirm_delete(slot)
    local req = delete_save.confirm_req({ y = 0.5 })
    ask(req.widget, req.place, function(dialog, keys)
      local picked, cancelled = req.list:control(keys)
      if picked == nil and not cancelled then
        return
      end
      dialog:close()
      if
        picked ~= nil
        and delete_save.is_yes(req.rows[picked])
        and delete_save.delete(slot.slot_num, slot.pool, show_message)
      then
        follow()
      end
    end)
  end

  -- A pick lists only the saves that pass its filter, which a deleted save
  -- no longer does, so it has no delete button.
  ---@type trx.ui.StackLayer?
  local footer = nil
  if pick == nil then
    footer = ctx:push({
      root = button.widget,
      place = { x = 1, y = 1 },
    })
  end

  ---@type trx.ui.StackLayer
  local layer
  layer = ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return trx.ui.canvas.width, trx.ui.canvas.height
      end,
      paint = paint,
    }),
    place = function()
      return 0, 0
    end,
    on_close = function()
      layout:detach()
      if footer ~= nil then
        footer:close()
      end
    end,
    on_input = function(_, keys)
      if state.is_loaded then
        return
      end
      local slot = slots[state.selected]
      if
        footer ~= nil and button:control(keys, delete_save.is_deletable(slot))
      then
        confirm_delete(slot)
        return
      end
      local step = keys:pressed(Role.MENU_UP) and -1
        or keys:pressed(Role.MENU_DOWN) and 1
        or nil
      local to = step and scroll.step(state.selected, step, count)
      if to ~= nil then
        state.selected = to
        sound("move")
        follow()
      elseif keys:pressed(Role.MENU_CONFIRM) then
        if pick ~= nil then
          if slot ~= nil and pick.on_pick(slot) then
            layer:close()
          else
            sound("no")
          end
          return
        end
        if
          slot == nil
          or not can_take(slot)
          or (
            kind == M.Kind.LOAD
            and trx.savegame.is_free(slot.slot_num, slot.pool)
          )
        then
          sound("no")
          return
        end
        if kind == M.Kind.LOAD and stays then
          state.is_loaded = true
        else
          layer:close()
        end
        if kind == M.Kind.LOAD then
          trx.savegame.load(slot.slot_num, slot.pool)
        else
          trx.savegame.save(slot.slot_num, slot.pool)
        end
        on_done(true)
      elseif keys:pressed(Role.OPTION) or keys:pressed(Role.MENU_BACK) then
        layer:close()
        if pick ~= nil then
          pick.on_back()
        else
          on_done(false)
        end
      end
    end,
  })
  follow()
  return layer
end

-- Shows the list over a screen and runs it until the player picks a slot or
-- backs out. A load leaves for the game it loads, and a save stays in the
-- menu the list was opened from. With `stays` set, the list stays on screen
-- after a load and closes with the screen.
---@param ctx trx.ui.ScreenContext
---@param kind common.ui.flat_menu.saves.Kind
---@param on_done fun(done: boolean) Runs with `true` once a game is saved or loaded.
---@param stays? boolean
---@return trx.ui.StackLayer
function M.push(ctx, kind, on_done, stays)
  return open(ctx, kind, nil, on_done, stays)
end

-- Shows the saves that pass the filter of `pick`, under its heading, and runs
-- it until the player picks a save or backs out. Returns nil and plays the
-- refusal sound where no save passes.
---@param ctx trx.ui.ScreenContext
---@param pick common.ui.flat_menu.saves.Pick
---@return trx.ui.StackLayer?
function M.pick(ctx, pick)
  if #all_slots(pick.filter) == 0 then
    sound("no")
    return nil
  end
  return open(ctx, M.Kind.PICK, pick)
end

return M
