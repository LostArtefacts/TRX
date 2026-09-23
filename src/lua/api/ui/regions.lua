local raw = trxc.events
local api = trx.api

require("trx.ui")
require("trx.ui.primitive")
require("trx.ui.widgets")
require("trx.events")

local primitive = trx.ui.primitive

-- Stores a root widget and reservation for each layer and region.
local roots = { [trx.ui.Layer.UNDER] = {}, [trx.ui.Layer.OVER] = {} }

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

local function root_of(layer, region)
  local root = roots[layer][region]
  if root == nil then
    root = trx.ui.widgets.Stack({
      children = {},
      orientation = trx.ui.Orientation.VERTICAL,
      spacing = 3,
      align = ALIGN_OF[region] or trx.ui.HAlign.LEFT,
    })
    roots[layer][region] = root
  end
  return root
end

local function attach(region, widget, layer)
  local root = root_of(layer, region)
  table.insert(root.children, widget)
  widget._parent = root
  widget._layer = layer
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
  widget._layer = nil
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

The layer decides whether the widget is covered by the engine interface or
covers it. A widget is under it unless the call says otherwise. Each layer
keeps room of its own in the region, so widgets on the two layers stack rather
than sit on top of each other.

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
    {
      name = "layer",
      type = "ui.Layer",
      optional = true,
      description = "Which layer to draw on. Defaults to `trx.ui.Layer.UNDER`.",
    },
  },
  examples = {
    [[trx.ui.regions.place(trx.ui.Region.TOP_LEFT, health_bar)]],
    [[trx.ui.regions.place(
  trx.ui.Region.BOTTOM_LEFT,
  console,
  trx.ui.Layer.OVER
)]],
  },
  impl = function(region, widget, layer)
    layer = layer or trx.ui.Layer.UNDER
    if type(region) == "table" and region.get ~= nil then
      attach(region:get(), widget, layer)
      widget._region_listener = region:on(function(value)
        unattach(widget)
        attach(value, widget, layer)
      end)
    else
      attach(region, widget, layer)
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

-- Reserves room for one layer's root in a region. The fallback belongs to the
-- region rather than to a layer, so only the under layer offers it.
local function reserve_root(layer, region, fallback)
  local root = roots[layer][region]
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
end

-- Reserve room for each region while the scene is being built. Signal changes
-- have already invalidated any stale widget measurements by this point. Each
-- layer reserves separately, so the two never land on the same place.
trx.events.on_ui_draw(function(region)
  reserve_root(trx.ui.Layer.UNDER, region, fallbacks[region])
  reserve_root(trx.ui.Layer.OVER, region, nil)
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
  for _, root in pairs(roots[trx.ui.Layer.UNDER]) do
    paint_placed(root)
  end
  for _, fallback in pairs(fallbacks) do
    paint_placed(fallback)
  end
end)

trx.events.on_ui_paint_over(function()
  for _, root in pairs(roots[trx.ui.Layer.OVER]) do
    paint_placed(root)
  end
end)
