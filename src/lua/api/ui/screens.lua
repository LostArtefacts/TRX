local raw = trxc.screens
local raw_events = trxc.events
local raw_enum = trxc.enum
local api = trx.api

require("trx.ui")
require("trx.ui.layers")
require("trx.events")

-- The event types and the choices are the engine's, read back as any other
-- enum is. Neither is public: a script defines a screen and ends it through
-- its context, and never names either.
local function constants(backing)
  local result = {}
  for _, constant in ipairs(raw_enum.values(backing)) do
    result[constant.name] = constant.value
  end
  return result
end

local events = constants("LUA_EVENT_TYPE")
local choices = constants("UI_TAKEOVER_CHOICE")

-------------------------------------------------------------------------------
-- The screens
--
-- Each screen, and each ring entry of the ring entry screen, has a stack of
-- definitions. The top one answers, and one that a level script defines comes
-- off with the level.
-------------------------------------------------------------------------------

local definitions = {}

-- The context of each screen a script holds, by screen.
local held = {}

api.namespace("ui.screens", {
  description = [[
Lets a script draw an engine screen in place of the engine.

Define a screen with `trx.ui.screens.define`. When the engine opens the screen,
it calls the function that the definition gives. The function pushes layers
through the context it receives and returns the first one. The engine then draws
nothing for the screen and reads no input for it, until the script ends the
screen through the context.

The screen also ends when the layer that the definition returned closes, for
any reason, and the screen's other layers close with it. A layer that raises an
error therefore gives the screen back to the engine.

While a script holds a screen, a game-flow command such as
`trx.savegame.load` waits for the screen to end. In the inventory ring, the
ring spins out before the command runs.]],
})

api.enum("ui.Screen", {
  backing = "UI_TAKEOVER",
  description = "An engine screen that a script can draw.",
  values = {
    RING_ENTRY = [[
An entry that the player uses in the inventory ring. The context reports the
entry as `trx.ui.ScreenContext.object`. A definition can name the entry it
draws. A ring opened to save or load leaves when the screen ends, and any ring
leaves when the screen ends with `trx.ui.ScreenContext:confirm`.]],
    SAVE_LOAD = [[
The quick save or load screen. The save and load keys open it when the instant
screen setting is on. The context reports whether it opened for saving or
loading as `trx.ui.ScreenContext.mode`.]],
  },
})

local function key_of(screen, object)
  if object ~= nil then
    return screen .. ":" .. object
  end
  return tostring(screen)
end

