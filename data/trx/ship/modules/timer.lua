-- Runs a function once some ticks have passed, or again every few ticks.
--
-- A timer counts the ticks of one of two clocks. The game clock counts the
-- ticks in which the level runs its logic, so it stops while the game is
-- paused, in the inventory and in the menus. The interface clock counts every
-- tick, whatever is on screen, and is the clock for menus and fades.
--
-- A timer made by a level script stops when the level ends, as the event
-- handlers of a level script do. Timers are not written into a savegame.
--
-- Usage:
--   local timer = require("common.timer")
--   local alarm = timer.after(90, function() trx.log.info("time is up") end)
--   timer.every(30, blink, { clock = timer.Clock.UI })
--   alarm:cancel()

local M = {}

-- The clocks a timer counts.
M.Clock = {
  -- The ticks in which the level runs its logic.
  GAME = "game",
  -- Every tick.
  UI = "ui",
}

local HOOKS = {
  [M.Clock.GAME] = function(callback)
    return trx.events.before_control(callback)
  end,
  [M.Clock.UI] = function(callback)
    return trx.events.on_tick(callback)
  end,
}

---@class common.timer.Timer
---@field is_running boolean Whether the timer still has a tick to run on.
---@field private listener? trx.events.Listener
local Timer = {}
Timer.__index = Timer

-- Stops the timer, so that its function does not run again. Returns whether
-- the timer was still running.
function Timer:cancel()
  if self.listener == nil then
    return false
  end
  self.listener:detach()
  self.listener = nil
  self.is_running = false
  return true
end

-- Makes a timer that runs `fn` after `ticks` ticks, and again every `every`
-- ticks after that where `every` is given. The tick that makes the timer does
-- not count. An error in `fn` stops the timer, and is raised again.
local function start(ticks, every, fn, options)
  if math.type(ticks) ~= "integer" or ticks < 0 then
    error("the ticks must be a whole number of at least 0", 3)
  end
  local clock = options ~= nil and options.clock or M.Clock.GAME
  local hook = HOOKS[clock]
  if hook == nil then
    error("the clock is not one of common.timer.Clock", 3)
  end
  local self = setmetatable({ is_running = true }, Timer)
  local left = math.max(ticks, 1)
  self.listener = hook(function()
    left = left - 1
    if left > 0 then
      return
    end
    if every ~= nil then
      left = every
    else
      self:cancel()
    end
    local ok, err = pcall(fn)
    if not ok then
      self:cancel()
      error(err, 0)
    end
  end)
  return self
end

-- Runs `fn` once, `ticks` ticks from now. A timer of 0 ticks runs on the next
-- tick. `options.clock` picks the clock, `timer.Clock.GAME` by default.
function M.after(ticks, fn, options)
  return start(ticks, nil, fn, options)
end

-- Runs `fn` every `ticks` ticks, from `ticks` ticks from now, until the timer
-- is cancelled. `options.clock` picks the clock, `timer.Clock.GAME` by
-- default.
function M.every(ticks, fn, options)
  if math.type(ticks) ~= "integer" or ticks < 1 then
    error("the ticks must be a whole number of at least 1", 2)
  end
  return start(ticks, ticks, fn, options)
end

return M
