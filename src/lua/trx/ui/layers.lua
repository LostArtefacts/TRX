local raw = trxc.ui
local raw_events = trxc.events
local raw_fade = trxc.fade
local raw_game = trxc.game
local raw_input = trxc.input
local h = require("trx.internal.helpers")
local curve_name = require("trx.ui.tween").curve_name
local track = require("trx.signal").track

require("trx.ui")
require("trx.ui.widgets")
require("trx.ui.regions")
local at_level_end = require("trx.events").at_level_end
require("trx.input")

local ui = trx.ui

local base = require("trx.ui.widgets.base")
local value_of = base.value_of
local Tween = h.class_of("ui.Tween")

-- The event the engine fires as a fade layer's move ends.
local fade_done_event = h.constants("LUA_EVENT_TYPE").UI_FADE_DONE

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
---Only the active layer, the top layer that reads input, reads the player's
---input. The layers below it read nothing until it closes. While a layer that
---reads input is open during play, the layers take the devices from the game
---with `trx.input.grab`: Lara does not move, and the game's own keys, such as
---the pause key, do nothing. The level keeps running.
---
---A layer that a level script pushes closes when the level ends.
---@class (partial,exact) trx.ui.layers
ui.layers = h.namespace("ui.layers")

---@class (partial) trx.ui.layers
local layers = trx.ui.layers

-------------------------------------------------------------------------------
-- Input
-------------------------------------------------------------------------------

---The player's input, as the active layer reads it.
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

-- The grab that the layers hold while a layer reads input during play.
---@type trx.input.Grab?
local grab = nil

-- While the layers hold the devices, the game reads nothing, and the engine
-- keeps what the devices say for the layers instead.
local function is_held(role)
  if grab ~= nil then
    return raw_input.grab_is_held(role)
  end
  return trx.input.is_held(role)
end

local function is_pressed(role)
  if grab ~= nil then
    return raw_input.grab_is_pressed(role)
  end
  return trx.input.is_pressed(role)
end

---Returns whether a role became active this tick, and uses the press up.
---@param role trx.input.Role The role.
---@return boolean # Whether it was pressed.
function Keys:pressed(role)
  if self._used[role] or not is_pressed(role) then
    return false
  end
  self._used[role] = true
  return true
end

---Returns whether a role is active. Does not use it up.
---@param role trx.input.Role The role.
---@return boolean # Whether it is held.
function Keys:held(role)
  return is_held(role)
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
    entry.count = is_held(role) and entry.count + 1 or 0
  elseif entry.count == 0 and is_held(role) then
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

local function active_layer()
  for i = #stack, 1, -1 do
    if stack[i].modal then
      return stack[i]
    end
  end
  return nil
end

-- Takes the devices from the game while a layer reads input during play, and
-- gives them back otherwise.
local function update_grab()
  local top = active_layer()
  local wanted = top ~= nil and raw_game.is_playing()
  if wanted and grab == nil then
    grab = trx.input.grab()
  elseif not wanted and grab ~= nil then
    grab:release()
    grab = nil
  end
end

local function remove(layer)
  local i = index_of(layer)
  if i == nil then
    return nil
  end
  table.remove(stack, i)
  update_grab()
  layer._open = false
  -- Holds off only the presses that closed the layer. Holding off every
  -- press would also stop a held menu key from repeating.
  for role in pairs(layer._keys._used) do
    trx.input.hold_off(role)
  end
  if layer._unload ~= nil then
    layer._unload:detach()
    layer._unload = nil
  end
  for _, listener in pairs(layer._follows) do
    listener:detach()
  end
  layer._follows = {}
  base.unhold(layer.root, layer)
  layer.root:release()
  return h.failure(layer.on_close, layer)
end

