-- Screens: which definition answers, what holding a screen means to the
-- engine, and every way a screen ends.

local h = require("harness")
local test = h.test

local RING_ENTRY = trx.ui.Screen.RING_ENTRY
local CHOICE_NONE = 0
local CHOICE_CANCEL = 1
local CHOICE_CONFIRM = 2

local PASSPORT = trx.catalog.objects.PASSPORT_OPTION
local COMPASS = trx.catalog.objects.COMPASS_OPTION
-- Only the override case defines this entry, from a global script, because a
-- global definition stays for the rest of the suite.
local SOUND = trx.catalog.objects.SOUND_OPTION

local function label(text)
  return trx.ui.widgets.Label({ text = text })
end

-- Defines as a level script, so that ending the level clears the definition
-- for the next case.
local function define(screen, open, options)
  fake.as_level_script(function()
    trx.ui.screens.define(screen, open, options)
  end)
end

local function finish_case()
  fake.release(RING_ENTRY)
  fake.end_level()
  assert(trx.ui.layers.count() == 0, "a layer outlived the case")
end

local function simple(on_ctx)
  return function(ctx)
    if on_ctx ~= nil then
      on_ctx(ctx)
    end
    return ctx:push({ root = label("screen") })
  end
end

test("a screen with no definition stays the engine's", function()
  assert(fake.offer(RING_ENTRY, PASSPORT) == false)
  assert(not fake.is_held(RING_ENTRY))
  finish_case()
end)

test("a definition that pushes a layer holds the screen", function()
  define(RING_ENTRY, simple())
  assert(fake.offer(RING_ENTRY, PASSPORT) == true)
  assert(fake.is_held(RING_ENTRY))
  assert(trx.ui.layers.count() == 1)
  finish_case()
end)

test("a definition that returns nothing leaves the screen", function()
  define(RING_ENTRY, function(ctx)
    ctx:push({ root = label("dropped") })
    return nil
  end)
  assert(fake.offer(RING_ENTRY, PASSPORT) == false)
  assert(trx.ui.layers.count() == 0, "the pushed layer was left open")
  finish_case()
end)

test("a definition must return one of its own layers", function()
  local stray
  define(RING_ENTRY, function(ctx)
    ctx:push({ root = label("own") })
    stray = trx.ui.layers.push({ root = label("stray") })
    return stray
  end)
  assert(fake.offer(RING_ENTRY, PASSPORT) == false)
  assert(fake.errors() == 1)
  stray:close()
  finish_case()
end)

test("an error while opening leaves the screen", function()
  define(RING_ENTRY, function()
    error("broken")
  end)
  assert(fake.offer(RING_ENTRY, PASSPORT) == false)
  assert(fake.errors() == 1)
  finish_case()
end)

test("cancelling ends the screen and closes its layers", function()
  local held
  define(
    RING_ENTRY,
    simple(function(ctx)
      held = ctx
    end)
  )
  fake.offer(RING_ENTRY, PASSPORT)
  assert(held:cancel() == true)
  assert(held:cancel() == false)
  assert(not held.is_held)
  assert(trx.ui.layers.count() == 0)
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CANCEL)
  finish_case()
end)

test("confirming ends the screen as a choice", function()
  local held
  define(
    RING_ENTRY,
    simple(function(ctx)
      held = ctx
    end)
  )
  fake.offer(RING_ENTRY, PASSPORT)
  assert(held:confirm() == true)
  assert(held:confirm() == false)
  assert(held:cancel() == false)
  assert(trx.ui.layers.count() == 0)
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CONFIRM)
  finish_case()
end)

test("the screen ends when the layer it returned closes", function()
  local layers = {}
  define(RING_ENTRY, function(ctx)
    layers[1] = ctx:push({ root = label("one") })
    layers[2] = ctx:push({ root = label("two") })
    return layers[1]
  end)
  fake.offer(RING_ENTRY, PASSPORT)
  layers[1]:close()
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CANCEL)
  assert(not layers[2].is_open, "the other layer outlived the screen")
  finish_case()
end)

test("closing another layer keeps the screen", function()
  local layers = {}
  define(RING_ENTRY, function(ctx)
    layers[1] = ctx:push({ root = label("one") })
    layers[2] = ctx:push({ root = label("two") })
    return layers[1]
  end)
  fake.offer(RING_ENTRY, PASSPORT)
  layers[2]:close()
  assert(fake.take_choice(RING_ENTRY) == CHOICE_NONE)
  assert(fake.is_held(RING_ENTRY))
  finish_case()
end)

test("an error in a screen's layer gives the screen back", function()
  define(RING_ENTRY, function(ctx)
    return ctx:push({
      root = label("broken"),
      on_input = function()
        error("broken")
      end,
    })
  end)
  fake.offer(RING_ENTRY, PASSPORT)
  fake.tick()
  fake.tick()
  assert(fake.errors() == 1)
  assert(fake.take_choice(RING_ENTRY) == CHOICE_CANCEL)
  finish_case()
end)

test("an engine release closes the layers without a choice", function()
  local closed = false
  local held
  define(RING_ENTRY, function(ctx)
    held = ctx
    return ctx:push({
      root = label("released"),
      on_close = function()
        closed = true
      end,
    })
  end)
  fake.offer(RING_ENTRY, PASSPORT)
  fake.release(RING_ENTRY)
  assert(closed)
  assert(not held.is_held)
  assert(trx.ui.layers.count() == 0)
  finish_case()
end)

test("a screen that has ended pushes no more layers", function()
  local held
  define(
    RING_ENTRY,
    simple(function(ctx)
      held = ctx
    end)
  )
  fake.offer(RING_ENTRY, PASSPORT)
  held:cancel()
  h.raises(function()
    held:push({ root = label("late") })
  end, "the screen has ended")
  finish_case()
end)

test("a ring entry definition draws only its own entry", function()
  local seen
  define(
    RING_ENTRY,
    simple(function(ctx)
      seen = ctx.object
    end),
    { object = COMPASS }
  )
  assert(fake.offer(RING_ENTRY, PASSPORT) == false)
  assert(fake.offer(RING_ENTRY, COMPASS) == true)
  assert(seen == COMPASS)
  finish_case()
end)

test("an entry's own definition comes before the general one", function()
  local which
  define(
    RING_ENTRY,
    simple(function()
      which = "general"
    end)
  )
  define(
    RING_ENTRY,
    simple(function()
      which = "compass"
    end),
    { object = COMPASS }
  )
  fake.offer(RING_ENTRY, COMPASS)
  assert(which == "compass")
  fake.release(RING_ENTRY)
  fake.offer(RING_ENTRY, PASSPORT)
  assert(which == "general")
  finish_case()
end)

test("defining a screen twice needs an override", function()
  define(RING_ENTRY, simple())
  h.raises(function()
    define(RING_ENTRY, simple())
  end, "override = true")
  finish_case()
end)

test("an override answers until its level ends", function()
  local which
  trx.ui.screens.define(
    RING_ENTRY,
    simple(function()
      which = "shipped"
    end),
    { object = SOUND }
  )
  define(
    RING_ENTRY,
    simple(function()
      which = "mod"
    end),
    { object = SOUND, override = true }
  )
  fake.offer(RING_ENTRY, SOUND)
  assert(which == "mod")
  fake.release(RING_ENTRY)
  fake.end_level()
  fake.offer(RING_ENTRY, SOUND)
  assert(which == "shipped")
  finish_case()
end)

return h.report()
