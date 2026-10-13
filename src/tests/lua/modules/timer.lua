-- The shipped timer module: delays, repeats, cancelling, and the two clocks.

local h = require("harness")
local test = h.test

local timer = require("common.timer")

local function ticks(count)
  for _ = 1, count do
    fake.tick()
  end
end

local function controls(count)
  for _ = 1, count do
    fake.control()
  end
end

local function counter()
  local runs = { count = 0 }
  function runs.fn()
    runs.count = runs.count + 1
  end
  return runs
end

test("a timer runs once, after its ticks", function()
  local runs = counter()
  local t = timer.after(3, runs.fn, { clock = timer.Clock.UI })
  ticks(2)
  assert(runs.count == 0, "the timer ran early")
  fake.tick()
  assert(runs.count == 1, "the timer did not run")
  assert(not t.is_running, "the timer still runs")
  ticks(5)
  assert(runs.count == 1, "the timer ran twice")
end)

test("a timer of no ticks runs on the next tick", function()
  local runs = counter()
  timer.after(0, runs.fn, { clock = timer.Clock.UI })
  assert(runs.count == 0, "the timer ran at once")
  fake.tick()
  assert(runs.count == 1, "the timer did not run")
end)

test("a repeating timer runs every few ticks until cancelled", function()
  local runs = counter()
  local t = timer.every(2, runs.fn, { clock = timer.Clock.UI })
  ticks(6)
  assert(runs.count == 3, "the timer ran " .. runs.count .. " times")
  assert(t:cancel(), "the timer was not running")
  ticks(4)
  assert(runs.count == 3, "the timer ran after it was cancelled")
  assert(not t:cancel(), "a second cancel reported a running timer")
end)

test("a cancelled timer does not run", function()
  local runs = counter()
  local t = timer.after(2, runs.fn, { clock = timer.Clock.UI })
  fake.tick()
  t:cancel()
  ticks(3)
  assert(runs.count == 0, "the timer ran")
end)

test("a timer can cancel itself", function()
  local runs = 0
  local t
  t = timer.every(1, function()
    runs = runs + 1
    t:cancel()
  end, { clock = timer.Clock.UI })
  ticks(3)
  assert(runs == 1, "the timer ran " .. runs .. " times")
end)

test("the game clock counts only the ticks the level runs", function()
  local runs = counter()
  timer.after(2, runs.fn)
  ticks(5)
  assert(runs.count == 0, "the game clock counted a paused tick")
  controls(2)
  assert(runs.count == 1, "the timer did not run")
end)

test("an error stops the timer", function()
  local runs = 0
  local t = timer.every(1, function()
    runs = runs + 1
    error("broken")
  end, { clock = timer.Clock.UI })
  local before = fake.errors()
  ticks(3)
  assert(runs == 1, "the timer ran " .. runs .. " times")
  assert(fake.errors() > before, "the error was not raised")
  assert(not t.is_running, "the timer still runs")
end)

test("a timer takes whole ticks and a known clock", function()
  assert(not pcall(timer.after, -1, function() end), "took negative ticks")
  assert(not pcall(timer.after, 1.5, function() end), "took a fraction")
  assert(not pcall(timer.every, 0, function() end), "took a zero period")
  assert(
    not pcall(timer.after, 1, function() end, { clock = "wall" }),
    "took an unknown clock"
  )
end)
return h.report()
