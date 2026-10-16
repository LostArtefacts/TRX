local raw = trxc.screens
local raw_hooks = trxc.hooks
local h = require("trx.internal.helpers")

require("trx.ui")
require("trx.ui.layers")
local at_level_end = require("trx.events").at_level_end

local ui = trx.ui

-- The choices are the engine's, read back as any other enum is. They are not
-- public: a script defines a screen and ends it through its context, and never
-- names one.
local choices = h.constants("UI_TAKEOVER_CHOICE")

-------------------------------------------------------------------------------
-- The screens
--
-- Each screen, and each ring entry of the ring entry screen, has a stack of
-- definitions. The top one answers, and one that a level script defines comes
-- off with the level.
-------------------------------------------------------------------------------

---@type table<string, { open: function }[]>
local definitions = {}

-- The context of each screen a script holds, by screen.
local held = {}

---@class (partial) trx.ui
---@field screens trx.ui.screens
---@field Screen table<string, trx.ui.Screen>
---@field MenuMode table<string, trx.ui.MenuMode>

---Lets a script draw an engine screen in place of the engine.
---
---Define a screen with `trx.ui.screens.define`. When the engine opens the
---screen, it calls the function that the definition gives. The function pushes
---layers through the context it receives and returns the first one. The engine
---then draws nothing for the screen and reads no input for it, until the
---script ends the screen through the context.
---
---The screen also ends when the layer that the definition returned closes, for
---any reason, and the screen's other layers close with it. A layer that raises
---an error therefore gives the screen back to the engine.
---
---While a script holds a screen, a game-flow command such as
---`trx.savegame.load` waits for the screen to end. In the inventory ring, the
---ring spins out before the command runs.
---@class (partial,exact) trx.ui.screens
ui.screens = h.namespace("ui.screens")

---@class (partial) trx.ui.screens
local screens = trx.ui.screens

---An engine screen that a script can draw. Each screen hands its definition
---a context of its own kind, which reports what the screen shows.
---@enum trx.ui.Screen
local Screen = {
  ---An entry that the player uses in the inventory ring, with a
  ---`trx.ui.EntryContext`. A definition can name the entry it draws. A ring
  ---opened to save or load leaves when the screen ends, and any ring leaves
  ---when the screen ends with `trx.ui.ScreenContext:confirm`.
  RING_ENTRY = h.IntegerConstant,
  ---The pause screen, from the moment the game or an FMV pauses until it
  ---resumes or leaves for the title screen. It has a `trx.ui.PauseContext`.
  PAUSE = h.IntegerConstant,
  ---The quick save or load screen, with a `trx.ui.SaveLoadContext`. The save
  ---and load keys open it when the instant screen setting is on.
  SAVE_LOAD = h.IntegerConstant,
  ---The statistics that the game shows when a level ends, and the totals it
  ---shows when the game ends, with a `trx.ui.StatsContext`. The player skips
  ---the screen as they would the engine's, and the screen also ends with
  ---`trx.ui.ScreenContext:cancel` or `trx.ui.ScreenContext:confirm`.
  STATS = h.IntegerConstant,
  ---An entry that the player examines in the inventory ring: its name and
  ---description, and turning it by hand. It has a `trx.ui.EntryContext`. The
  ---screen ends with `trx.ui.ScreenContext:cancel`, which puts the entry
  ---away.
  EXAMINE = h.IntegerConstant,
  ---An entry that the player combines in the inventory ring: picking what to
  ---combine it with, out of `trx.inventory.combine_partners`, and combining
  ---the two with `trx.inventory.combine`. It has a `trx.ui.EntryContext`. The
  ---screen ends with `trx.ui.ScreenContext:cancel` or
  ---`trx.ui.ScreenContext:confirm`, which both put the entry away, and the
  ---ring then rests on what a combination made.
  COMBINE = h.IntegerConstant,
  ---The globe that TR3 picks its next adventure on, between levels, with a
  ---`trx.ui.GlobeContext`. The script picks an adventure by starting its
  ---level with `trx.game.play_level`, and `trx.ui.ScreenContext:cancel`
  ---leaves for the title screen.
  GLOBE_SELECT = h.IntegerConstant,
  ---A view of the level from one of its cameras, which the game shows while
  ---the level starts, as TR4 does. It has a `trx.ui.TimedContext`. The view
  ---ends with `trx.ui.ScreenContext:confirm` or
  ---`trx.ui.ScreenContext:cancel`.
  LEVEL_VIEW = h.IntegerConstant,
  ---A picture that the game shows between levels, while a level loads, or
  ---for the legal notice and the credits, with a `trx.ui.PictureContext`. The
  ---picture ends with `trx.ui.ScreenContext:confirm` or
  ---`trx.ui.ScreenContext:cancel`.
  PICTURE = h.IntegerConstant,
  ---The menu the game opens over the play and on the title screen, such as
  ---the inventory ring, with a `trx.ui.MenuContext`. The menu ends with
  ---`trx.ui.ScreenContext:confirm` once it has done what the player chose.
  MENU = h.IntegerConstant,
}
ui.Screen = h.enum("ui.Screen", "UI_TAKEOVER", Screen)

