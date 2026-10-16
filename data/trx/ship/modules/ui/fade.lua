-- Every fade the shipped interface draws, so that each one runs the same way,
-- and how one screen gives way to the next. The engine runs each move; this
-- module decides which fade runs, and whether fades run at all.
--
-- Usage:
--   local fade = require("common.ui.fade")
--   fade.setup()  -- once, from a global script
--
--   local f = fade.screen(ctx)
--   f:enter({ ticks = ctx.fade_in_ticks })
--   f:leave({ ticks = ctx.fade_out_ticks, hold = 3 })
--
--   local show = fade.screen(ctx):show({
--     skip = trx.input.SkipContext.TO_GAME,
--     leave = function(fades) fades:leave({ ticks = ctx.fade_out_ticks }) end,
--   })
--
--   local black = fade.across({ from = 0, to = 1, ticks = 15 })
--   black:to(0, 15)
--
--   fade.hand_over({ ticks = 15, hold = 3 })  -- as a screen leaves

local timer = require("common.timer")

local M = {}

local BLACK = trx.math.color("000000")

-- Whether fades run: the fade effects setting, and a screen's own setting
-- where it has one.
function M.enabled(key)
  if not trx.config.get("visuals.enable_fade_effects") then
    return false
  end
  return key == nil or trx.config.get(key)
end

-- The settings of a fade layer that holds what `spec` names, at an opacity.
local function layer_settings(spec, opacity)
  local settings = { opacity = opacity }
  for key, value in pairs(spec) do
    settings[key] = value
  end
  return settings
end

-------------------------------------------------------------------------------
-- Screen fades
-------------------------------------------------------------------------------

---@class common.ui.fade.Screen
---@field private ctx trx.ui.ScreenContext
---@field private is_enabled fun(): boolean
---@field private entered? trx.ui.FadeLayer
---@field private entered_fill boolean
---@field private left? trx.ui.FadeLayer
---@field private pending? table
---@field private holding? common.timer.Timer
---@field private layers trx.ui.FadeLayer[]
local Screen = {}
Screen.__index = Screen

-- A screen's fades, which close with the screen. `enabled` decides whether
-- they run, `fade.enabled` by default.
function M.screen(ctx, options)
  ---@type common.ui.fade.Screen
  local self = setmetatable({
    ctx = ctx,
    is_enabled = options ~= nil and options.enabled or M.enabled,
    entered_fill = false,
    layers = {},
  }, Screen)
  ctx:on_end(function()
    if self.holding ~= nil then
      self.holding:cancel()
      self.holding = nil
    end
    for _, layer in ipairs(self.layers) do
      layer:close()
    end
  end)
  return self
end

