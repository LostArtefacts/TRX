local raw = trxc.ui
local h = require("trx.internal.helpers")
local base = require("trx.ui.widgets.base")

require("trx.config")

local primitive = trx.ui.primitive
---@class (partial) trx.ui.widgets
local widgets = trx.ui.widgets
local value_of = base.value_of
local new_widget = base.new_widget

-- The geometry of a list in canvas units at the default text size.

local LIST_HINT_SCALE = 0.7
local LIST_HINT_NUDGE_UP = -3.5
local LIST_HINT_NUDGE_DOWN = 5.25
local LIST_RIGHT_GAP = 8
local LIST_RULE_LINE = 2
local LIST_RULE_PAD_Y = 4
local LIST_RULE_HEIGHT = LIST_RULE_LINE + 2 * LIST_RULE_PAD_Y

local DEFAULTS = {
  row_pad = 4,
  row_spacing = 3,
  row_height = 15,
  hint_height = 7,
  rule_overhang = 7,
  frame_overhang = 0,
}

-- Reads a setting of a list, or its default where the list has none.
local function setting(list, key)
  local value = value_of(list[key])
  if value == nil then
    return DEFAULTS[key]
  end
  return value
end

---One entry of a `trx.ui.widgets.List`.
---@trx.record
---@class trx.ui.ListRow
---@field text string The text. It is centered unless the row has a right part.
---@field right? string Text drawn against the right edge. The main text is then drawn against the left edge.
---@field rule? boolean Whether a line separates the row from the row above it.