---What the engine opens its menu for, which the menu screen is told as it
---opens.
---@enum trx.ui.MenuMode
local MenuMode = {
  ---Opened during play.
  GAME = h.IntegerConstant,
  ---The title screen's menu.
  TITLE = h.IntegerConstant,
  ---The keys ring, opened against a locked door or receptacle.
  KEYS = h.IntegerConstant,
  ---Opened to save, with the save list already on show.
  SAVE = h.IntegerConstant,
  ---Opened to load, with the save list already on show.
  LOAD = h.IntegerConstant,
  ---Opened because Lara died.
  DEATH = h.IntegerConstant,
  ---Opened by a save crystal.
  SAVE_CRYSTAL = h.IntegerConstant,
}
ui.MenuMode = h.enum("ui.MenuMode", "INVENTORY_MODE", MenuMode)

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

-- Ends the screen. The layers close after the context is marked done, so the
-- close callbacks see a screen that is already over. An error in a callback
-- does not stop the rest, and the first one is raised at the end.
local function finish(ctx, choice)
  if ctx._done then
    return false
  end
  ctx._done = true
  if held[ctx.screen] == ctx then
    held[ctx.screen] = nil
  end

  local layers = ctx._layers
  ctx._layers = {}
  if choice ~= nil and ctx._report == nil then
    raw.close(ctx.screen, choice)
  end
  local failed = nil
  for _, layer in ipairs(layers) do
    local err = h.failure(layer.close, layer)
    failed = failed or err
  end
  local ended = ctx._ended
  ctx._ended = {}
  for _, callback in ipairs(ended) do
    local err = h.failure(callback)
    failed = failed or err
  end
  if ctx._report ~= nil then
    local err = h.failure(ctx._report, choice == choices.CONFIRM)
    failed = failed or err
  end
  if failed ~= nil then
    error(failed, 0)
  end
  return true
end

local function on_layer_closed(ctx, layer)
  if not ctx._done and ctx._main == layer then
    finish(ctx, choices.CANCEL)
  end
end

local function reported(name)
  return {
    get = function(self)
      return self._fields[name]
    end,
  }
end

local background = reported("background")

---A screen that a script holds, which the definition receives. Each screen
---hands over a context of its own kind, which adds what that screen reports to
---what every screen has.
---@class (exact) trx.ui.ScreenContext
---@trx.readonly is_held, screen
---@field screen trx.ui.Screen The screen.
---@field is_held boolean Whether the script still holds the screen.
---@field private _screen trx.ui.Screen
---@field private _fields table<string, any>
---@field private _layers trx.ui.StackLayer[]
---@field private _ended function[]
---@field private _done boolean
---@field private _main? trx.ui.StackLayer
---@field private _report? function
local Context = h.class("ui.ScreenContext", {
  fields = {
    screen = {
      get = function(self)
        return self._screen
      end,
    },
    is_held = {
      get = function(self)
        return not self._done
      end,
    },
  },
})

