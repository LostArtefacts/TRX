-- Signals store values, notify listeners, and compose into derived signals.
-- Most assertions check listener counts because derived signals should fire
-- only when their own value changes, not whenever any input changes.

local h = require("harness")
local test = h.test

local signal = trx.signal

-- Count listener calls and keep the last value they received.
local function watch(s)
  local seen = { count = 0, last = nil }
  seen.listener = s:on(function(value)
    seen.count = seen.count + 1
    seen.last = value
  end)
  return seen
end

-- Runs before anything else reads `signal.tick`: with the tick attached from
-- the first read rather than at module load, a level script's read is what
-- arms it, and the handler then goes when that level does.
-- The engine tick reaches the tick signal through a handler of its own. One
-- attached from a level script goes when the level does, taking every polled
-- signal in the session with it.
test("a polled signal still reads after a level a script polled in", function()
  local reads = 0
  fake.as_level_script(function()
    signal.polled(function()
      reads = reads + 1
      return reads
    end)
  end)

  fake.end_level()
  local after_level = reads
  fake.tick()
  fake.tick()
  assert(reads == after_level, "the level's own signal stopped reading")

  local session_reads = 0
  signal.polled(function()
    session_reads = session_reads + 1
    return session_reads
  end)
  local before = session_reads
  fake.tick()
  assert(session_reads > before, "the tick still reaches a polled signal")
end)

test("a signal holds what it was made with", function()
  assert(signal.new(7):get() == 7)
  assert(signal.new(false):get() == false)
  assert(signal.new():get() == nil)
end)

test("setting a new value tells the listeners", function()
  local s = signal.new(1)
  local seen = watch(s)
  assert(seen.count == 0, "attaching is not a change")

  assert(s:set(2))
  assert(s:get() == 2)
  assert(seen.count == 1)
  assert(seen.last == 2)
end)

test("setting the value it already holds does nothing", function()
  local s = signal.new(1)
  local seen = watch(s)
  assert(not s:set(1), "no change is reported")
  assert(seen.count == 0, "and no listener runs")
end)

test("a detached listener hears no more", function()
  local s = signal.new(1)
  local seen = watch(s)
  s:set(2)
  assert(seen.listener:detach())
  s:set(3)
  assert(seen.count == 1, "the change after detaching was not heard")
  assert(not seen.listener:detach(), "detaching twice reports it was spent")
end)

test("a listener that detaches another is not heard from again", function()
  local s = signal.new(1)
  local first_ran = 0
  local second = nil
  s:on(function()
    first_ran = first_ran + 1
    second:detach()
  end)
  local second_ran = 0
  second = s:on(function()
    second_ran = second_ran + 1
  end)

  s:set(2)
  assert(first_ran == 1)
  assert(second_ran == 0, "detached before its turn came round")
end)

test("both signals true, and only when the answer moves", function()
  local a, b = signal.new(false), signal.new(false)
  local both = a & b
  local seen = watch(both)
  assert(both:get() == false)

  a:set(true)
  assert(both:get() == false, "one of two is not both")
  assert(seen.count == 0, "the answer did not move, so nothing fired")

  b:set(true)
  assert(both:get() == true)
  assert(seen.count == 1)

  a:set(false)
  assert(both:get() == false)
  assert(seen.count == 2)
end)

test("either signal true", function()
  local a, b = signal.new(false), signal.new(false)
  local either = a | b
  local seen = watch(either)

  a:set(true)
  assert(either:get() == true)
  assert(seen.count == 1)

  b:set(true)
  assert(either:get() == true)
  assert(seen.count == 1, "already true, so the answer did not move")
end)

test("the opposite of a signal", function()
  local a = signal.new(false)
  local not_a = ~a
  assert(not_a:get() == true)
  a:set(true)
  assert(not_a:get() == false)
end)

test("anything but false and nil counts as true", function()
  local a = signal.new(0)
  local b = signal.new("")
  assert((a & b):get() == true, "zero and the empty string are true in Lua")
  a:set(nil)
  assert((a & b):get() == false)
end)

test("combinations nest", function()
  local ui, playing, cutscene =
    signal.new(true), signal.new(true), signal.new(false)
  local allowed = ui & playing & ~cutscene
  assert(allowed:get() == true)

  cutscene:set(true)
  assert(allowed:get() == false, "a cutscene takes the screen")
  cutscene:set(false)
  assert(allowed:get() == true)

  ui:set(false)
  assert(allowed:get() == false)
end)

test("whether a signal holds a given value", function()
  local status = signal.new("holstered")
  local armed = status:eq("ready")
  local seen = watch(armed)
  assert(armed:get() == false)

  status:set("drawing")
  assert(seen.count == 0, "still not ready, so the answer did not move")

  status:set("ready")
  assert(armed:get() == true)
  assert(seen.count == 1)
end)

test("whether a signal holds more than a number", function()
  local poison = signal.new(0)
  local poisoned = poison:above(0)
  local seen = watch(poisoned)
  assert(poisoned:get() == false)

  poison:set(4)
  assert(poisoned:get() == true)
  assert(seen.count == 1)

  poison:set(9)
  assert(seen.count == 1, "still poisoned, so the answer did not move")

  poison:set(0)
  assert(poisoned:get() == false)
  assert(seen.count == 2)
end)

