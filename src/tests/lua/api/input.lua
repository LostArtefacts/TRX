-- The input API as a script actually sees it.

local h = require("harness")
local test, raises = h.test, h.raises

local input = trx.input
local Role = input.Role

test("a press is not a hold", function()
  assert(input.is_held(Role.JUMP) == false)
  assert(input.is_pressed(Role.JUMP) == false)

  fake.set_held(true)
  assert(input.is_held(Role.JUMP) == true)
  assert(input.is_pressed(Role.JUMP) == false, "holding is not pressing")

  fake.set_pressed(true)
  assert(input.is_pressed(Role.JUMP) == true)
end)

test("taking a press reaches the engine", function()
  input.hold_off(Role.JUMP)
  local calls = fake.calls()
  assert(calls.hold_off.count == 1)
  assert(calls.hold_off.role == Role.JUMP)
end)

test("a role that is not one raises", function()
  for _, fn in ipairs({ input.is_held, input.is_pressed, input.hold_off }) do
    raises(function()
      fn(-1)
    end, "unknown input role")
    raises(function()
      fn(9999)
    end, "unknown input role")
  end
end)

test("a bound role has a glyph, and an unbound one has none", function()
  assert(input.has_glyph(Role.JUMP) == true)
  assert(input.key_name(Role.JUMP) == "K")

  fake.set_bound(false)
  assert(input.has_glyph(Role.JUMP) == false)
  assert(input.key_name(Role.JUMP) == nil, "an unbound role has no key")

  fake.set_bound(true)
  assert(input.has_glyph(Role.JUMP, { slot = 2 }) == false)
end)

test("the second key of a role is its own binding", function()
  assert(input.key_name(Role.JUMP, 1) == "K")
  assert(input.key_name(Role.JUMP, 2) == nil, "nothing is in the second slot")
  assert(input.key_name(Role.JUMP, { slot = 2 }) == nil)
end)

test("slots are counted from one", function()
  raises(function()
    input.key_name(Role.JUMP, 0)
  end, "slot")
  raises(function()
    input.key_name(Role.JUMP, 3)
  end, "slot")
end)

test("the roles and the layouts are named", function()
  assert(input.role_name(Role.JUMP) == "Jump")
  assert(input.layout_name() == "Default", "the current layout answers")
  assert(input.layout_name(input.Layout.CUSTOM_2) == "Custom 2")
end)

test("the current device answers where none is named", function()
  assert(input.backend == input.Backend.KEYBOARD)
  assert(input.layout == input.Layout.DEFAULT)
  assert(input.is_backend_enabled(input.Backend.KEYBOARD) == true)
  assert(input.is_backend_enabled(input.Backend.CONTROLLER) == false)
end)

test("the device and the layout are read-only", function()
  raises(function()
    input.backend = input.Backend.TOUCH
  end, "read-only")
  raises(function()
    input.layout = input.Layout.CUSTOM_1
  end, "read-only")
end)

test("a conflict is reported per layout", function()
  assert(input.is_conflicted(Role.JUMP) == false)
  fake.set_conflicted(true)
  assert(input.is_conflicted(Role.JUMP, input.Backend.KEYBOARD) == true)
  assert(
    input.is_conflicted(Role.JUMP, { backend = input.Backend.KEYBOARD })
      == true
  )
end)

test("listening is turned on and off", function()
  assert(input.is_listening == false)
  input.listen(true)
  assert(input.is_listening == true)
  input.listen(false)
  assert(input.is_listening == false)
end)

test("scoped listening is always turned off", function()
  local a, b = input.with_listen(function()
    assert(input.is_listening == true)
    return "taken", 4
  end)
  assert(a == "taken")
  assert(b == 4)
  assert(input.is_listening == false)

  raises(function()
    input.with_listen(function()
      assert(input.is_listening == true)
      error("boom", 0)
    end)
  end, "boom")
  assert(input.is_listening == false)

  input.listen(true)
  input.with_listen(function()
    assert(input.is_listening == true)
    input.listen(false)
  end)
  assert(input.is_listening == true)
  input.listen(false)
end)

test("turning listening off twice reaches the engine once", function()
  -- Leaving listen mode resets the debounce, so a spurious exit would swallow
  -- a press the game has not read yet.
  input.listen(false)
  assert(fake.calls().exit_listen.count == 0)

  input.listen(true)
  input.listen(false)
  assert(fake.calls().exit_listen.count == 1)
  input.listen(false)
  assert(fake.calls().exit_listen.count == 1)
end)

test("binding takes the key the player is holding", function()
  assert(
    input.bind_pressed(Role.JUMP, { layout = input.Layout.CUSTOM_1 }) == true
  )
  local calls = fake.calls()
  assert(calls.bind.count == 1)
  assert(calls.bind.role == Role.JUMP)
  assert(calls.bind.slot == 0, "the engine counts slots from zero")
  assert(calls.bind.backend == input.Backend.KEYBOARD)
  assert(calls.bind.layout == input.Layout.CUSTOM_1)

  -- With no key held, nothing is taken and the script asks again next frame.
  fake.set_anything_down(false)
  assert(input.bind_pressed(Role.JUMP, 1, nil, input.Layout.CUSTOM_1) == false)
end)

