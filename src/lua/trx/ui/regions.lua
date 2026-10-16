local h = require("trx.internal.helpers")

require("trx.ui")
require("trx.ui.primitive")
require("trx.ui.widgets")
local at_level_end = require("trx.events").at_level_end

local ui = trx.ui
local base = require("trx.ui.widgets.base")

local primitive = trx.ui.primitive

-- The gap between two widgets placed in one region.
local ROOT_SPACING = 3

-- The root stack of each region.
---@type table<trx.ui.Region, trx.ui.Widget?>
local roots = {}

-- What regions keeps for each placed widget: the root that holds it, what the
-- root holds for it, the listener that moves it with a region signal, and the
-- level unload handler that removes it.
---@type table<trx.ui.Widget, { root: trx.ui.Widget, held: trx.ui.Widget, region_listener: trx.signal.Listener?, unload: trx.events.Listener? }>
local placed = setmetatable({}, { __mode = "k" })

-- The reservation of each root in the scene being built.
---@type table<trx.ui.Widget, integer>
local slots = setmetatable({}, { __mode = "k" })

-- The order of each child of a root, as `trx.ui.regions.place` took it.
---@type table<trx.ui.Widget, number>
local orders = setmetatable({}, { __mode = "k" })

-- The child that holds a widget placed to show alone, and whether its region
-- holds nothing else in the scene being built.
---@type table<trx.ui.Widget, trx.signal.Signal>
local alone = setmetatable({}, { __mode = "k" })

-- Horizontal alignment follows the region's screen edge.
local ALIGN_OF = {
  [trx.ui.Region.TOP_LEFT] = trx.ui.HAlign.LEFT,
  [trx.ui.Region.LEFT] = trx.ui.HAlign.LEFT,
  [trx.ui.Region.BOTTOM_LEFT] = trx.ui.HAlign.LEFT,
  [trx.ui.Region.TOP_CENTER] = trx.ui.HAlign.CENTER,
  [trx.ui.Region.CENTER] = trx.ui.HAlign.CENTER,
  [trx.ui.Region.BOTTOM_CENTER] = trx.ui.HAlign.CENTER,
  [trx.ui.Region.TOP_RIGHT] = trx.ui.HAlign.RIGHT,
  [trx.ui.Region.RIGHT] = trx.ui.HAlign.RIGHT,
  [trx.ui.Region.BOTTOM_RIGHT] = trx.ui.HAlign.RIGHT,
}

-------------------------------------------------------------------------------
-- The regions a script draws into
--
-- Widgets do not know where they are on screen. Regions give them a place:
-- each region owns one root stack and reserves room for that whole stack.
--
-- Reserving once per region keeps script widgets grouped together. They stack
-- after the engine's own UI in that region instead of interleaving with it.
-------------------------------------------------------------------------------

---@class (partial) trx.ui
---@field regions trx.ui.regions

---Places script widgets on the screen.
---
---The screen has nine regions. Engine UI uses those regions for bars, overlay
---text, inventory-ring hints, and dialogs. A widget placed in a region stacks
---after the engine UI in that region, and draws under it.
---
---Place a widget once when the script loads. Use signals when the widget must
---change later.
---@class (partial,exact) trx.ui.regions
ui.regions = h.namespace("ui.regions")

---@class (partial) trx.ui.regions
local regions = trx.ui.regions

local function root_of(region)
  local root = roots[region]
  if root == nil then
    root = trx.ui.widgets.Stack({
      children = {},
      orientation = trx.ui.Orientation.VERTICAL,
      spacing = ROOT_SPACING,
      align = ALIGN_OF[region],
    })
    roots[region] = root
  end
  return root
end

-- Adds a widget to the root of a region after every child of a lower or the
-- same order, and returns the root. A widget that has another holder is an
-- error at the given level.
local function attach(region, widget, level)
  local root = root_of(region)
  base.hold(widget, root, level + 2)
  local order = orders[widget] or 0
  local at = #root.children + 1
  for i, child in ipairs(root.children) do
    if (orders[child] or 0) > order then
      at = i
      break
    end
  end
  table.insert(root.children, at, widget)
  root:wake()
  return root
