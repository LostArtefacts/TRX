local raw = trxc.ui
local api = trx.api

require("trx.ui")
require("trx.ui.primitive")
require("trx.ui.widgets")
require("trx.ui.regions")
require("trx.events")
require("trx.input")

local primitive = trx.ui.primitive

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

local DEPTH_STEP = 256
local MAX_LAYERS = 15

local stack = {}

local tick = 0

api.namespace("ui.layers", {
  description = [[
Draws screens of widgets over the rest of the interface.

A layer holds one widget tree, such as a menu or a question. Layers are kept in
a stack. Each layer draws over the layers below it and over the widgets placed
with `trx.ui.regions.place`. The engine interface still draws over all of them.

Only the top layer that takes input reads the player's input. The layers below
it read nothing until it closes.

A layer that a level script pushes closes when the level ends.]],
})

-------------------------------------------------------------------------------
-- Input
-------------------------------------------------------------------------------

local Keys = api.type("ui.LayerKeys", {
  description = [[
The player's input, as the top layer reads it.

A press that a layer reads is used up. It does not reach other code, and it
does not reach the layer below when this one closes.]],
  methods = {
    pressed = {
      description = [[
Returns whether a role became active this tick, and uses the press up.]],
      params = {
        { name = "role", type = "input.Role", description = "The role." },
      },
      returns = { type = "boolean", description = "Whether it was pressed." },
      impl = function(self, role)
        if self._used[role] or not trx.input.is_pressed(role) then
          return false
        end
        self._used[role] = true
        trx.input.hold_off(role)
        return true
      end,
    },
    held = {
      description = "Returns whether a role is active. Does not use it up.",
      params = {
        { name = "role", type = "input.Role", description = "The role." },
      },
      returns = { type = "boolean", description = "Whether it is held." },
      impl = function(_, role)
        return trx.input.is_held(role)
      end,
    },
    held_for = {
      description = [[
Returns for how many ticks a role has been held. A tick in which the layer did
not ask counts as a release.]],
      params = {
        { name = "role", type = "input.Role", description = "The role." },
      },
      returns = { type = "integer", description = "The number of ticks." },
      impl = function(self, role)
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
      end,
    },
  },
})

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

local Layer = api.type("ui.StackLayer", {
  description = "One screen of widgets on the stack.",
  fields = {
    is_open = {
      type = "boolean",
      description = "Whether the layer is still on the stack.",
      get = function(self)
        return rawget(self, "_open")
      end,
    },
  },
  methods = {
    close = {
      description = [[
Removes the layer from the stack, and releases its widgets. Does nothing if the
layer is already closed.]],
      returns = {
        type = "boolean",
        description = "Whether the layer was open.",
      },
      impl = function(self)
        if not rawget(self, "_open") then
          return false
        end
        return close(self)
      end,
    },
    set_root = {
      description = [[
Replaces the widget tree that the layer draws, and releases the old one.]],
      params = {
        { name = "root", type = "ui.Widget", description = "The new tree." },
      },
      impl = function(self, root)
        local old = self.root
        rawset(self, "root", root)
        rawset(self, "_slot", nil)
        if old ~= root then
          old:release()
        end
      end,
    },
    is_top = {
      description = "Returns whether the layer is the one that reads input.",
      returns = { type = "boolean", description = "Whether it is on top." },
      impl = function(self)
        return top_modal() == self
      end,
    },
  },
})

api.define("ui.layers.push", {
  description = [[
Puts a layer on top of the stack.

The layer reads no input on the tick it is pushed, because the press that
opened it is often still active.]],
  params = {
    {
      name = "settings",
      type = "table",
      description = "The layer settings.",
      fields = {
        {
          name = "root",
          type = "ui.Widget",
          description = "The widget tree to draw.",
        },
        {
          name = "region",
          type = "ui.Region",
          optional = true,
          description = [[
The region that the tree takes room in. The tree then stacks with the other
widgets in that region. Without a region or a place, the tree is centered in
`trx.ui.safe_area`.]],
        },
        {
          name = "place",
          type = "function",
          optional = true,
          description = [[
Returns the top left corner of the tree, in canvas units. It receives the
width and the height that the tree measures.]],
        },
        {
          name = "modal",
          type = "boolean",
          optional = true,
          description = "Whether the layer reads input. `true` by default.",
        },
        {
          name = "on_input",
          type = "function",
          optional = true,
          description = [[
Runs once a tick while the layer is the top layer that reads input. It
receives the layer and a `trx.ui.LayerKeys`. An error closes the layer.]],
        },
        {
          name = "on_close",
          type = "function",
          optional = true,
          description = [[
Runs once when the layer closes, for any reason. It receives the layer.]],
        },
      },
    },
  },
  returns = { type = "ui.StackLayer", description = "The pushed layer." },
  examples = {
    [[local layer = trx.ui.layers.push({
  root = trx.ui.widgets.Label({ text = "Paused" }),
  on_input = function(layer, keys)
    if keys:pressed(trx.input.Role.MENU_BACK) then
      layer:close()
    end
  end,
})]],
  },
  impl = function(settings)
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
    if trxc.events.is_level_script() then
      rawset(
        layer,
        "_unload",
        trx.events.on_level_unload(function()
          remove(layer)
        end)
      )
    end
    return layer
  end,
})

api.define("ui.layers.top", {
  description = "Returns the top layer that reads input.",
  returns = {
    type = "ui.StackLayer",
    nullable = true,
    description = "The layer, or `nil` if no layer reads input.",
  },
  impl = top_modal,
})

api.define("ui.layers.count", {
  description = "Returns how many layers are on the stack.",
  returns = { type = "integer", description = "The number of layers." },
  impl = function()
    return #stack
  end,
})

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
  local safe = trx.ui.safe_area
  return safe.x + (safe.width - w) / 2, safe.y + (safe.height - h) / 2, w, h
end

local function paint(layer)
  local x, y, w, h = box_of(layer)
  if x ~= nil then
    layer.root:paint(x, y, w, h)
  end
end

trx.events.on_ui_draw(function(region)
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

trx.events.on_ui_paint(function()
  local failed = nil
  local snapshot = { table.unpack(stack) }
  for i, layer in ipairs(snapshot) do
    raw.push_depth(-i * DEPTH_STEP)
    local ok, err = pcall(paint, layer)
    raw.pop_depth()
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
