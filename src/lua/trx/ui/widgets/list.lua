local raw = trxc.ui
local api = trx.api
local base = require("trx.ui.widgets.base")

require("trx.config")
require("trx.game")

local primitive = trx.ui.primitive
local value_of = base.value_of
local new_widget = base.new_widget
local text_scale = base.text_scale

-- Match the engine's list-dialog geometry in canvas units at the default text size.

local LIST_ROW_HEIGHT = 15
local LIST_HINT_HEIGHT = 7
local LIST_HINT_SCALE = 0.7
local LIST_HINT_ANCHOR_UP = 1.5
local LIST_HINT_ANCHOR_DOWN = -1.0
local LIST_RIGHT_GAP = 8
local LIST_RULE_LINE = 2
local LIST_RULE_PAD_Y = 4
local LIST_RULE_HEIGHT = LIST_RULE_LINE + 2 * LIST_RULE_PAD_Y

local function list_rule_overhang()
  return trx.game.tr_version >= 2 and 7 or 10
end

local function list_frame_overhang()
  return trx.game.tr_version == 1 and 1 or 0
end

api.type("ui.ListRow", {
  record = true,
  description = "One entry of a `trx.ui.widgets.List`.",
  fields = {
    text = {
      type = "string",
      description = "The text. It is centered unless the row has a right part.",
    },
    right = {
      type = "string",
      optional = true,
      description = "Text drawn against the right edge. The main text is then drawn against the left edge.",
    },
    rule = {
      type = "boolean",
      optional = true,
      description = "Whether a line separates the row from the row above it.",
    },
  },
})