local function definition_of(screen, object)
  local stack = definitions[key_of(screen, object)]
  if (stack == nil or #stack == 0) and object ~= nil then
    stack = definitions[key_of(screen, nil)]
  end
  if stack == nil then
    return nil
  end
  return stack[#stack]
end

-------------------------------------------------------------------------------
-- The context
-------------------------------------------------------------------------------

local Context

-- Ends the screen. The layers close after the context is marked done, so the
-- close callbacks see a screen that is already over.
local function finish(ctx, choice)
  if rawget(ctx, "_done") then
    return false
  end
  rawset(ctx, "_done", true)
  if held[ctx.screen] == ctx then
    held[ctx.screen] = nil
  end

  local layers = rawget(ctx, "_layers")
  rawset(ctx, "_layers", {})
  if choice ~= nil then
    raw.close(ctx.screen, choice)
  end
  for _, layer in ipairs(layers) do
    layer:close()
  end
  return true
end

local function on_layer_closed(ctx, layer)
  if not rawget(ctx, "_done") and rawget(ctx, "_main") == layer then
    finish(ctx, choices.CANCEL)
  end
end

Context = api.type("ui.ScreenContext", {
  description = "A screen that a script holds, which the definition receives.",
  fields = {
    screen = {
      type = "ui.Screen",
      description = "The screen.",
      get = function(self)
        return rawget(self, "_screen")
      end,
    },
    object = {
      type = "catalog.objects",
      nullable = true,
      description = "The ring entry that the player uses, for `trx.ui.Screen.RING_ENTRY`.",
      get = function(self)
        if rawget(self, "_screen") == trx.ui.Screen.RING_ENTRY then
          return rawget(self, "_arg")
        end
        return nil
      end,
    },
    mode = {
      type = "inventory_ring.Mode",
      nullable = true,
      description = "What the quick save or load screen opened for, for `trx.ui.Screen.SAVE_LOAD`.",
      get = function(self)
        if rawget(self, "_screen") == trx.ui.Screen.SAVE_LOAD then
          return rawget(self, "_arg")
        end
        return nil
      end,
    },
    is_held = {
      type = "boolean",
      description = "Whether the script still holds the screen.",
      get = function(self)
        return not rawget(self, "_done")
      end,
    },
  },
  methods = {
    push = {
      description = [[
Pushes a layer that belongs to the screen, with the settings that
`trx.ui.layers.push` takes. The layer closes when the screen ends.]],
      params = {
        {
          name = "settings",
          type = "table",
          description = "The layer settings.",
        },
      },
      returns = { type = "ui.StackLayer", description = "The pushed layer." },
      impl = function(self, settings)
        if rawget(self, "_done") then
          error("the screen has ended", 2)
        end
        local on_close = settings.on_close
        local layer_settings = {}
        for key, value in pairs(settings) do
          layer_settings[key] = value
        end
        layer_settings.on_close = function(layer)
          if on_close ~= nil then
            on_close(layer)
          end
          on_layer_closed(self, layer)
        end
        local layer = trx.ui.layers.push(layer_settings)
        local layers = rawget(self, "_layers")
        layers[#layers + 1] = layer
        return layer
      end,
    },
    cancel = {
      description = [[
Ends the screen, and closes its layers. A ring entry is put away, and the quick
save or load screen closes. Does nothing if the screen has already ended.]],
      returns = {
        type = "boolean",
        description = "Whether the screen was still held.",
      },
      impl = function(self)
        return finish(self, choices.CANCEL)
      end,
    },
    confirm = {
      description = [[
Ends the screen as a choice that the player made, and closes its layers. A ring
entry leaves the ring, as an entry that the player uses does. Does nothing if
the screen has already ended.]],
      returns = {
        type = "boolean",
        description = "Whether the screen was still held.",
      },
      impl = function(self)
        return finish(self, choices.CONFIRM)
      end,
    },
  },
})

local function new_context(screen, arg)
  return setmetatable({
    _screen = screen,
    _arg = arg,
    _layers = {},
    _done = false,
  }, Context)
end

-------------------------------------------------------------------------------
-- Definitions
-------------------------------------------------------------------------------

api.define("ui.screens.define", {
  description = [[
Defines how a script draws a screen.

The function receives a `trx.ui.ScreenContext` when the engine opens the
screen. It pushes the screen's layers through the context and returns the first
one. Returning nothing leaves the screen to the engine.

A screen has one definition. Defining it again is an error unless the options
say `override = true`. The new definition then replaces the old one, which comes
back when a level script's definition goes with its level.]],
  params = {
    {
      name = "screen",
      type = "ui.Screen",
      description = "The screen.",
    },
    {
      name = "open",
      type = "function",
      description = "Runs when the engine opens the screen.",
    },
    {
      name = "options",
      type = "table",
      optional = true,
      description = "The definition options.",
      fields = {
        {
          name = "object",
          type = "catalog.objects",
          optional = true,
          description = [[
The ring entry that the definition draws, for `trx.ui.Screen.RING_ENTRY`.
Without it, the definition draws every entry that has no definition of its
own.]],
        },
        {
          name = "override",
          type = "boolean",
          optional = true,
          description = "Whether to replace a definition that exists. `false` by default.",
        },
      },
    },
  },
  examples = {
    [[trx.ui.screens.define(trx.ui.Screen.RING_ENTRY, function(ctx)
  return ctx:push({
    root = trx.ui.widgets.Label({ text = "North" }),
    on_input = function(_, keys)
      if keys:pressed(trx.input.Role.MENU_BACK) then
        ctx:cancel()
      end
    end,
  })
end, { object = trx.catalog.objects.COMPASS_OPTION })]],
  },
  impl = function(screen, open, options)
    options = options or {}
    if options.object ~= nil and screen ~= trx.ui.Screen.RING_ENTRY then
      error("only a ring entry screen names an object", 2)
    end
    local key = key_of(screen, options.object)
    local stack = definitions[key]
    if stack == nil then
      stack = {}
      definitions[key] = stack
    end
    if #stack > 0 and not options.override then
      error("the screen is already defined; pass override = true", 2)
    end
    local definition = { open = open }
    stack[#stack + 1] = definition
    if raw_events.is_level_script() then
      trx.events.on_level_unload(function()
        for i, other in ipairs(stack) do
          if other == definition then
            table.remove(stack, i)
            break
          end
        end
      end)
    end
  end,
})

-------------------------------------------------------------------------------
-- The engine's side
-------------------------------------------------------------------------------

local function open_screen(screen, arg)
  local object = screen == trx.ui.Screen.RING_ENTRY and arg or nil
  local definition = definition_of(screen, object)
  if definition == nil then
    return false
  end

  local ctx = new_context(screen, arg)
  local ok, result = pcall(definition.open, ctx)
  if not ok then
    finish(ctx, nil)
    error(result, 0)
  end
  if result == nil then
    finish(ctx, nil)
    return false
  end

  local owned = false
  for _, layer in ipairs(rawget(ctx, "_layers")) do
    owned = owned or layer == result
  end
  if not owned then
    finish(ctx, nil)
    error("a screen definition must return a layer from ctx:push", 0)
  end

  rawset(ctx, "_main", result)
  held[screen] = ctx
  return true
end

raw_events.attach(events.SCREEN_OPEN, open_screen)

raw_events.attach(events.SCREEN_RELEASE, function(screen)
  local ctx = held[screen]
  if ctx ~= nil then
    finish(ctx, nil)
  end
end)