local function list_visible(list)
  local visible = value_of(list.page_rows) or #list._rows
  if value_of(list.reserve) then
    return math.max(visible, 1)
  end
  return math.max(math.min(visible, #list._rows), 1)
end

local function list_scrolls(list)
  return value_of(list.scroll_hints) ~= false
    and list_visible(list) < #list._rows
end

local function list_rules_from(list, first, count)
  local rules = 0
  for i = first, math.min(first + count - 1, #list._rows) do
    if i > 1 and list._rows[i].rule then
      rules = rules + 1
    end
  end
  return rules
end

-- Keep the list height constant while it scrolls by reserving the largest rule count.
local function list_rules(list, count)
  local rules = 0
  for first = 1, math.max(#list._rows - count + 1, 1) do
    rules = math.max(rules, list_rules_from(list, first, count))
  end
  return rules
end

local function list_scroll_into_view(list)
  local visible = list_visible(list)
  local first = list._first
  if list._selected < first then
    first = list._selected
  elseif list._selected > first + visible - 1 then
    first = list._selected - visible + 1
  end
  list._first = math.max(math.min(first, #list._rows - visible + 1), 1)
end

---A column of rows that the player picks one entry from. The row under the
---cursor is drawn in a frame, or in the picked colour while
---`trx.ui.frames_shown` is off. Arrows show where the list runs past the rows
---it shows.
---@class (exact) trx.ui.List: trx.ui.Widget
local List = h.class("ui.List", { extends = "ui.Widget" })

---Replaces the rows. The cursor stays on the same index where it can.
---@param rows trx.ui.ListRow[] The new rows.
function List:set_rows(rows)
  self._rows = rows
  self._selected = math.max(math.min(self._selected, #rows), 1)
  self:wake()
  list_scroll_into_view(self)
end

---Returns the index of the row under the cursor.
---@return integer? # The index, or `nil` for an empty list.
function List:selection()
  if #self._rows == 0 then
    return nil
  end
  return self._selected
end

---Moves the cursor to a row. Does nothing for an index out of range.
---@param index integer The row.
function List:select(index)
  if index >= 1 and index <= #self._rows then
    self._selected = index
    list_scroll_into_view(self)
  end
end

---Moves the cursor by a number of rows. Past either end, the cursor goes to
---the other end where the `ui.enable_wraparound` setting is on, and stays
---otherwise. <!--noref: ui.enable_wraparound-->
---@param step integer How many rows to move. Negative moves up.
---@return boolean # Whether the cursor moved.
function List:move(step)
  local count = #self._rows
  if count == 0 then
    return false
  end
  local index = self._selected + step
  if index < 1 or index > count then
    if not trx.config.get("ui.enable_wraparound") then
      return false
    end
    index = index < 1 and count or 1
  end
  if index == self._selected then
    return false
  end
  self._selected = index
  list_scroll_into_view(self)
  return true
end

---Reads the menu keys for one tick. Up and down move the cursor, confirm
---picks the row under it, and back reports that the player backed out. Uses up
---only the presses that it reads.
---@param keys trx.ui.LayerKeys The input of the layer the list is on.
---@return integer? # The picked row, or `nil` where none was picked.
---@return boolean # Whether the player backed out.
function List:control(keys)
  if keys:pressed(trx.input.Role.MENU_DOWN) then
    self:move(1)
  elseif keys:pressed(trx.input.Role.MENU_UP) then
    self:move(-1)
  end
  if #self._rows > 0 and keys:pressed(trx.input.Role.MENU_CONFIRM) then
    return self._selected, false
  end
  return nil, keys:pressed(trx.input.Role.MENU_BACK)
end

local function list_row_width(list, row)
  local scale = raw.drawn_text_scale()
  local width = primitive.measure_text(row.text, 1.0) / scale
  if row.right ~= nil then
    width = width
      + LIST_RIGHT_GAP
      + primitive.measure_text(row.right, 1.0) / scale
  end
  return width + 2 * setting(list, "row_pad")
end

local function list_measure(list)
  local scale = raw.drawn_text_scale()
  local count = list_visible(list)
  list_scroll_into_view(list)
  local width = value_of(list.min_w) or 0
  for _, row in ipairs(list._rows) do
    width = math.max(width, list_row_width(list, row))
  end
  local height = count * setting(list, "row_height")
    + (count - 1) * setting(list, "row_spacing")
    + list_rules(list, count) * LIST_RULE_HEIGHT
  if list_scrolls(list) then
    height = height + 2 * setting(list, "hint_height")
  end
  return width * scale, height * scale
end

-- Draws an arrow with its line centred on the room for the arrow, then moved
-- by `nudge`.
local function list_hint(list, glyph, nudge, x, y, w, scale)
  local gw, gh = primitive.measure_text(glyph, LIST_HINT_SCALE)
  local slack = setting(list, "hint_height") * scale - gh
  primitive.text(
    glyph,
    x + (w - gw) / 2,
    y + slack / 2 + nudge * scale,
    LIST_HINT_SCALE,
    0
  )
end

local function list_draw_row(list, row, x, y, w, scale)
  local pad = setting(list, "row_pad") * scale
  if row.right ~= nil then
    primitive.text(row.text, x + pad, y, 1.0, 0)
    local rw = primitive.measure_text(row.right, 1.0)
    primitive.text(row.right, x + w - pad - rw, y, 1.0, 0)
    return
  end
  local tw = primitive.measure_text(row.text, 1.0)
  primitive.text(row.text, x + (w - tw) / 2, y, 1.0, 0)
end

local function list_paint(list, x, y, w)
  local scale = raw.drawn_text_scale()
  local count = list_visible(list)
  local last = math.min(list._first + count - 1, #list._rows)
  local scrolls = list_scrolls(list)
  local top = y

  if scrolls then
    if list._first > 1 then
      list_hint(list, "\\{arrow up}", LIST_HINT_NUDGE_UP, x, top, w, scale)
    end
    top = top + setting(list, "hint_height") * scale
  end

  for i = list._first, last do
    local row = list._rows[i]
    if i > 1 and row.rule then
      local overhang = setting(list, "rule_overhang") * scale
      primitive.horizontal_line(
        x - overhang,
        x + w + overhang,
        top + (LIST_RULE_PAD_Y + LIST_RULE_LINE / 2) * scale,
        0
      )
      top = top + LIST_RULE_HEIGHT * scale
    end
    local picked = i == list._selected
    if picked then
      local overhang = setting(list, "frame_overhang") * scale
      primitive.panel(
        x,
        top - overhang,
        base.FRAME_Z,
        w,
        setting(list, "row_height") * scale + 2 * overhang,
        trx.ui.FrameStyle.SELECTED
      )
    end
    if picked and not trx.ui.frames_shown then
      local previous = raw.set_text_color(base.PICKED_COLOR)
      list_draw_row(list, row, x, top, w, scale)
      raw.set_text_color(previous)
    else
      list_draw_row(list, row, x, top, w, scale)
    end
    top = top
      + (setting(list, "row_height") + setting(list, "row_spacing")) * scale
  end

  if scrolls and last < #list._rows then
    local _, height = list:measure()
    list_hint(
      list,
      "\\{arrow down}",
      LIST_HINT_NUDGE_DOWN,
      x,
      y + height - setting(list, "hint_height") * scale,
      w,
      scale
    )
  end
end

---@class (exact) trx.ui.widgets.List.settings: trx.ui.widgets.settings
---@field rows? trx.ui.ListRow[] The rows. None by default.
---@field page_rows? any How many rows to show at once, or a signal that holds that value. Every row by default.
---@field reserve? any Whether to keep room for the rows of a page when the list holds fewer, or a signal that holds that value. `false` by default.
---@field min_w? any The least width, in canvas units at the default text size, or a signal that holds it.
---@field row_pad? any The room on each side of a row's text, or a signal that holds it. `4` by default.
---@field row_spacing? any The gap between two rows, or a signal that holds it. `3` by default.
---@field row_height? any The height of a row, or a signal that holds it. `15` by default.
---@field hint_height? any The height of the arrows that say the list runs past its rows, or a signal that holds it. `7` by default.
---@field rule_overhang? any How far a rule between rows reaches past each side of the list, or a signal that holds it. `7` by default.
---@field frame_overhang? any How far the frame of the row under the cursor reaches above and below the row, or a signal that holds it. `0` by default.
---@field scroll_hints? any Whether to show arrows where the list runs past its rows, or a signal that holds that value. `true` by default.

---A column of rows that the player picks one entry from. Its lengths are in
---canvas units at the default text size.
---
---The list keeps the cursor and the scroll position. Read the player's input
---with `trx.ui.List:control` from the input callback of the layer that holds
---the list.
---
---```lua
---local list = trx.ui.widgets.List({
---  rows = { { text = "Yes" }, { text = "No" } },
---})
---```
---@param settings trx.ui.widgets.List.settings The list settings.
---@return trx.ui.List # The list.
function widgets.List(settings)
  local rest = {}
  for key, value in pairs(settings) do
    if key ~= "rows" then
      rest[key] = value
    end
  end
  local self = new_widget(rest, list_measure, list_paint, List)
  self._rows = settings.rows or {}
  self._selected = 1
  self._first = 1
  return self
end