-- Removes a layer that raised an error. Returns that error, followed by the
-- error that the close callback raised, if any.
local function remove_failed(layer, err)
  local close_err = remove(layer)
  if close_err == nil then
    return err
  end
  return tostring(err) .. "\n" .. tostring(close_err)
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
---@field private place? trx.ui.Placement|fun(w: number, h: number): number, number
---@field private modal boolean
---@field private on_input? fun(layer: trx.ui.StackLayer, keys: trx.ui.LayerKeys)
---@field private on_close? fun(layer: trx.ui.StackLayer)
---@field private _open boolean
---@field private _fresh boolean
---@field private _keys trx.ui.LayerKeys
---@field private _unload? trx.events.Listener
---@field private _spot? { x: number, y: number, w: number, h: number, moving: boolean }
---@field private _follows table<trx.signal.Signal, trx.signal.Listener>
local Layer = h.class("ui.StackLayer", {
  fields = {
    is_open = {
      get = function(self)
        return self._open
      end,
    },
  },
})

---Removes the layer from the stack, and releases its widgets. Does nothing if
---the layer is already closed.
---@return boolean # Whether the layer was open.
function Layer:close()
  if not self._open then
    return false
  end
  return close(self)
end

---Replaces the widget tree that the layer draws, and releases the old one.
---@param root trx.ui.Widget The new tree.
---@param place? trx.ui.Placement|function Where the new tree goes, as `trx.ui.layers.push` takes it. The tree keeps the old placement where this is `nil`.
function Layer:set_root(root, place)
  base.hold(root, self, 2)
  local old = self.root
  -- The root and the placement are plain names, so the class takes the
  -- writes raw.
  rawset(self, "root", root)
  if place ~= nil then
    rawset(self, "place", place)
  end
  self._spot = nil
  if old ~= root then
    base.unhold(old, self)
    old:release()
  end
end

---Where a layer puts its tree within an area of the canvas. The tree takes a
---share of the room that it leaves free on each axis, and starts from the top
---left of the area where it is too large for it.
---
---```lua
---trx.ui.layers.push({ root = dialog, place = { y = 1, margin = 50 } })
---```
---@trx.record
---@class trx.ui.Placement
---@field x? any How far across the free width the tree stands, from `0` against the left edge to `1` against the right edge, or a signal that holds it. `0.5` by default.
---@field y? any How far down the free height the tree stands, from `0` against the top edge to `1` against the bottom edge, or a signal that holds it. `0.5` by default.
---@field margin? any The room kept clear inside each edge of the area, in canvas units, or a signal that holds it. `0` by default.
---@field canvas? any Whether the area is the whole canvas, or a signal that holds that value. By default, the area is `trx.ui.safe_area_for` the width of the tree, as the engine centers its own boxes.

---@class (exact) trx.ui.layers.push.settings
---@field root trx.ui.Widget The widget tree to draw.
---@field place? trx.ui.Placement|function Where the tree goes. A function receives the width and the height that the tree measures, and returns the top left corner of the tree, in canvas units. The layer works the corner out again only when the tree's size, the room the interface lays out in, or a signal that the placement reads changes, and on every frame while the placement reads a `trx.ui.Tween`. The tree is centered in `trx.ui.safe_area_for` its width by default.
---@field modal? boolean Whether the layer reads input. A layer that reads input takes it from the layers below, and during play from the game. By default, a layer reads input when it has `on_input`. Set it to `true` for a layer that holds the input back without reading any. <!--noref: on_input-->
---@field on_input? function Runs once a tick while the layer is the active layer. It receives the layer and a `trx.ui.LayerKeys`. An error closes the layer.
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
    place = settings.place,
    modal = settings.modal == true
      or settings.modal == nil and settings.on_input ~= nil,
    on_input = settings.on_input,
    on_close = settings.on_close,
    _open = true,
    _fresh = true,
    _keys = setmetatable({ _used = {}, _held = {} }, Keys),
    _follows = {},
  }, Layer)
  base.hold(settings.root, layer, 2)
  stack[#stack + 1] = layer
  update_grab()
  layer._unload = at_level_end(function()
    local err = remove(layer)
    if err ~= nil then
      error(err, 0)
    end
  end)
  return layer
end

-------------------------------------------------------------------------------
-- Fade layers
-------------------------------------------------------------------------------

-- The open fade layers, keyed by the id the engine knows each by.
---@type table<integer, trx.ui.FadeLayer>
local fades = {}

-- Closes a fade layer. Returns the error that its on_close raised, or nil, so
-- that the close completes before the error is raised.
local function finish_fade(layer)
  local id = layer._id
  if fades[id] ~= layer then
    return nil
  end
  fades[id] = nil
  raw_fade.close(id)
  if layer._unload ~= nil then
    layer._unload:detach()
    layer._unload = nil
  end
  return h.failure(layer.on_close, layer)
end

---A layer that the engine draws over its interface: one colour, or the frame as
---it was when the layer was made. Its opacity moves on a curve that the engine
---runs, so nothing is painted by the script. Fade layers stack in the order
---they were made, above every engine screen and below the console.
---
---A backdrop layer is drawn under the interface instead, as the backdrop that
---the game draws behind its screens. Its opacity says how far the backdrop has
---come in: a picture comes in from black, and any other style from the frame
---behind it.
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
        return fades[self._id] == self
      end,
    },
    opacity = {
      get = function(self)
        return raw_fade.opacity(self._id)
      end,
    },
  },
})

