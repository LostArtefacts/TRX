local raw = trxc.ui
local raw_events = trxc.events
local raw_fade = trxc.fade
local h = require("trx.internal.helpers")
local curve_name = require("trx.ui.tween").curve_name

require("trx.ui")
require("trx.ui.primitive")
require("trx.ui.widgets")
require("trx.ui.regions")
require("trx.events")
require("trx.input")

local ui = trx.ui

local primitive = trx.ui.primitive
local f32 = require("trx.ui.widgets.base").f32

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

-- The event the engine fires as a fade layer's move ends.
local fade_done_event = 0
for _, constant in ipairs(trxc.enum.values("LUA_EVENT_TYPE")) do
  if constant.name == "UI_FADE_DONE" then
    fade_done_event = constant.value
  end
end

local MAX_LAYERS = 15

local stack = {}

local tick = 0

---@class (partial) trx.ui
---@field layers trx.ui.layers

---Draws screens of widgets over the rest of the interface.
---
---A layer holds one widget tree, such as a menu or a question. Layers are kept
---in a stack. Each layer draws over the layers below it and over the widgets
---placed with `trx.ui.regions.place`. The engine interface still draws over
---all of them.
---
---Only the top layer that takes input reads the player's input. The layers
---below it read nothing until it closes.
---
---A layer that a level script pushes closes when the level ends.
---@class (partial,exact) trx.ui.layers
ui.layers = h.namespace("ui.layers")

---@class (partial) trx.ui.layers
local layers = trx.ui.layers

-------------------------------------------------------------------------------
-- Input
-------------------------------------------------------------------------------

---The player's input, as the top layer reads it.
---
---A layer reads each press as pressed once per tick. A menu key that the
---player holds keeps reading as pressed at the rate that the game's own menus
---repeat it. The presses that a layer read on the tick that it closes do not
---reach the layer below, because they stay inactive until the player releases
---them.
---@class (exact) trx.ui.LayerKeys
---@field private _used table<trx.input.Role, boolean>
---@field private _held table<trx.input.Role, { count: integer, tick: integer }>
local Keys = h.class("ui.LayerKeys")

---Returns whether a role became active this tick, and uses the press up.
---@param role trx.input.Role The role.
---@return boolean # Whether it was pressed.
function Keys:pressed(role)
  if self._used[role] or not trx.input.is_pressed(role) then
    return false
  end
  self._used[role] = true
  return true
end

---Returns whether a role is active. Does not use it up.
---@param role trx.input.Role The role.
---@return boolean # Whether it is held.
function Keys:held(role)
  return trx.input.is_held(role)
end

---Returns for how many ticks a role has been held. A tick in which the layer
---did not ask counts as a release.
---@param role trx.input.Role The role.
---@return integer # The number of ticks.
function Keys:held_for(role)
  local entry = self._held[role]
  if entry == nil or entry.tick < tick - 1 then
    entry = { count = 0, tick = tick }
    self._held[role] = entry
  end
  if entry.tick ~= tick then
    entry.tick = tick
    entry.count = trx.input.is_held(role) and entry.count + 1 or 0
  elseif entry.count == 0 and trx.input.is_held(role) then
    entry.count = 1
  end
  return entry.count
end

-------------------------------------------------------------------------------
-- The layer
-------------------------------------------------------------------------------

local function index_of(layer)
  for i, other in ipairs(stack) do
    if other == layer then
      return i
    end
  end
  return nil
end

local function top_modal()
  for i = #stack, 1, -1 do
    if stack[i].modal then
      return stack[i]
    end
  end
  return nil
end

local function remove(layer)
  local i = index_of(layer)
  if i == nil then
    return nil
  end
  table.remove(stack, i)
  rawset(layer, "_open", false)
  -- Holds off only the presses that closed the layer. Holding off every
  -- press would also stop a held menu key from repeating.
  for role in pairs(layer._keys._used) do
    trx.input.hold_off(role)
  end
  if layer._unload ~= nil then
    layer._unload:detach()
    rawset(layer, "_unload", nil)
  end
  layer.root:release()
  if layer.on_close ~= nil then
    local ok, err = pcall(layer.on_close, layer)
    if not ok then
      return err
    end
  end
  return nil
end

local function close(layer)
  local err = remove(layer)
  if err ~= nil then
    error(err, 0)
  end
  return true
end

---One screen of widgets on the stack.
---@class (exact) trx.ui.StackLayer
---@trx.readonly is_open
---@field is_open boolean Whether the layer is still on the stack.
---@field private root trx.ui.Widget
---@field private region? trx.ui.Region
---@field private place? fun(w: number, h: number): number, number
---@field private modal boolean
---@field private on_input? fun(layer: trx.ui.StackLayer, keys: trx.ui.LayerKeys)
---@field private on_close? fun(layer: trx.ui.StackLayer)
---@field private _open boolean
---@field private _fresh boolean
---@field private _keys trx.ui.LayerKeys
---@field private _unload? trx.events.Listener
---@field private _slot? integer
local Layer = h.class("ui.StackLayer", {
  fields = {
    is_open = {
      get = function(self)
        return rawget(self, "_open")
      end,
    },
  },
})