test("unbinding leaves the role with no key", function()
  input.unbind(Role.JUMP, {
    slot = 2,
    backend = input.Backend.CONTROLLER,
    layout = input.Layout.CUSTOM_2,
  })
  local calls = fake.calls()
  assert(calls.unbind.count == 1)
  assert(calls.unbind.slot == 1)
  assert(calls.unbind.backend == input.Backend.CONTROLLER)
  assert(calls.unbind.layout == input.Layout.CUSTOM_2)
end)

test("a layout goes back to what the game ships with", function()
  input.reset_layout({ layout = input.Layout.CUSTOM_3 })
  assert(fake.calls().reset_layout.layout == input.Layout.CUSTOM_3)
end)

-- The default layout is read-only because custom layouts reset from it.
test("the default layout cannot be written", function()
  for _, call in ipairs({
    function()
      input.bind_pressed(Role.JUMP)
    end,
    function()
      input.unbind(Role.JUMP)
    end,
    function()
      input.reset_layout()
    end,
  }) do
    raises(call, "default layout")
  end
  assert(fake.calls().bind.count == 0, "nothing reached the engine")
end)

test("a role the game holds for itself cannot be bound", function()
  fake.set_rebindable(false)
  raises(function()
    input.bind_pressed(Role.JUMP, 1, nil, input.Layout.CUSTOM_1)
  end, "cannot be rebound")
  assert(input.is_rebindable(Role.JUMP) == false)

  fake.set_unbindable(false)
  raises(function()
    input.unbind(Role.JUMP, 1, nil, input.Layout.CUSTOM_1)
  end, "cannot be unbound")
  assert(input.is_unbindable(Role.JUMP) == false)
end)

-------------------------------------------------------------------------------
-- capturing a key over several frames
-------------------------------------------------------------------------------

local function tick()
  trx.signal.tick:set(trx.signal.tick:get() + 1)
end

test("a capture waits for the player to let go", function()
  fake.set_anything_held(true)
  local bound = nil
  local capture = input.capture(
    Role.JUMP,
    { layout = input.Layout.CUSTOM_1 },
    function(value)
      bound = value
    end
  )

  tick()
  assert(input.is_listening == false, "listening waits for the release")
  assert(fake.calls().bind.count == 0)

  fake.set_anything_held(false)
  tick()
  assert(input.is_listening == true)
  assert(fake.calls().bind.count == 0, "the press comes after the release")

  tick()
  assert(bound == true)
  assert(fake.calls().bind.count == 1)
  assert(fake.calls().bind.role == Role.JUMP)
  assert(fake.calls().bind.layout == input.Layout.CUSTOM_1)
  assert(input.is_listening == false, "the capture turns listening off")
  assert(capture:cancel() == false, "a capture that landed is over")
end)

test("a capture takes nothing until a key is pressed", function()
  fake.set_anything_down(false)
  local bound = nil
  local capture = input.capture(
    Role.JUMP,
    { layout = input.Layout.CUSTOM_1 },
    function(value)
      bound = value
    end
  )

  tick()
  tick()
  assert(bound == nil, "the capture is still waiting")
  assert(fake.calls().bind.count == 1)

  assert(capture:cancel() == true)
  assert(bound == false)
  assert(input.is_listening == false)

  tick()
  assert(fake.calls().bind.count == 1, "a cancelled capture reads nothing")
end)

test("a capture writes no read-only binding", function()
  raises(function()
    input.capture(Role.JUMP, { layout = input.Layout.DEFAULT })
  end, "cannot be changed")

  fake.set_rebindable(false)
  raises(function()
    input.capture(Role.JUMP, { layout = input.Layout.CUSTOM_1 })
  end, "cannot be rebound")
end)

-------------------------------------------------------------------------------
-- the signals
-------------------------------------------------------------------------------

test("the same role hands back the same signal", function()
  assert(input.signals.held(Role.JUMP) == input.signals.held(Role.JUMP))
  assert(input.signals.held(Role.JUMP) ~= input.signals.held(Role.ACTION))
  assert(input.signals.held(Role.JUMP) ~= input.signals.pressed(Role.JUMP))
end)

test("a held signal follows the key down and up", function()
  local seen = {}
  local held = input.signals.held(Role.ROLL)
  held:on(function(value)
    seen[#seen + 1] = value
  end)

  fake.set_held(true)
  tick()
  fake.set_held(false)
  tick()

  assert(#seen == 2, "the signal moved twice")
  assert(seen[1] == true and seen[2] == false)
end)

test("a role signal outlives the level that asked for it", function()
  -- A level-scoped signal is stopped when the level ends, which would leave
  -- every later reader holding the same dead signal.
  local signal = input.signals.held(Role.DRAW)
  assert(signal:stop() == false, "the signal follows no level")

  fake.set_held(true)
  tick()
  assert(signal:get() == true)
  fake.set_held(false)
  tick()
  assert(signal:get() == false)
end)

test("a pressed signal moves for one tick", function()
  local count = 0
  local pressed = input.signals.pressed(Role.LOOK)
  pressed:on(function(value)
    if value then
      count = count + 1
    end
  end)

  fake.set_pressed(true)
  tick()
  -- The engine reports pressed for one frame, no matter how long the key is down.
  fake.set_pressed(false)
  tick()
  tick()

  assert(count == 1, "one press, one listener run")
end)

return h.report()