---Stops drawing the layer. Does nothing if the layer is already closed.
---@return boolean # Whether the layer was open.
function FadeLayer:close()
  if fades[self._id] ~= self then
    return false
  end
  local err = finish_fade(self)
  if err ~= nil then
    error(err, 0)
  end
  return true
end

---@class (exact) trx.ui.FadeLayer.to.options
---@field curve? function The curve of the move, one of `trx.ui.ease`. `trx.ui.ease.linear` by default.
---@field close? boolean Whether the layer closes once the move ends. `false` by default.
---@field on_done? function Runs once when the move ends, and not if another move or a close cuts it short. A move of no ticks made in a tick handler ends within that tick. It receives the layer.

---Moves the opacity to a target. The move takes the ticks it is given whatever
---the fade settings are.
---@param target number The opacity to go to, from `0` to `1`.
---@param ticks integer How many ticks the move takes.
---@param options? trx.ui.FadeLayer.to.options How the move goes, and what happens when it ends.
---@return trx.ui.FadeLayer # The same layer.
function FadeLayer:to(target, ticks, options)
  options = options or {}
  local name = curve_name(options.curve)
  if name == nil then
    error("the curve is not one of trx.ui.ease", 2)
  end
  self._on_done = options.on_done
  raw_fade.to(self._id, target, ticks, name, options.close == true)
  return self
end

---@class (exact) trx.ui.layers.fade.settings
---@field fill? trx.math.Color The colour that the layer holds.
---@field snapshot? boolean Whether the layer holds the frame on screen now, in place of a colour.
---@field backdrop? string The backdrop style that the layer holds, in place of a colour, by the names that the `ui.stats_background_style` setting takes. <!--noref: ui.stats_background_style-->
---@field path? string The picture of an `image` backdrop, as `trx.ui.primitive.background` takes it. <!--noref: image-->
---@field opacity? number How opaque the layer starts, from `0` to `1`. `1` by default.
---@field on_close? function Runs once when the layer closes, for any reason. It receives the layer.
---@field holds_screen? boolean Whether the engine waits to start its next screen while the layer is open. `false` by default.

---Puts up a fade layer, on top of the others drawn where it is. A level
---script's layer closes when the level ends.
---
---```lua
---local black = trx.ui.layers.fade({ fill = trx.math.color("000000"), opacity = 0 })
---black:to(1, 15, { close = true })
---```
---@param settings trx.ui.layers.fade.settings The layer settings.
---@return trx.ui.FadeLayer # The layer.
function layers.fade(settings)
  local opacity = settings.opacity or 1
  local id
  if settings.snapshot then
    id = raw_fade.push_snapshot(opacity)
  elseif settings.backdrop ~= nil then
    id = raw_fade.push_backdrop(settings.backdrop, settings.path, opacity)
  elseif settings.fill ~= nil then
    local fill = settings.fill
    id = raw_fade.push_fill(fill.r, fill.g, fill.b, opacity)
  else
    error("a fade layer needs something to hold", 2)
  end
  if id == 0 then
    error("too many fade layers", 2)
  end
  if settings.holds_screen then
    raw_fade.hold_screen(id)
  end
  local layer =
    setmetatable({ on_close = settings.on_close, _id = id }, FadeLayer)
  fades[id] = layer
  layer._unload = at_level_end(function()
    layer:close()
  end)
  return layer