-- Puts up a fade layer that closes with the screen.
function Screen:fade(settings)
  local layer = trx.ui.layers.fade(settings)
  self.layers[#self.layers + 1] = layer
  return layer
end

function Screen:ticks(ticks)
  return self.is_enabled() and ticks or 0
end

-- Moves a layer, then rests it for the hold, then runs `on_done`, once. A
-- later move of the same screen cuts it short, and so does the layer closing.
function Screen:move(layer, target, ticks, hold, on_done, close)
  if self.holding ~= nil then
    self.holding:cancel()
    self.holding = nil
  end
  local token = {}
  self.pending = token
  local function done()
    if self.pending ~= token then
      return
    end
    self.pending = nil
    if on_done ~= nil then
      on_done()
    end
  end
  hold = self:ticks(hold or 0)
  layer:to(target, self:ticks(ticks), {
    close = close and hold == 0,
    on_done = function()
      if self.pending ~= token then
        return
      end
      if hold == 0 then
        done()
        return
      end
      self.holding = timer.after(hold, function()
        self.holding = nil
        if not layer.is_open then
          return
        end
        if close then
          layer:close()
        end
        done()
      end, { clock = timer.Clock.UI })
    end,
  })
end

-- Fades the screen in. The layer holds what `layer` names, black by default,
-- and goes from `from` to `to`, 1 and 0 by default. A layer that fades out of
-- sight closes, unless it is a colour, which `leave` takes up again.
function Screen:enter(options)
  local spec = options.layer or { fill = BLACK }
  local from = options.from or 1
  local to = options.to or 0
  local layer = self:fade(layer_settings(spec, from))
  self.entered = layer
  self.entered_fill = spec.fill ~= nil
  if from ~= to then
    local close = to == 0 and spec.fill == nil
    self:move(layer, to, options.ticks or 0, 0, options.on_done, close)
  elseif options.on_done ~= nil then
    options.on_done()
  end
  return layer
end

-- Fades the screen out to black, rests there for the hold, then runs
-- `on_done`, which ends a held screen by default. A colour the screen came in
-- through fades out again; anything else is covered by a new black.
function Screen:leave(options)
  local ctx = self.ctx
  local layer = self.entered
  if layer == nil or not layer.is_open or not self.entered_fill then
    layer = self:fade({ fill = BLACK, opacity = 0 })
  end
  self.left = layer
  local on_done = options.on_done
    or function()
      if ctx.is_held then
        ctx:confirm()
      end
    end
  self:move(layer, 1, options.ticks or 0, options.hold, on_done)
  return layer
end

-- Keeps the screen black for some ticks after it has left, then runs
-- `on_done`.
function Screen:wait(ticks, on_done)
  self:move(self.left, 1, 0, ticks, on_done)
end

-------------------------------------------------------------------------------
-- Screens shown for a time
-------------------------------------------------------------------------------

---@class common.ui.fade.Show
---@field progress trx.ui.Tween How far the screen is through its time, from 0 to 1.
---@field is_leaving boolean Whether the screen has started to leave.
---@field private fades common.ui.fade.Screen
---@field private options table
---@field private showing? common.timer.Timer
local Show = {}
Show.__index = Show

-- Starts to leave, as the time runs out or the player skips.
function Show:leave()
  if self.is_leaving then
    return
  end
  self.is_leaving = true
  if self.showing ~= nil then
    self.showing:cancel()
    self.showing = nil
  end
  self.progress:set(1)
  self.options.leave(self.fades)
end

-- Reads one tick of input: a skip leaves, and a skip while the screen leaves
-- ends it.
function Show:control(keys)
  if not keys:pressed(trx.input.Role.MENU_SKIP) then
    return
  end
  trx.input.hold_off_skip(self.options.skip)
  local ctx = self.fades.ctx
  if not self.is_leaving then
    self:leave()
  elseif ctx.is_held then
    ctx:confirm()
  end
end

-- Fades in a screen that the game shows for a time, such as a picture or a
-- view of the level, and starts its time. It shows for its display ticks while
-- its progress fills, and then leaves. The options are:
--   layer - what the screen fades in from, as `enter` takes it, black by
--           default
--   skip  - the trx.input.SkipContext that a skip holds off
--   leave - runs as the screen leaves, with the screen's fades
function Screen:show(options)
  local ctx = self.ctx --[[@as trx.ui.TimedContext]]
  local show = setmetatable({
    fades = self,
    options = options,
    progress = trx.ui.tween(0),
    is_leaving = false,
  }, Show)
  ctx:on_end(function()
    if show.showing ~= nil then
      show.showing:cancel()
    end
  end)
  self:enter({
    layer = options.layer,
    ticks = ctx.fade_in_ticks,
    on_done = function()
      if show.is_leaving then
        return
      end
      show.progress:to(1, ctx.display_ticks)
      show.showing = timer.after(ctx.display_ticks, function()
        show.showing = nil
        show:leave()
      end, { clock = timer.Clock.UI })
    end,
  })
  return show
end

-------------------------------------------------------------------------------
-- Fades that outlive their screen
-------------------------------------------------------------------------------

---@class common.ui.fade.Across
---@field private layer trx.ui.FadeLayer
local Across = {}
Across.__index = Across

-- The open fades that outlive their screens, and the events that drop each.
---@type table<common.ui.fade.Across, table<string, boolean>>
local across = {}

local function drop_on(event)
  for fade, events in pairs(across) do
    if events[event] then
      fade:close()
    end
  end
end

-- Whether the fade is still drawn.
function Across:is_open()
  return self.layer.is_open
end

-- Moves the fade to `target`, over `ticks` while fades run and at once where
-- they do not, then runs `on_done`. A fade that moves out of sight closes.
function Across:to(target, ticks, on_done)
  self.layer:to(target, M.enabled() and ticks or 0, {
    close = target == 0,
    on_done = on_done,
  })
  return self
end

-- Stops drawing the fade.
function Across:close()
  across[self] = nil
  self.layer:close()
end

-- Puts up a fade that outlives the screen that starts it, black by default,
-- and moves it from `from` to `to`. It closes once it fades out of sight, or
-- when one of the `drop_on` events comes: "title", "unload", "cutscene" or
-- "demo".
function M.across(options)
  local settings =
    layer_settings(options.layer or { fill = BLACK }, options.from or 1)
  local self = setmetatable({}, Across)
  settings.on_close = function()
    across[self] = nil
  end
  self.layer = trx.ui.layers.fade(settings)
  local events = {}
  for _, event in ipairs(options.drop_on or {}) do
    events[event] = true
  end
  across[self] = events
  if options.to ~= nil then
    self:to(options.to, options.ticks, options.on_done)
  end
  return self
end

-------------------------------------------------------------------------------
-- Values that fade
-------------------------------------------------------------------------------

-- A tween that fades from `from` to `to` over `ticks`, after waiting `delay`
-- ticks, which a widget takes in place of a fixed value, and a function that
-- starts the fade again from the beginning. It starts at once.
---@return trx.ui.Tween, fun()
function M.value(options)
  local tween = trx.ui.tween(options.from)
  local function restart()
    tween:set(options.from)
    tween:to(options.to, options.ticks, { delay = options.delay })
  end
  restart()
  return tween, restart
end

-------------------------------------------------------------------------------
-- Changing screens
-------------------------------------------------------------------------------

-- How the screen that left asked the next one to come in, or nil for a cut.
---@type { ticks: integer, hold: integer }?
local handed = nil

-- Asks for the screen on show to fade to black before the next screen starts,
-- over `ticks` and then resting black for `hold`, unless the next screen
-- blends in from it. The next screen waits for the fade to end.
function M.hand_over(options)
  handed = { ticks = options.ticks or 0, hold = options.hold or 0 }
end

-- Fades the frame on show to black, rests there, and lets the next screen
-- start.
local function fade_to_black(ticks, hold)
  local frame = trx.ui.layers.fade({ snapshot = true, holds_screen = true })
  local black = trx.ui.layers.fade({
    fill = BLACK,
    opacity = 0,
    holds_screen = true,
    on_close = function()
      frame:close()
    end,
  })
  black:to(1, ticks, {
    on_done = function()
      timer.after(hold, function()
        black:close()
      end, { clock = timer.Clock.UI })
    end,
  })
end

-- The game fades to black over the console as it exits, over a third of a
-- second, and holds the black for a tenth before it closes.
local EXIT_TICKS = math.floor(trx.game.LOGIC_FPS / 3 + 0.5)
local EXIT_HOLD = math.floor(trx.game.LOGIC_FPS / 10 + 0.5)

local function on_exit()
  if not M.enabled("visuals.enable_exit_fade_effects") then
    return
  end
  local black = trx.ui.layers.fade({
    fill = BLACK,
    opacity = 0,
    over_console = true,
    holds_screen = true,
  })
  black:to(1, EXIT_TICKS, {
    on_done = function()
      timer.after(EXIT_HOLD, function()
        black.holds_screen = false
      end, { clock = timer.Clock.UI })
    end,
  })
end

local function on_screen_change(cross_fades, fade_ticks, fade_hold)
  local request = handed
  handed = nil
  if request == nil and fade_ticks > 0 then
    request = { ticks = fade_ticks, hold = fade_hold }
  end
  if request ~= nil and not cross_fades then
    fade_to_black(request.ticks, request.hold)
  end
end

-------------------------------------------------------------------------------
-- Setup
-------------------------------------------------------------------------------

-- Listens for the screens changing and the game exiting, and for the events
-- that drop the fades that outlive their screens. Called once, from a global
-- script.
function M.setup()
  trx.events.on_screen_change(on_screen_change)
  trx.events.on_exit(on_exit)
  trx.events.on_game_start(function()
    local level = trx.game.current_level
    if level ~= nil and level.type == trx.game.LevelType.CUTSCENE then
      drop_on("cutscene")
    elseif level ~= nil and level.type == trx.game.LevelType.DEMO then
      drop_on("demo")
    end
  end)
  trx.events.on_title_start(function()
    drop_on("title")
  end)
  trx.events.on_level_unload(function()
    drop_on("unload")
  end)
end

return M
