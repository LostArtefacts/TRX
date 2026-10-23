-- A list as the original TR4 prints its menus: a heading, and rows under it,
-- the picked one pulsing. The list stands in the middle of the room that the
-- rest of the interface leaves it, such as below the logo of the title
-- screen. A list longer than that room scrolls, and the selection wraps
-- around where the player asks for it.

local accent = require("common.ui.accent")
local arrows = require("common.ui.flat_menu.arrows")
local scroll = require("common.ui.scroll")
local TEXT_SCALE = require("common.ui.flat_menu.text").SCALE

local M = {}

local Role = trx.input.Role

-- The room between two rows of the list, in canvas units.
local ROW_GAP = 2

local function colored(role, text)
  return accent.text(text, role)
end

-- The height of a line of the list.
local function line()
  local _, height = trx.ui.primitive.measure_text("Ag", TEXT_SCALE)
  return height + ROW_GAP
end

---@class common.ui.flat_menu.list.Row
---@field text string
---@field choose? fun() Runs when the player confirms the row.

---@class common.ui.flat_menu.List
---@field heading? string
---@field heading_role string
---@field heading_gap number How many lines above the first row the heading stands.
---@field rows common.ui.flat_menu.list.Row[]
---@field selected integer
---@field first integer The row at the top of the room, as the list scrolls.
local List = {}
List.__index = List

---@class common.ui.flat_menu.list.settings
---@field heading? string
---@field heading_role? string The role of the heading among `common.ui.accent.Role`. A flat menu heading by default.
---@field heading_gap? number How many lines above the first row the heading stands: a line and a half by default, as the original's pause menu has it.
---@field rows common.ui.flat_menu.list.Row[]
---@field selected? integer The row picked first. 1 by default.

-- Makes a list.
---@param settings common.ui.flat_menu.list.settings
---@return common.ui.flat_menu.List
function M.new(settings)
  local self = setmetatable({
    heading = settings.heading,
    heading_role = settings.heading_role or accent.Role.MENU_HEADING,
    heading_gap = settings.heading_gap or 3 / 2,
    rows = settings.rows,
    selected = settings.selected or 1,
    first = 1,
  }, List)
  self:follow()
  return self
end

-- Returns where the first row stands, how many rows show, and how far above
-- the first row the heading stands.
function List:layout()
  local primitive = trx.ui.primitive
  local width = self.heading ~= nil
      and primitive.measure_text(self.heading, TEXT_SCALE)
    or 0
  for _, row in ipairs(self.rows) do
    width = math.max(width, (primitive.measure_text(row.text, TEXT_SCALE)))
  end
  local area = trx.ui.safe_area_for(width)
  local step = line()
  local above = self.heading ~= nil and step * self.heading_gap or 0
  local fit = math.floor((area.height - above) / step)
  local shown = math.max(math.min(#self.rows, fit), 0)
  local height = above + step * shown
  return area.y + (area.height - height) / 2 + above, shown, above
end

-- Scrolls the list so that the picked row shows.
function List:follow()
  local _, shown = self:layout()
  shown = math.max(shown, 1)
  if self.selected < self.first then
    self.first = self.selected
  elseif self.selected > self.first + shown - 1 then
    self.first = self.selected - shown + 1
  end
end

-- Returns the row picked, or nil for an empty list.
---@return common.ui.flat_menu.list.Row?
function List:picked()
  return self.rows[self.selected]
end

-- Moves the selection up or down. Returns whether it moved.
---@param keys trx.ui.LayerKeys
---@return boolean
function List:move(keys)
  local count = #self.rows
  if count == 0 then
    return false
  end
  local step = keys:pressed(Role.MENU_UP) and -1
    or keys:pressed(Role.MENU_DOWN) and 1
    or nil
  local to = step and scroll.step(self.selected, step, count)
  if to == nil then
    return false
  end
  self.selected = to
  self:follow()
  return true
end

-- Draws the list.
function List:paint()
  local primitive = trx.ui.primitive
  local w = trx.ui.canvas.width
  local step = line()
  local y, shown, above = self:layout()
  local first = self.first
  if self.selected > first + shown - 1 then
    first = math.max(self.selected - shown + 1, 1)
  end

  local function centred(text, at)
    local text_w = primitive.measure_text(text, TEXT_SCALE)
    primitive.text(text, (w - text_w) / 2, at, TEXT_SCALE)
  end

  if self.heading ~= nil then
    centred(colored(self.heading_role, self.heading), y - above)
  end
  for i = first, math.min(first + shown - 1, #self.rows) do
    local color = i == self.selected and accent.Role.PICKED
      or accent.Role.MENU_ROW
    centred(colored(color, self.rows[i].text), y + step * (i - first))
  end
  if shown > 0 and first > 1 then
    arrows.beside(arrows.UP, y)
  end
  if shown > 0 and first + shown - 1 < #self.rows then
    arrows.beside(arrows.DOWN, y + step * (shown - 1))
  end
end

return M
