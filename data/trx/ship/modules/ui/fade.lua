-- Every fade the shipped interface draws, so that each one runs the same way.
-- The engine runs each move; this module decides which fade runs, and
-- whether fades run at all.
--
-- Usage:
--   local fade = require("common.ui.fade")
--   fade.setup()  -- once, from a global script
--
--   local f = fade.screen(ctx)
--   f:enter({ ticks = ctx.fade_in_ticks })
--   f:leave({ ticks = ctx.fade_out_ticks, hold = 3 })

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
local Screen = {}
Screen.__index = Screen

-- A screen's fades, which close with the screen. `enabled` decides whether
-- they run, `fade.enabled` by default.
function M.screen(ctx, options)
  return setmetatable({
    ctx = ctx,
    is_enabled = options ~= nil and options.enabled or M.enabled,
    entered_fill = false,
  }, Screen)
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
  local layer = self.ctx:fade(layer_settings(spec, from))
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
    layer = ctx:fade({ fill = BLACK, opacity = 0 })
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
-- Fades that outlive their screen
-------------------------------------------------------------------------------

-- The open layers that outlive their screens, and the events that drop each.
---@type table<trx.ui.FadeLayer, table<string, boolean>>
local across = {}

local function drop_on(event)
  for layer, events in pairs(across) do
    if events[event] then
      across[layer] = nil
      layer:close()
    end
  end
end

-- Moves a fade that outlives its screen, and closes it once it is out of
-- sight.
function M.move(layer, target, ticks, on_done)
  layer:to(target, M.enabled() and ticks or 0, {
    close = target == 0,
    on_done = on_done,
  })
end

-- Puts up a fade that outlives the screen that starts it, black by default,
-- and moves it from `from` to `to`. It closes once it fades out of sight, or
-- when one of the `drop_on` events comes: "title", "unload", "cutscene" or
-- "demo".
function M.across(options)
  local settings =
    layer_settings(options.layer or { fill = BLACK }, options.from or 1)
  settings.on_close = function(layer)
    across[layer] = nil
  end
  local layer = trx.ui.layers.fade(settings)
  local events = {}
  for _, event in ipairs(options.drop_on or {}) do
    events[event] = true
  end
  across[layer] = events
  if options.to ~= nil then
    M.move(layer, options.to, options.ticks, options.on_done)
  end
  return layer
end

-------------------------------------------------------------------------------
-- Values that fade
-------------------------------------------------------------------------------

---@class common.ui.fade.Value
---@field private tween trx.ui.Tween
---@field private options table
local Value = {}
Value.__index = Value

-- The value now.
function Value:get()
  return self.tween:get()
end

-- Starts the fade again from the beginning.
function Value:restart()
  local options = self.options
  self.tween:set(options.from)
  self.tween:to(options.to, options.ticks, { delay = options.delay })
  return self
end

-- A value that fades from `from` to `to` over `ticks`, after waiting `delay`
-- ticks, which a widget takes in place of a fixed value. It starts at once.
function M.value(options)
  local value = setmetatable({
    tween = trx.ui.tween(options.from),
    options = options,
  }, Value)
  return value:restart()
end

-------------------------------------------------------------------------------
-- Setup
-------------------------------------------------------------------------------

-- Listens for the events that drop the fades that outlive their screens.
-- Called once, from a global script.
function M.setup()
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