local function list_visible(list)
  local visible = value_of(list.visible) or #list._rows
  if list.reserve then
    return math.max(visible, 1)
  end
  return math.max(math.min(visible, #list._rows), 1)
end

local function list_scrolls(list)
  return list.scroll_hints ~= false and list_visible(list) < #list._rows
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

local List = api.type("ui.List", {
  extends = "ui.Widget",
  description = [[
A column of rows that the player picks one entry from. The row under the cursor
is drawn in a frame. Arrows show where the list runs past the rows it shows.]],
  methods = {
    set_rows = {
      description = [[
Replaces the rows. The cursor stays on the same index where it can.]],
      params = {
        {
          name = "rows",
          type = "ui.ListRow",
          list = true,
          description = "The new rows.",
        },
      },
      impl = function(self, rows)
        self._rows = rows
        self._selected = math.max(math.min(self._selected, #rows), 1)
        self:wake()
        list_scroll_into_view(self)
      end,
    },
    selection = {
      description = "Returns the index of the row under the cursor.",
      returns = {
        type = "integer",
        nullable = true,
        description = "The index, or `nil` for an empty list.",
      },
      impl = function(self)
        if #self._rows == 0 then
          return nil
        end
        return self._selected
      end,
    },
    select = {
      description = "Moves the cursor to a row. Does nothing for an index out of range.",
      params = {
        { name = "index", type = "integer", description = "The row." },
      },
      impl = function(self, index)
        if index >= 1 and index <= #self._rows then
          self._selected = index
          list_scroll_into_view(self)
        end
      end,
    },
    move = {
      description = [[
Moves the cursor by a number of rows. Past either end, the cursor goes to the
other end where the `ui.enable_wraparound` setting is on, and stays otherwise.
<!--noref: ui.enable_wraparound-->]],
      params = {
        {
          name = "step",
          type = "integer",
          description = "How many rows to move. Negative moves up.",
        },
      },
      returns = { type = "boolean", description = "Whether the cursor moved." },
      impl = function(self, step)
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
      end,
    },
    control = {
      description = [[
Reads the menu keys for one tick. Up and down move the cursor, and confirm picks
the row under it. Uses up only the presses that it reads.]],
      params = {
        {
          name = "keys",
          type = "ui.LayerKeys",
          description = "The input of the layer the list is on.",
        },
      },
      returns = {
        type = "integer",
        nullable = true,
        description = "The picked row, or `nil` where none was picked.",
      },
      impl = function(self, keys)
        if keys:pressed(trx.input.Role.MENU_DOWN) then
          self:move(1)
        elseif keys:pressed(trx.input.Role.MENU_UP) then
          self:move(-1)
        end
        if #self._rows > 0 and keys:pressed(trx.input.Role.MENU_CONFIRM) then
          return self._selected
        end
        return nil
      end,
    },
  },
})

local function list_row_width(list, row)
  local scale = raw.drawn_text_scale()
  local width = primitive.measure_text(row.text, 1.0) / scale
  if row.right ~= nil then
    width = width
      + LIST_RIGHT_GAP
      + primitive.measure_text(row.right, 1.0) / scale
  end
  return width + 2 * list.row_pad
end

local function list_measure(list)
  list_scroll_into_view(list)
  local scale = raw.drawn_text_scale()
  local count = list_visible(list)
  local width = list.width or 0
  for _, row in ipairs(list._rows) do
    width = math.max(width, list_row_width(list, row))
  end
  local height = count * LIST_ROW_HEIGHT
    + (count - 1) * list.row_spacing
    + list_rules(list, count) * LIST_RULE_HEIGHT
  if list_scrolls(list) then
    height = height + 2 * LIST_HINT_HEIGHT
  end
  return width * scale, height * scale
end

local function list_hint(glyph, anchor, x, y, w, scale)
  local gw, gh = primitive.measure_text(glyph, LIST_HINT_SCALE)
  local slack = LIST_HINT_HEIGHT * scale - gh
  primitive.text(
    glyph,
    x + (w - gw) / 2,
    y + slack * anchor,
    LIST_HINT_SCALE,
    0
  )
end

local function list_draw_row(list, row, x, y, w, scale)
  local pad = list.row_pad * scale
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
      list_hint("\\{arrow up}", LIST_HINT_ANCHOR_UP, x, top, w, scale)
    end
    top = top + LIST_HINT_HEIGHT * scale
  end

  for i = list._first, last do
    local row = list._rows[i]
    if i > 1 and row.rule then
      local overhang = list_rule_overhang() * scale
      primitive.horizontal_line(
        x - overhang,
        x + w + overhang,
        top + (LIST_RULE_PAD_Y + LIST_RULE_LINE / 2) * scale,
        0
      )
      top = top + LIST_RULE_HEIGHT * scale
    end
    if i == list._selected then
      local overhang = list_frame_overhang() * scale
      primitive.panel(
        x,
        top - overhang,
        160,
        w,
        LIST_ROW_HEIGHT * scale + 2 * overhang,
        trx.ui.FrameStyle.SELECTED
      )
    end
    list_draw_row(list, row, x, top, w, scale)
    top = top + (LIST_ROW_HEIGHT + list.row_spacing) * scale
  end

  if scrolls and last < #list._rows then
    local _, height = list:measure()
    list_hint(
      "\\{arrow down}",
      LIST_HINT_ANCHOR_DOWN,
      x,
      y + height - LIST_HINT_HEIGHT * scale,
      w,
      scale
    )
  end
end

api.define("ui.widgets.List", {
  description = [[
A column of rows that the player picks one entry from.

The list keeps the cursor and the scroll position. Read the player's input
with `trx.ui.List:control` from the input callback of the layer that holds the
list.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The list settings.",
      fields = {
        {
          name = "rows",
          type = "ui.ListRow",
          list = true,
          optional = true,
          description = "The rows. None by default.",
        },
        {
          name = "visible",
          type = "any",
          optional = true,
          description = [[
How many rows to show at once, or a signal that holds that value. Every row by
default.]],
        },
        {
          name = "reserve",
          type = "boolean",
          optional = true,
          description = [[
Whether to keep room for the visible rows when the list holds fewer.
`false` by default.]],
        },
        {
          name = "width",
          type = "number",
          optional = true,
          description = "The least width, in canvas units at the default text size.",
        },
        {
          name = "row_pad",
          type = "number",
          optional = true,
          description = "The room on each side of a row's text. `4` by default.",
        },
        {
          name = "row_spacing",
          type = "number",
          optional = true,
          description = "The gap between two rows. `3` by default.",
        },
        {
          name = "scroll_hints",
          type = "boolean",
          optional = true,
          description = "Whether to show arrows where the list runs past its rows. `true` by default.",
        },
        {
          name = "shown",
          type = "any",
          optional = true,
          description = "Whether the list is shown, or a signal that holds that value.",
        },
      },
    },
  },
  returns = { type = "ui.List", description = "The list." },
  examples = {
    [[local list = trx.ui.widgets.List({
  rows = { { text = "Yes" }, { text = "No" } },
})]],
  },
  impl = function(settings)
    local self = new_widget(settings, list_measure, list_paint)
    setmetatable(self, List)
    self._rows = settings.rows or {}
    self.rows = nil
    self.row_pad = settings.row_pad or 4
    self.row_spacing = settings.row_spacing or 3
    self._selected = 1
    self._first = 1
    return self:wakes_on(self.visible, text_scale())
  end,
})