end

---Returns the active layer: the top layer that reads input.
---@return trx.ui.StackLayer? # The layer, or `nil` if no layer reads input.
---@type fun(): trx.ui.StackLayer?
layers.active = active_layer

---Returns how many layers are on the stack.
---@return integer # The number of layers.
function layers.count()
  return #stack
end

-------------------------------------------------------------------------------
-- Drawing and input
-------------------------------------------------------------------------------

local NO_PLACEMENT = {}

-- The top left corner of a tree of the given size, as a placement table puts
-- it.
local function spot_in(placement, w, h)
  local area = value_of(placement.canvas) and trx.ui.canvas
    or trx.ui.safe_area_for(w)
  local margin = value_of(placement.margin) or 0
  local function along(start, room, size, share)
    local slack = room - 2 * margin - size
    return start + margin + (slack > 0 and slack * share or 0)
  end
  return along(area.x, area.width, w, value_of(placement.x) or 0.5),
    along(area.y, area.height, h, value_of(placement.y) or 0.5)
end

local function spot_of(place, w, h)
  if type(place) == "function" then
    return place(w, h)
  end
  return spot_in(place or NO_PLACEMENT, w, h)
end

-- Forgets the corner of the tree when the signal changes. A signal followed
-- already adds nothing.
local function follow(layer, signal)
  local follows = layer._follows
  if follows[signal] == nil then
    follows[signal] = signal:on(function()
      layer._spot = nil
    end)
  end
end

local function box_of(layer)
  local w, h = layer.root:measure()
  local spot = layer._spot
  if spot == nil or spot.moving or spot.w ~= w or spot.h ~= h then
    follow(layer, ui.signals.layout)
    local moving = false
    local x, y = track(function(read)
      if getmetatable(read) == Tween then
        moving = true
      else
        follow(layer, read)
      end
    end, spot_of, layer.place, w, h)
    spot = { x = x, y = y, w = w, h = h, moving = moving }
    layer._spot = spot
  end
  return spot.x, spot.y, w, h
end

local function paint(layer)
  layer.root:paint(box_of(layer))
end

-- The layers painted this frame, copied into one table kept from frame to
-- frame, so that a layer that closes as it paints does not move the rest. A
-- layer that closes before its turn, such as with the screen of a layer that
-- failed, is skipped.
local painting = {}

trx.ui.on_paint(function()
  local count = #stack
  for i = 1, math.max(count, #painting) do
    painting[i] = stack[i]
  end
  local failed = nil
  for i = 1, count do
    local layer = painting[i]
    if layer._open then
      local err = h.failure(paint, layer)
      if err ~= nil then
        failed = failed or remove_failed(layer, err)
      end
    end
  end
  if failed ~= nil then
    error(failed, 0)
  end
end)

trx.events.on_tick(function()
  tick = tick + 1
  update_grab()
  local layer = active_layer()
  if layer == nil then
    return
  end
  if layer._fresh then
    layer._fresh = false
    return
  end
  if layer.on_input == nil then
    return
  end
  local used = layer._keys._used
  for role in pairs(used) do
    used[role] = nil
  end
  local err = h.failure(layer.on_input, layer, layer._keys)
  if err ~= nil then
    error(remove_failed(layer, err), 0)
  end
end)

-- The input module lets go of every grab as a level ends.
trx.events.on_level_unload(function()
  grab = nil
end)

-- The engine reports each move that ends, once, after closing the layers told
-- to close with it.
raw_events.attach(fade_done_event, function(id)
  local layer = fades[id]
  if layer == nil then
    return
  end
  local on_done = layer._on_done
  layer._on_done = nil
  local failed = nil
  if not raw_fade.is_open(id) then
    failed = finish_fade(layer)
  end
  local err = h.failure(on_done, layer)
  failed = failed or err
  if failed ~= nil then
    error(failed, 0)
  end
end)