---Pushes a layer that belongs to the screen, with the settings that
---`trx.ui.layers.push` takes. The layer closes when the screen ends.
---@param settings trx.ui.layers.push.settings The layer settings.
---@return trx.ui.StackLayer # The pushed layer.
function Context:push(settings)
  if self._done then
    error("the screen has ended", 2)
  end
  ---@type table<string, any>
  local forwarded = {}
  for key, value in pairs(settings) do
    forwarded[key] = value
  end
  forwarded.on_close = function(layer)
    local err = h.failure(settings.on_close, layer)
    on_layer_closed(self, layer)
    if err ~= nil then
      error(err, 0)
    end
  end
  local layer =
    trx.ui.layers.push(forwarded --[[@as trx.ui.layers.push.settings]])
  local layers = self._layers
  layers[#layers + 1] = layer
  return layer
end

---Runs a function once the screen ends, for any reason, after its layers have
---closed.
---
---```lua
---local black = trx.ui.layers.fade({ fill = trx.math.color("000000") })
---ctx:on_end(function()
---  black:close()
---end)
---```
---@param callback function What to run.
function Context:on_end(callback)
  if self._done then
    error("the screen has ended", 2)
  end
  local ended = self._ended
  ended[#ended + 1] = callback
end

---Ends the screen, and closes its layers. A ring entry is put away, the pause
---screen returns to the game, the quick save or load screen
---closes, and the statistics screen ends as the player skipping it does. Does
---nothing if the screen has already ended.
---@return boolean # Whether the screen was still held.
function Context:cancel()
  return finish(self, choices.CANCEL)
end

---Ends the screen as a choice that the player made, and closes its layers. A
---ring entry leaves the ring, as an entry that the player uses does, a
---combined entry is put away, and the statistics screen ends as the player
---skipping it does. Does nothing if the screen has already ended.
---@return boolean # Whether the screen was still held.
function Context:confirm()
  return finish(self, choices.CONFIRM)
end

---The context of an entry of the inventory ring, for
---`trx.ui.Screen.RING_ENTRY`, `trx.ui.Screen.EXAMINE` and
---`trx.ui.Screen.COMBINE`.
---@class (exact) trx.ui.EntryContext: trx.ui.ScreenContext
---@trx.readonly object
---@field object trx.catalog.objects The entry that the player uses, examines or combines.
local EntryContext = h.class("ui.EntryContext", {
  extends = "ui.ScreenContext",
  fields = { object = reported("object") },
})

---The context of the pause screen, for `trx.ui.Screen.PAUSE`.
---@class (exact) trx.ui.PauseContext: trx.ui.ScreenContext
local Pause = h.class("ui.PauseContext", { extends = "ui.ScreenContext" })

---Ends the pause screen, and returns to the game.
---@return boolean # Whether the screen was still held.
function Pause:resume()
  return finish(self, choices.RESUME)
end

---Ends the pause screen, and leaves for the title screen.
---@return boolean # Whether the screen was still held.
function Pause:exit_to_title()
  return finish(self, choices.EXIT_TO_TITLE)
end

---The context of the quick save or load screen, for
---`trx.ui.Screen.SAVE_LOAD`.
---@class (exact) trx.ui.SaveLoadContext: trx.ui.ScreenContext
---@trx.readonly mode
---@field mode trx.ui.MenuMode Whether the screen opened for saving or for loading.
local SaveLoadContext = h.class("ui.SaveLoadContext", {
  extends = "ui.ScreenContext",
  fields = { mode = reported("mode") },
})

---The context of the statistics screen, for `trx.ui.Screen.STATS`.
---@class (exact) trx.ui.StatsContext: trx.ui.ScreenContext
---@trx.readonly background, is_bare, is_final, level
---@field level trx.game.Level The level whose statistics the screen shows. The totals name the level that ended the game.
---@field is_final boolean Whether the screen shows the totals for the whole game rather than one level.
---@field is_bare boolean Whether the screen is drawn bare, as plain lines of text without a frame.
---@field background string? The picture the screen stands on, as `trx.ui.primitive.background` takes it, or `nil` for none.
local StatsContext = h.class("ui.StatsContext", {
  extends = "ui.ScreenContext",
  fields = {
    level = reported("level"),
    is_final = reported("is_final"),
    is_bare = reported("is_bare"),
    background = background,
  },
})

---The context of the TR3 globe, for `trx.ui.Screen.GLOBE_SELECT`.
---@class (exact) trx.ui.GlobeContext: trx.ui.ScreenContext
---@trx.readonly background
---@field background string? The picture the globe stands on, as `trx.ui.primitive.background` takes it, or `nil` for none.
local GlobeContext = h.class("ui.GlobeContext", {
  extends = "ui.ScreenContext",
  fields = { background = background },
})

---The context of a screen that the game shows for a time, for
---`trx.ui.Screen.LEVEL_VIEW`.
---@class (exact) trx.ui.TimedContext: trx.ui.ScreenContext
---@trx.readonly display_ticks, fade_in_ticks, fade_out_ticks
---@field display_ticks integer How many ticks the screen shows before it fades out.
---@field fade_in_ticks integer How many ticks the screen takes to fade in.
---@field fade_out_ticks integer How many ticks the screen takes to fade out.
local TimedContext = h.class("ui.TimedContext", {
  extends = "ui.ScreenContext",
  fields = {
    display_ticks = reported("display_ticks"),
    fade_in_ticks = reported("fade_in_ticks"),
    fade_out_ticks = reported("fade_out_ticks"),
  },
})

---The context of a picture, for `trx.ui.Screen.PICTURE`.
---@class (exact) trx.ui.PictureContext: trx.ui.TimedContext
---@trx.readonly background, can_cross_fade_in, is_loading
---@field background string? The picture, as `trx.ui.primitive.background` takes it, or `nil` for none.
---@field is_loading boolean Whether the picture shows while a level loads.
---@field can_cross_fade_in boolean Whether the picture may fade in from the frame shown before it, rather than from black.
local PictureContext = h.class("ui.PictureContext", {
  extends = "ui.TimedContext",
  fields = {
    background = background,
    is_loading = reported("is_loading"),
    can_cross_fade_in = reported("can_cross_fade_in"),
  },
})

-- The context class of each screen.
---@type table<integer, table>
---The context of the game's menu, for `trx.ui.Screen.MENU`.
---@class (exact) trx.ui.MenuContext: trx.ui.ScreenContext
---@trx.readonly mode, object, background, is_live
---@field mode trx.ui.MenuMode What the menu opened for.
---@field object trx.catalog.objects? The entry the menu opens on, such as the crowbar a door asks for, or `nil` for none.
---@field background string? The picture the title's menu stands on, as `trx.ui.primitive.background` takes it, or `nil` for none.
---@field is_live boolean Whether the title level runs behind the menu, in place of a picture.
local MenuContext = h.class("ui.MenuContext", {
  extends = "ui.ScreenContext",
  fields = {
    mode = reported("mode"),
    object = reported("object"),
    background = background,
    is_live = reported("is_live"),
  },
})

---@type table<trx.ui.Screen, table>
local KINDS = {
  [trx.ui.Screen.RING_ENTRY] = EntryContext,
  [trx.ui.Screen.EXAMINE] = EntryContext,
  [trx.ui.Screen.COMBINE] = EntryContext,
  [trx.ui.Screen.PAUSE] = Pause,
  [trx.ui.Screen.SAVE_LOAD] = SaveLoadContext,
  [trx.ui.Screen.STATS] = StatsContext,
  [trx.ui.Screen.GLOBE_SELECT] = GlobeContext,
  [trx.ui.Screen.LEVEL_VIEW] = TimedContext,
  [trx.ui.Screen.PICTURE] = PictureContext,
  [trx.ui.Screen.MENU] = MenuContext,
}

-- `args` is what the engine reports as it opens the screen, keyed by the
-- context fields.
local function new_context(screen, args)
  return setmetatable({
    _screen = screen,
    _fields = args,
    _layers = {},
    _ended = {},
    _done = false,
  }, KINDS[screen])
end

-------------------------------------------------------------------------------
-- Definitions
-------------------------------------------------------------------------------

---@class (exact) trx.ui.screens.define.options
---@field object? trx.catalog.objects The ring entry that the definition draws, for `trx.ui.Screen.RING_ENTRY`. Without it, the definition draws every entry that has no definition of its own.
---@field override? boolean Whether to replace a definition that exists. `false` by default.

---Defines how a script draws a screen.
---
---The function receives a `trx.ui.ScreenContext` when the engine opens the
---screen. It pushes the screen's layers through the context and returns the
---first one. Returning nothing leaves the screen to the engine.
---
---A screen has one definition. Defining it again is an error unless the
---options say `override = true`. The new definition then replaces the old one,
---which comes back when a level script's definition goes with its level.
---
---```lua
---trx.ui.screens.define(trx.ui.Screen.RING_ENTRY, function(ctx)
---  return ctx:push({
---    root = trx.ui.widgets.Label({ text = "North" }),
---    on_input = function(_, keys)
---      if keys:pressed(trx.input.Role.MENU_BACK) then
---        ctx:cancel()
---      end
---    end,
---  })
---end, { object = trx.catalog.objects.COMPASS_OPTION })
---```
---@param screen trx.ui.Screen The screen.
---@param open function Runs when the engine opens the screen.
---@param options? trx.ui.screens.define.options The definition options.
function screens.define(screen, open, options)
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
  at_level_end(function()
    for i, other in ipairs(stack) do
      if other == definition then
        table.remove(stack, i)
        break
      end
    end
  end)
end

-- Opens a screen's definition and returns its context, or nil where no
-- definition draws the screen. A context opened with a report tells it how
-- the screen ended instead of telling the engine.
local function start(screen, args, report)
  local object = screen == trx.ui.Screen.RING_ENTRY and args.object or nil
  local definition = definition_of(screen, object)
  if definition == nil then
    return nil
  end

  local ctx = new_context(screen, args)
  ctx._report = report
  local ok, result = pcall(definition.open, ctx)
  if not ok then
    finish(ctx, nil)
    error(result, 0)
  end
  if result == nil then
    finish(ctx, nil)
    return nil
  end

  local owned = false
  for _, layer in ipairs(ctx._layers) do
    owned = owned or layer == result
  end
  if not owned then
    finish(ctx, nil)
    error("a screen definition must return a layer from ctx:push", 0)
  end

  ctx._main = result
  return ctx
end

---Opens a screen from a script, as the engine opens one, with its definition
---drawing it. This is how a menu that a script draws opens what an entry
---leads to, such as `trx.ui.Screen.EXAMINE`.
---
---The fields are what the screen reports to its definition, such as the
---object of a ring entry's screens. The function given runs once the screen
---ends, after its layers have closed, and is told whether it ended with
---`trx.ui.ScreenContext:confirm`.
---
---```lua
---trx.ui.screens.open(trx.ui.Screen.EXAMINE, { object = object }, function(confirmed)
---  trx.log.info(confirmed and "confirmed" or "put away")
---end)
---```
---@param screen trx.ui.Screen The screen.
---@param fields table What the screen reports to its definition.
---@param on_end? function Runs once the screen ends, with whether it ended confirmed.
---@return trx.ui.ScreenContext? # The screen's context, or `nil` where no definition draws the screen, in which case the function given never runs.
function screens.open(screen, fields, on_end)
  return start(screen, fields, on_end or function() end)
end

-------------------------------------------------------------------------------
-- The engine's side
-------------------------------------------------------------------------------

local function open_screen(screen, args)
  local ctx = start(screen, args, nil)
  if ctx == nil then
    return false
  end
  held[screen] = ctx
  return true
end

raw_hooks.set("screen_open", open_screen)

raw_hooks.set("screen_release", function(screen)
  local ctx = held[screen]
  if ctx ~= nil then
    finish(ctx, nil)
  end
end)