end

local function unattach(widget, root)
  for i, child in ipairs(root.children) do
    if child == widget then
      table.remove(root.children, i)
      break
    end
  end
  base.unhold(widget, root)
  root:wake()
end

local function remove(widget)
  local entry = placed[widget]
  if entry == nil then
    return false
  end
  placed[widget] = nil
  if entry.region_listener ~= nil then
    entry.region_listener:detach()
  end
  if entry.unload ~= nil then
    entry.unload:detach()
  end
  unattach(entry.held, entry.root)
  if entry.held ~= widget then
    alone[entry.held] = nil
    unattach(widget, entry.held)
  end
  return true
end

-- A widget a level script placed leaves the screen with the level that placed
-- it, as its event handlers do. One a global script placed stays for the
-- session.
local function bind_scope(widget)
  return at_level_end(function()
    remove(widget)
    widget:release()
  end)
end

---How a widget stands among the others in its region.
---@trx.record
---@class trx.ui.regions.place.options
---@field order? number Where the widget stacks: a lower order stacks first, and widgets of the same order stack in the order they were placed. `0` by default.
---@field alone? boolean Whether the widget shows only while nothing else in its region does, as an arrow in a corner that a bar can take. `false` by default.

---Places a widget in a region.
---
---If the region argument is a signal, the widget moves when the signal
---changes. Placing a widget that is already placed moves it.
---
---```lua
---trx.ui.regions.place(trx.ui.Region.TOP_LEFT, health_bar)
---```
---@param region any The target region, or a signal that holds one.
---@param widget trx.ui.Widget The widget to place.
---@param options? trx.ui.regions.place.options How it stands among the others.
function regions.place(region, widget, options)
  remove(widget)
  options = options or {}
  -- The region holds a widget placed to show alone through a stack of its
  -- own, which the region shows and hides.
  local held = widget
  if options.alone then
    local free = trx.signal.new(true)
    held = trx.ui.widgets.Stack({ visible = free, children = { widget } })
    alone[held] = free
  end
  orders[held] = options.order
  local entry = { held = held }
  if base.is_signal(region) then
    entry.root = attach(region:get(), held, 2)
    entry.region_listener = region:on(function(value)
      unattach(held, entry.root)
      entry.root = attach(value, held, 2)
    end)
  else
    entry.root = attach(region, held, 2)
  end
  entry.unload = bind_scope(widget)
  placed[widget] = entry
end

---Takes a widget out of its region and keeps it, so that it can be placed
---again.
---
---To be done with a widget, release it with `trx.ui.Widget:release` instead,
---which takes it out of its region as well. Widgets that a level script placed
---are released when the level ends.
---@param widget trx.ui.Widget The widget to remove.
---@return boolean # Whether the widget was in a region.
function regions.remove(widget)
  return remove(widget)
end

base.set_unplace(remove)

-- Reserve room for each region while the scene is being built. Signal changes
-- have already invalidated any stale widget measurements by this point.
trx.ui.on_reserve(function(region)
  local root = roots[region]
  if root == nil then
    return
  end
  slots[root] = nil
  -- A widget placed to show alone shows while no other child of the region
  -- takes room.
  local children = root.children --[[@as trx.ui.Widget[] ]]
  local taken = false
  for _, child in ipairs(children) do
    if alone[child] == nil and child:is_visible() then
      local w, h = child:measure()
      taken = taken or (w > 0 and h > 0)
    end
  end
  for _, child in ipairs(children) do
    if alone[child] ~= nil then
      alone[child]:set(not taken)
    end
  end
  if root:is_visible() then
    local w, h = root:measure()
    if w > 0 and h > 0 then
      slots[root] = primitive.reserve(region, w, h)
    end
  end
end)

-- Paint widgets after the engine has assigned boxes to their reservations.
trx.ui.on_paint(function()
  for _, root in pairs(roots) do
    local slot = slots[root]
    if slot ~= nil then
      local x, y, w, h = primitive.slot_box(slot)
      if x ~= nil then
        root:paint(x, y, w, h)
      end
    end
  end
end)