---Removes the layer from the stack, and releases its widgets. Does nothing if
---the layer is already closed.
---@return boolean # Whether the layer was open.
function Layer:close()
  if not rawget(self, "_open") then
    return false
  end
  return close(self)
end

---Replaces the widget tree that the layer draws, and releases the old one.
---@param root trx.ui.Widget The new tree.
function Layer:set_root(root)
  local old = self.root
  rawset(self, "root", root)
  rawset(self, "_slot", nil)
  if old ~= root then
    old:release()
  end
end

---Returns whether the layer is the one that reads input.
---@return boolean # Whether it is on top.
function Layer:is_top()
  return top_modal() == self
end

---@class (exact) trx.ui.layers.push.settings
---@field root trx.ui.Widget The widget tree to draw.
---@field region? trx.ui.Region The region that the tree takes room in. The tree then stacks with the other widgets in that region. Without a region or a place, the tree is centered in `trx.ui.safe_area_for` its width, as the engine centers its own boxes, and starts from the top left of that area where it is too large for it.
---@field place? function Returns the top left corner of the tree, in canvas units. It receives the width and the height that the tree measures.
---@field modal? boolean Whether the layer reads input. `true` by default.
---@field on_input? function Runs once a tick while the layer is the top layer that reads input. It receives the layer and a `trx.ui.LayerKeys`. An error closes the layer.
---@field on_close? function Runs once when the layer closes, for any reason. It receives the layer.

