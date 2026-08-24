local raw = trxc.events
local api = trx.api

require("trx.ui")
require("trx.ui.primitive")
require("trx.ui.widgets")
require("trx.events")

local primitive = trx.ui.primitive

-- Root widget for each region, plus its reservation for the current scene.
local roots = {}

-- Widget to show when a region's root has no visible size.
local fallbacks = {}

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

api.namespace("ui.regions", {
  description = [[
Places script widgets on the screen.

The screen has nine regions. Engine UI uses those regions for bars, overlay
text, inventory-ring hints, and dialogs. A widget placed in a region stacks
after the engine UI in that region.

Place a widget once when the script loads. Use signals when the widget must
change later.]],
})

local function root_of(region)
  local root = roots[region]
  if root == nil then
    root = trx.ui.widgets.Stack({
      children = {},
      orientation = trx.ui.Orientation.VERTICAL,
      spacing = 3,
      align = ALIGN_OF[region] or trx.ui.HAlign.LEFT,
    })
    roots[region] = root
  end
  return root
end

local function attach(region, widget)
  local root = root_of(region)
  table.insert(root.children, widget)
  widget._parent = root
  root:wake()
end

local function unattach(widget)
  local root = widget._parent
  if root == nil then
    return
  end
  for i, child in ipairs(root.children) do
    if child == widget then
      table.remove(root.children, i)
      break
    end
  end
  widget._parent = nil
  root:wake()
end

local function remove(widget)
  local listener = widget._region_listener
  if listener ~= nil then
    listener:detach()
    widget._region_listener = nil
  end
  if widget._parent == nil then
    return false
  end
  unattach(widget)
  return true
end

-- A widget a level script placed leaves the screen with the level that placed
-- it, as its event handlers do. One a global script placed stays for the
-- session.
local function bind_scope(widget)
  if not raw.is_level_script() then
    return
  end
  trx.events.on_level_unload(function()
    remove(widget)
    widget:release()
  end)
end

api.define("ui.regions.place", {
  description = [[
Places a widget in a region.

If the region argument is a signal, the widget moves when the signal changes.]],
  params = {
    {
      name = "region",
      type = "any",
      description = "The target region, or a signal that holds one.",
    },
    {
      name = "widget",
      type = "ui.Widget",
      description = "The widget to place.",
    },
  },
  examples = {
    [[trx.ui.regions.place(trx.ui.Region.TOP_LEFT, health_bar)]],
  },
  impl = function(region, widget)
    if type(region) == "table" and region.get ~= nil then
      attach(region:get(), widget)
      widget._region_listener = region:on(function(value)
        unattach(widget)
        attach(value, widget)
      end)
    else
      attach(region, widget)
    end
    bind_scope(widget)
  end,
})

api.define("ui.regions.remove", {
  description = [[
Removes a widget from its region.

Use this for temporary widgets. Widgets owned by a level script are removed
when the level ends. Call `trx.ui.Widget:release` separately to detach their
signal listeners.]],
  params = {
    {
      name = "widget",
      type = "ui.Widget",
      description = "The widget to remove.",
    },
  },
  returns = {
    type = "boolean",
    description = "Whether the widget was in a region.",
  },
  impl = function(widget)
    return remove(widget)
  end,
})

api.define("ui.regions.fallback", {
  description = [[
Sets the widget to draw when a region has no visible content.

A region with only non-shown widgets draws nothing. A fallback can reserve that
empty place instead, for example the corner arrows shown when a bar is off
screen. Each region has at most one fallback.]],
  params = {
    {
      name = "region",
      type = "ui.Region",
      description = "The target region.",
    },
    {
      name = "widget",
      type = "ui.Widget",
      description = "The fallback widget.",
    },
  },
  impl = function(region, widget)
    fallbacks[region] = widget
    if raw.is_level_script() then
      trx.events.on_level_unload(function()
        if fallbacks[region] == widget then
          fallbacks[region] = nil
        end
        widget:release()
      end)
    end
  end,
})

-- Reserve room for each region while the scene is being built. Signal changes
-- have already invalidated any stale widget measurements by this point.
trx.events.on_ui_draw(function(region)
  local root = roots[region]
  local fallback = fallbacks[region]
  local w, h = 0, 0

  if root ~= nil then
    root._slot = nil
    if root:is_shown() then
      w, h = root:measure()
    end
  end

  -- A fallback replaces the root instead of sitting beside it, so measure it
  -- only when the root has no visible size.
  local shown = root
  if w <= 0 or h <= 0 then
    shown = nil
    if fallback ~= nil then
      fallback._slot = nil
      if fallback:is_shown() then
        w, h = fallback:measure()
        shown = fallback
      end
    end
  end

  if shown ~= nil and w > 0 and h > 0 then
    shown._slot = primitive.reserve(region, w, h)
  end
end)

-- Paint widgets after the engine has assigned boxes to their reservations.
local function paint_placed(widget)
  if widget == nil or widget._slot == nil then
    return
  end
  local x, y, w, h = primitive.slot_box(widget._slot)
  if x ~= nil then
    widget:paint(x, y, w, h)
  end
end

trx.events.on_ui_paint(function()
  for _, root in pairs(roots) do
    paint_placed(root)
  end
  for _, fallback in pairs(fallbacks) do
    paint_placed(fallback)
  end
end)
