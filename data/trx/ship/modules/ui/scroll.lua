-- A window onto a list of items, and the item the cursor is on, as the game's
-- dialogs scroll. Items count from 1. The cursor is nil while it is on no item.
--
-- Usage:
--   local scroll = require("common.ui.scroll")
--   local list = scroll.new({ count = 20, visible = 8 })
--   list:select_next(trx.config.get("ui.enable_wraparound"))
--   local first = list.first:get()

local M = {}

trx.locale.declare({
  ["settings/ui.enable_wraparound/title"] = "Scroll wrap",
  ["settings/ui.enable_wraparound/description"] = "Lets directional navigation in menus loop around.",
})

trx.config.declare({
  key = "ui.enable_wraparound",
  kind = "boolean",
  default = true,
})

---@class common.ui.Scroll
---@field first trx.signal.Signal The first item in the window.
---@field selected trx.signal.Signal The item the cursor is on, or nil for none.
---@field visible trx.signal.Signal How many items the window shows.
---@field count trx.signal.Signal How many items there are.
local Scroll = {}
Scroll.__index = Scroll

---@param value number
---@param low number
---@param high number
---@return integer
local function clamp(value, low, high)
  return math.max(low, math.min(value, high)) --[[@as integer]]
end

-- Moves the window so that it shows as many items as it can, and the cursor
-- where `follow` is set.
---@private
function Scroll:settle(follow)
  local first = self.first:get()
  local visible = self.visible:get()
  local selected = self.selected:get()
  if follow and selected ~= nil then
    first = clamp(first, selected - visible + 1, selected)
  end
  first = math.max(math.min(first, self.count:get() - visible + 1), 1)
  self.first:set(first)
end

-- Moves the cursor down one item. Returns false where it is on the last item
-- and `wrap` is not set.
---@param wrap? boolean Whether to go from the last item to the first.
---@return boolean
function Scroll:select_next(wrap)
  local selected = self.selected:get() or 0
  if selected < self.count:get() then
    self.selected:set(selected + 1)
  elseif wrap then
    self.selected:set(1)
  else
    return false
  end
  self:settle(true)
  return true
end

-- Moves the cursor up one item. Returns false where it is on the first item
-- and `wrap` is not set.
---@param wrap? boolean Whether to go from the first item to the last.
---@return boolean
function Scroll:select_prev(wrap)
  local selected = self.selected:get()
  if selected ~= nil and selected > 1 then
    self.selected:set(selected - 1)
  elseif wrap then
    self.selected:set(self.count:get())
  else
    return false
  end
  self:settle(true)
  return true
end

-- Puts the cursor on an item, or on none for nil, and brings it into view.
---@param index? integer
function Scroll:select(index)
  if index ~= nil then
    index = clamp(index, 1, math.max(self.count:get(), 1))
  end
  self.selected:set(index)
  self:settle(index ~= nil)
end

-- Moves the window down one item, leaving the cursor. Returns false where the
-- window shows the last item and `wrap` is not set.
---@param wrap? boolean Whether to go back to the top.
---@return boolean
function Scroll:scroll_down(wrap)
  local first = self.first:get()
  if first + self.visible:get() <= self.count:get() then
    self.first:set(first + 1)
  elseif wrap then
    self.first:set(1)
  else
    return false
  end
  self:settle(false)
  return true
end

-- Moves the window up one item, leaving the cursor. Returns false where the
-- window shows the first item and `wrap` is not set.
---@param wrap? boolean Whether to go to the bottom.
---@return boolean
function Scroll:scroll_up(wrap)
  local first = self.first:get()
  if first > 1 then
    self.first:set(first - 1)
  elseif wrap then
    self.first:set(self.count:get())
  else
    return false
  end
  self:settle(false)
  return true
end

-- Changes how many items there are, keeping the cursor on one of them.
---@param count integer
function Scroll:set_count(count)
  self.count:set(count)
  local selected = self.selected:get()
  if selected ~= nil then
    self.selected:set(clamp(selected, 1, math.max(count, 1)))
  end
  self:settle(true)
end

-- Changes how many items the window shows.
---@param visible integer
function Scroll:set_visible(visible)
  self.visible:set(math.max(visible, 0))
  self:settle(true)
end

-- The last item in the window.
---@return integer
function Scroll:last()
  return math.min(self.first:get() + self.visible:get() - 1, self.count:get()) --[[@as integer]]
end

-- Returns a signal that holds whether an item is in the window.
---@param index integer
---@return trx.signal.Signal
function Scroll:shows(index)
  return trx.signal.combine(
    self.first,
    self.visible,
    self.count,
    function(first, visible, count)
      return index >= first and index < first + visible and index <= count
    end
  )
end

-- Returns a signal that holds whether items lie above the window.
---@return trx.signal.Signal
function Scroll:more_above()
  return self.first:map(function(first)
    return first > 1
  end)
end

-- Returns a signal that holds whether items lie below the window.
---@return trx.signal.Signal
function Scroll:more_below()
  return trx.signal.combine(
    self.first,
    self.visible,
    self.count,
    function(first, visible, count)
      return first + visible - 1 < count
    end
  )
end

---@class common.ui.Scroll.settings
---@field count? integer How many items there are. `0` by default.
---@field visible? integer How many items the window shows. `0` by default.
---@field selected? integer The item the cursor starts on. None by default.

-- Creates a window onto a list.
---@param settings? common.ui.Scroll.settings
---@return common.ui.Scroll
function M.new(settings)
  settings = settings or {}
  local self = setmetatable({
    first = trx.signal.new(1),
    selected = trx.signal.new(nil),
    visible = trx.signal.new(math.max(settings.visible or 0, 0)),
    count = trx.signal.new(settings.count or 0),
  }, Scroll)
  if settings.selected ~= nil then
    self:select(settings.selected)
  end
  return self
end

return M