---Puts a layer on top of the stack.
---
---The layer reads no input on the tick it is pushed, because the press that
---opened it is often still active.
---
---```lua
---local layer = trx.ui.layers.push({
---  root = trx.ui.widgets.Label({ text = "Paused" }),
---  on_input = function(layer, keys)
---    if keys:pressed(trx.input.Role.MENU_BACK) then
---      layer:close()
---    end
---  end,
---})
---```
---@param settings trx.ui.layers.push.settings The layer settings.
---@return trx.ui.StackLayer # The pushed layer.
function layers.push(settings)
  if #stack >= MAX_LAYERS then
    error("too many layers", 2)
  end
  local layer = setmetatable({
    root = settings.root,
    region = settings.region,
    place = settings.place,
    modal = settings.modal ~= false,
    on_input = settings.on_input,
    on_close = settings.on_close,
    _open = true,
    _fresh = true,
    _keys = setmetatable({ _used = {}, _held = {} }, Keys),
  }, Layer)
  stack[#stack + 1] = layer
  if raw_events.is_level_script() then
    rawset(
      layer,
      "_unload",
      trx.events.on_level_unload(function()
        remove(layer)
      end)
    )
  end
  return layer
end

-------------------------------------------------------------------------------
-- Fade layers
-------------------------------------------------------------------------------

-- The open fade layers, keyed by the id the engine knows each by.
---@type table<integer, trx.ui.FadeLayer>
local fades = {}

local function finish_fade(layer)
  local id = rawget(layer, "_id")
  if fades[id] ~= layer then
    return
  end
  fades[id] = nil
  raw_fade.close(id)
  if layer._unload ~= nil then
    layer._unload:detach()
    rawset(layer, "_unload", nil)
  end
  if layer.on_close ~= nil then
    layer.on_close(layer)
  end
end

---A layer that the engine draws over its interface: one colour, or the frame as
---it was when the layer was made. Its opacity moves on a curve that the engine
---runs, so nothing is painted by the script. Fade layers stack in the order
---they were made, above every engine screen and below the console.
---@class (exact) trx.ui.FadeLayer
---@trx.readonly is_open, opacity
---@field is_open boolean Whether the layer is still drawn.
---@field opacity number How opaque the layer is now, from `0` to `1`.
---@field private on_close? fun(layer: trx.ui.FadeLayer)
---@field private _id integer
---@field private _on_done? fun(layer: trx.ui.FadeLayer)
---@field private _unload? trx.events.Listener
local FadeLayer = h.class("ui.FadeLayer", {
  fields = {
    is_open = {
      get = function(self)
        return fades[rawget(self, "_id")] == self
      end,
    },
    opacity = {
      get = function(self)
        return raw_fade.opacity(rawget(self, "_id"))
      end,
    },
  },
})

---Stops drawing the layer. Does nothing if the layer is already closed.
---@return boolean # Whether the layer was open.
function FadeLayer:close()
  if fades[rawget(self, "_id")] ~= self then
    return false
  end
  finish_fade(self)
  return true
end

---@class (exact) trx.ui.FadeLayer.to.options
---@field close? boolean Whether the layer closes once the move ends. `false` by default.
---@field on_done? function Runs once when the move ends, and not if another move or a close cuts it short. A move of no ticks made in a tick handler ends within that tick. It receives the layer.

---Moves the opacity to a target. The move takes the ticks it is given whatever
---the fade settings are.
---@param target number The opacity to go to, from `0` to `1`.
---@param ticks integer How many ticks the move takes.
---@param curve? function The curve of the move, one of `trx.ui.ease`. `trx.ui.ease.linear` by default.
---@param options? trx.ui.FadeLayer.to.options What happens when the move ends.
---@return trx.ui.FadeLayer # The same layer.
function FadeLayer:to(target, ticks, curve, options)
  local name = curve_name(curve)
  if name == nil then
    error("the curve is not one of trx.ui.ease", 2)
  end
  options = options or {}
  rawset(self, "_on_done", options.on_done)
  raw_fade.to(rawget(self, "_id"), target, ticks, name, options.close == true)
  return self
end

---@class (exact) trx.ui.layers.fade.settings
---@field fill? trx.math.Color The colour that the layer holds.
---@field snapshot? boolean Whether the layer holds the frame on screen now, in place of a colour.
---@field opacity? number How opaque the layer starts, from `0` to `1`. `1` by default.
---@field on_close? function Runs once when the layer closes, for any reason. It receives the layer.

---Puts up a fade layer, on top of the others drawn where it is. A level
---script's layer closes when the level ends.
---
---```lua
---local black = trx.ui.layers.fade({ fill = trx.math.color("000000"), opacity = 0 })
---black:to(1, 15, nil, { close = true })
---```
---@param settings trx.ui.layers.fade.settings The layer settings.
---@return trx.ui.FadeLayer # The layer.
function layers.fade(settings)
  local opacity = settings.opacity or 1
  local id
  if settings.snapshot then
    id = raw_fade.push_snapshot(opacity)
  elseif settings.fill ~= nil then
    local fill = settings.fill
    id = raw_fade.push_fill(fill.r, fill.g, fill.b, opacity)
  else
    error("a fade layer needs something to hold", 2)
  end
  if id == 0 then
    error("too many fade layers", 2)
  end
  local layer =
    setmetatable({ on_close = settings.on_close, _id = id }, FadeLayer)
  fades[id] = layer
  if raw_events.is_level_script() then
    rawset(
      layer,
      "_unload",
      trx.events.on_level_unload(function()
        layer:close()
      end)
    )
  end
  return layer
end

---Returns the top layer that reads input.
---@return trx.ui.StackLayer? # The layer, or `nil` if no layer reads input.
---@type fun(): trx.ui.StackLayer?
layers.top = top_modal

---Returns how many layers are on the stack.
---@return integer # The number of layers.
function layers.count()
  return #stack
end

-------------------------------------------------------------------------------
-- Drawing and input
-------------------------------------------------------------------------------

local function box_of(layer)
  if layer.region ~= nil then
    if layer._slot == nil then
      return nil
    end
    return primitive.slot_box(layer._slot)
  end
  local w, h = layer.root:measure()
  if layer.place ~= nil then
    local x, y = layer.place(w, h)
    return x, y, w, h
  end
  -- Centered in single precision, as the engine places its own boxes, and
  -- from the top left where the tree is too large for the room.
  local safe = trx.ui.safe_area_for(w)
  local function offset(slack)
    return slack >= 0 and f32(slack * 0.5) or 0
  end
  return f32(safe.x + offset(f32(safe.width - w))),
    f32(safe.y + offset(f32(safe.height - h))),
    w,
    h
end

local function paint(layer)
  local x, y, w, h = box_of(layer)
  if x ~= nil then
    layer.root:paint(x, y, w, h)
  end
end

trx.ui.on_draw(function(region)
  for _, layer in ipairs(stack) do
    if layer.region == region then
      rawset(layer, "_slot", nil)
      if layer.root:is_shown() then
        local w, h = layer.root:measure()
        if w > 0 and h > 0 then
          rawset(layer, "_slot", primitive.reserve(region, w, h))
        end
      end
    end
  end
end)

trx.ui.on_paint(function()
  local failed = nil
  local snapshot = { table.unpack(stack) }
  for _, layer in ipairs(snapshot) do
    local ok, err = pcall(paint, layer)
    if not ok then
      remove(layer)
      failed = failed or err
    end
  end
  if failed ~= nil then
    error(failed, 0)
  end
end)

trx.events.on_tick(function()
  tick = tick + 1
  local layer = top_modal()
  if layer == nil then
    return
  end
  if layer._fresh then
    rawset(layer, "_fresh", false)
    return
  end
  if layer.on_input == nil then
    return
  end
  layer._keys._used = {}
  local ok, err = pcall(layer.on_input, layer, layer._keys)
  if not ok then
    remove(layer)
    error(err, 0)
  end
end)

-- The engine reports each move that ends, once, after closing the layers told
-- to close with it.
raw_events.attach(fade_done_event, function(id)
  local layer = fades[id]
  if layer == nil then
    return
  end
  local on_done = rawget(layer, "_on_done")
  rawset(layer, "_on_done", nil)
  if not raw_fade.is_open(id) then
    finish_fade(layer)
  end
  if on_done ~= nil then
    on_done(layer)
  end
end)
