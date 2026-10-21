-- The list of save slots that the original TR4 shows to save or to load a
-- game: a heading at the top, and a line for each slot with the number of the
-- save and the level it was made in. The quick saves come first, as the
-- passport lists them, and can be loaded but not saved over.

local arrows = require("common.ui.flat_menu.arrows")
local pages = require("common.ui.screens.passport.pages")
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

local COLOR_HEADING = 6
local COLOR_PICKED = 1
local COLOR_OTHER = 2

---@class common.ui.flat_menu.saves.Slot
---@field slot_num integer
---@field pool integer

-- The slot the list was left on, which every list opens on again, as in the
-- original.
---@type common.ui.flat_menu.saves.Slot?
local left_on = nil

---@enum common.ui.flat_menu.saves.Kind
M.Kind = {
  SAVE = "save",
  LOAD = "load",
}

local function colored(color, text)
  return ("\\{color %d}%s"):format(color, text)
end

-- The x of a point the original gives across its 640-pixel screen.
local function across(x)
  local h = trx.ui.canvas.height
  return trx.ui.canvas.width / 2 + x * h * 4 / 3 / 640
end

-- Returns every slot the list shows: the quick saves, then the numbered
-- slots.
---@return common.ui.flat_menu.saves.Slot[]
local function all_slots()
  ---@type common.ui.flat_menu.saves.Slot[]
  local slots = {}
  pages.each_slot(function(slot_num, pool)
    slots[#slots + 1] = { slot_num = slot_num, pool = pool }
  end)
  return slots
end

-- Returns the number of a save, the level it was made in, and when it was
-- made, or no number and the text that says the slot is empty.
local function slot_text(slot)
  local info = trx.savegame.info(slot.slot_num, slot.pool)
  if info == nil then
    return nil, trx.locale.get("general/flat_menu/empty_slot"), nil
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

-- Shows the list over a screen and runs it until the player picks a slot or
-- backs out. A load leaves for the game it loads, and a save stays in the
-- menu the list was opened from.
---@param ctx trx.ui.ScreenContext
---@param kind common.ui.flat_menu.saves.Kind
---@param on_done fun(done: boolean) Runs with `true` once a game is saved or loaded.
---@return trx.ui.StackLayer
function M.push(ctx, kind, on_done)
  local slots = all_slots()
  local count = #slots
  -- A save cannot go into a quick save slot, so a save opens on the first
  -- numbered slot unless it was left on another.
  local function can_take(slot)
    return kind == M.Kind.LOAD or slot.pool == trx.savegame.Pool.NORMAL
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
  ---@type { selected: integer, first: number, rows: table[] }
  local state = { selected = selected or 1, first = 1, rows = {} }

  local heading = colored(
    COLOR_HEADING,
    kind == M.Kind.SAVE and trx.locale.get("general/passport/save_game")
      or trx.locale.get("general/passport/load_game")
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
  -- foot of the safe area. A list that does not fit gives its last line to
  -- the arrow that says it goes on.
  local function visible()
    local safe = trx.ui.safe_area
    local fit = math.floor((safe.y + safe.height - top()) / line())
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
      local color = index == state.selected and COLOR_PICKED or COLOR_OTHER
      local number, name, date = slot_text(slots[index])
      state.rows[i] = {
        number = number ~= nil and colored(color, number) or nil,
        name = colored(color, name),
        date = date ~= nil and colored(color, date) or nil,
      }
    end
  end

  local function paint()
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
    on_input = function(_, keys)
      local step = keys:pressed(Role.MENU_UP) and -1
        or keys:pressed(Role.MENU_DOWN) and 1
        or nil
      local to = step and scroll.step(state.selected, step, count)
      if to ~= nil then
        state.selected = to
        sound("move")
        follow()
      elseif keys:pressed(Role.MENU_CONFIRM) then
        local slot = slots[state.selected]
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
        layer:close()
        if kind == M.Kind.LOAD then
          trx.savegame.load(slot.slot_num, slot.pool)
        else
          trx.savegame.save(slot.slot_num, slot.pool)
        end
        on_done(true)
      elseif keys:pressed(Role.OPTION) or keys:pressed(Role.MENU_BACK) then
        layer:close()
        on_done(false)
      end
    end,
  })
  follow()
  return layer
end

return M
