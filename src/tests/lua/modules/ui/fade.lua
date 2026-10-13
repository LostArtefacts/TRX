-- The shipped fade module, which every shipped screen fades through.

local h = require("harness")
local test = h.test

local fade = require("common.ui.fade")

local VIEW = trx.ui.Screen.LEVEL_VIEW

local CHOICE_NONE = 0
local CHOICE_CONFIRM = 2

-- A bare screen to hang the fades on.
---@type trx.ui.ScreenContext?
local opened = nil
trx.ui.screens.define(VIEW, function(ctx)
  opened = ctx
  return ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return 1, 1
      end,
      paint = function() end,
    }),
  })
end)

local function ticks(count)
  for _ = 1, count do
    fake.tick()
  end
end

local function open()
  trx.config.set("visuals.enable_fade_effects", true)
  fake.set_timing(VIEW, 10, 4, 4)
  assert(fake.offer(VIEW, 0), "the screen was not taken")
  return assert(opened)
end

local function finish()
  fake.release(VIEW)
  fake.take_choice(VIEW)
  fake.release_all()
  assert(trx.ui.layers.count() == 0, "a layer outlived the screen")
  assert(fake.errors() == 0, "the module raised")
  trx.config.set("visuals.enable_fade_effects", true)
end

-- The alpha of each black that fills the screen, bottom to top.
local function blacks()
  local found = {}
  for _, op in ipairs(fake.scene()) do
    local alpha = op:match("^quad x=0 y=0 z=0 .* color=000000(%x%x)$")
    if alpha ~= nil then
      found[#found + 1] = tonumber(alpha, 16)
    end
  end
  return found
end

local function find(pattern)
  local scene = fake.scene()
  local layers = fake.scene_layers()
  for i, op in ipairs(scene) do
    if op:match(pattern) then
      return op, layers[i]
    end
  end
  return nil
end

test("a screen comes in from black", function()
  local f = fade.screen(open())
  f:enter({ ticks = 4 })
  assert(blacks()[1] == 0xFF, "the screen did not start black")
  ticks(5)
  assert(#blacks() == 0, "the black stayed")
  finish()
end)

test("a screen leaves through the black it came in through", function()
  local f = fade.screen(open())
  f:enter({ ticks = 2 })
  ticks(3)
  f:leave({ ticks = 4, on_done = function() end })
  ticks(5)
  local found = blacks()
  assert(#found == 1, ("%d blacks"):format(#found))
  assert(found[1] == 0xFF, "the screen did not go black")
  finish()
end)

test(
  "a screen that came in from a snapshot leaves through a new black",
  function()
    local f = fade.screen(open())
    f:enter({ layer = { snapshot = true }, ticks = 10 })
    f:leave({ ticks = 2, on_done = function() end })
    ticks(3)
    assert(find("^snapshot ") ~= nil, "the snapshot went")
    assert(blacks()[1] == 0xFF, "no black over the snapshot")
    finish()
  end
)

test("a leave rests for its hold, then ends the screen", function()
  local f = fade.screen(open())
  f:leave({ ticks = 2, hold = 3 })
  ticks(3)
  assert(fake.take_choice(VIEW) == CHOICE_NONE, "it ended before the hold")
  ticks(3)
  assert(fake.take_choice(VIEW) == CHOICE_CONFIRM, "it did not end")
  finish()
end)

test("with fade effects off a leave ends on the next tick", function()
  local f = fade.screen(open())
  trx.config.set("visuals.enable_fade_effects", false)
  f:leave({ ticks = 10, hold = 3 })
  fake.tick()
  assert(fake.take_choice(VIEW) == CHOICE_CONFIRM, "it did not end")
  finish()
end)

test("a screen's own setting decides whether it fades", function()
  local f = fade.screen(open(), {
    enabled = function()
      return false
    end,
  })
  f:enter({ ticks = 10 })
  fake.tick()
  assert(#blacks() == 0, "the screen faded in")
  finish()
end)

test("a later fade cuts the end of an earlier one", function()
  local f = fade.screen(open())
  local entered = 0
  f:enter({
    layer = { snapshot = true },
    ticks = 4,
    on_done = function()
      entered = entered + 1
    end,
  })
  f:leave({ ticks = 10, on_done = function() end })
  ticks(6)
  assert(entered == 0, "the cut fade still ended")
  finish()
end)

return h.report()