test("a value that is not a number is not above one", function()
  local s = signal.new(nil)
  assert(s:above(0):get() == false)
end)

test("a signal of a script's own reads what a combination does", function()
  -- The same derived signal can be read directly and listened to for changes.
  local a, b = signal.new(true), signal.new(true)
  local rule = a & b
  local woken = 0
  rule:on(function()
    woken = woken + 1
  end)

  assert(rule:get() == true, "read as a value")
  b:set(false)
  assert(woken == 1, "and heard as a change")
  assert(rule:get() == false)
end)

test("a setting speaks through a signal", function()
  local music = signal.config("audio.enable_music")
  assert(music:get() == trx.config.get("audio.enable_music"))

  local seen = watch(music)
  trx.config.set("audio.enable_music", false)
  assert(music:get() == false)
  assert(seen.count == 1)

  trx.config.set("audio.enable_music", false)
  assert(seen.count == 1, "set to what it holds, so nothing moved")

  trx.config.set("audio.enable_music", true)
  assert(music:get() == true, "heard a second change")
  assert(seen.count == 2)
end)

test("one signal per setting, however often it is asked for", function()
  assert(signal.config("visuals.fov") == signal.config("visuals.fov"))
end)

test("a setting combines with a signal of a script's own", function()
  trx.config.set("audio.enable_music", true)
  local wanted = signal.new(true)
  local both = signal.config("audio.enable_music") & wanted
  assert(both:get() == true)

  trx.config.set("audio.enable_music", false)
  assert(both:get() == false, "the setting is what moved")

  trx.config.set("audio.enable_music", true)
  wanted:set(false)
  assert(both:get() == false, "and now the script's own")
end)

test("the tick signal counts, so it says something new every time", function()
  local frame = signal.tick
  local seen = watch(frame)
  local first = frame:get()

  frame:set(first + 1)
  assert(seen.count == 1)
  frame:set(first + 2)
  assert(seen.count == 2, "a counter never sets what it already holds")

  seen.listener:detach()
end)

test("a polled signal reads once a tick and reports a move", function()
  local value = 1
  local polled = signal.polled(function()
    return value
  end)
  local seen = watch(polled)
  assert(polled:get() == 1)

  signal.tick:set(signal.tick:get() + 1)
  assert(seen.count == 0, "nothing moved, so nothing was said")

  value = 2
  signal.tick:set(signal.tick:get() + 1)
  assert(polled:get() == 2)
  assert(seen.count == 1)

  seen.listener:detach()
end)

test(
  "a mapped signal holds what the function made of the one below it",
  function()
    local source = signal.new(4)
    local doubled = source:map(function(value)
      return value * 2
    end)
    assert(doubled:get() == 8)

    local seen = watch(doubled)
    source:set(5)
    assert(doubled:get() == 10)
    assert(seen.count == 1)

    seen.listener:detach()
  end
)

test(
  "a mapped signal says nothing where its own answer did not move",
  function()
    local source = signal.new(1)
    local low = source:map(function(value)
      return value < 10
    end)
    local seen = watch(low)

    source:set(2)
    assert(seen.count == 0, "still below ten, so the answer stood")

    source:set(20)
    assert(seen.count == 1)

    seen.listener:detach()
  end
)

test("a combined signal reads every source, in the order given", function()
  local a = signal.new(1)
  local b = signal.new(2)
  local c = signal.new(3)
  local sum = signal.combine(a, b, c, function(x, y, z)
    return x + y + z
  end)
  assert(sum:get() == 6)

  b:set(20)
  assert(sum:get() == 24)
end)

test("a stopped signal holds what it had and follows nothing", function()
  local source = signal.new(1)
  local doubled = source:map(function(value)
    return value * 2
  end)

  assert(doubled:stop(), "it was following the source")
  source:set(5)
  assert(doubled:get() == 2, "it kept what it last held")
  assert(not doubled:stop(), "there is nothing left to stop")
end)

test("a polled signal stops reading once it is stopped", function()
  local reads = 0
  local polled = signal.polled(function()
    reads = reads + 1
    return reads
  end)

  signal.tick:set(signal.tick:get() + 1)
  local after_one = reads
  assert(after_one > 1, "the tick read it")

  polled:stop()
  signal.tick:set(signal.tick:get() + 1)
  assert(reads == after_one, "no further tick reached it")
end)

test("a level script's derived signal stops when the level ends", function()
  local source = signal.new(1)
  local doubled
  fake.as_level_script(function()
    doubled = source:map(function(value)
      return value * 2
    end)
  end)

  fake.end_level()
  source:set(5)
  assert(doubled:get() == 2, "it stopped following the source")
end)

test("a global script's derived signal outlives a level", function()
  local source = signal.new(1)
  local doubled = source:map(function(value)
    return value * 2
  end)

  fake.end_level()
  source:set(5)
  assert(doubled:get() == 10, "it still follows the source")
end)

-- The engine takes trxc off the globals once the API is sealed, so a module
-- that reads it when a script calls in, rather than when the module loads,
-- raises at the worst moment.
test("a signal is made after trxc leaves the globals", function()
  local capi = trxc
  trxc = nil
  local ok, err = pcall(signal.polled, function()
    return 1
  end)
  trxc = capi
  assert(ok, err)
end)

return h.report()
