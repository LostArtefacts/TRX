-- The event API as a script actually sees it.
--
-- Everything under these assertions is real: trxc.events, the listener registry
-- in lua/events.c, and src/lua/events.lua itself. `fake.fire()` calls the same
-- LUA_FireEvent* entrypoints the engine calls, so a handler here sees exactly
-- what a handler sees in the game.

local h = require("harness")
local test, raises = h.test, h.raises

test("a handler fires, and stops firing once detached", function()
  local calls = 0
  local listener = trx.events.before_control(function()
    calls = calls + 1
  end)

  fake.fire("before_control")
  fake.fire("before_control")
  assert(calls == 2, "handler did not fire twice")

  assert(trx.events.detach(listener) == true, "detach must report the removal")
  fake.fire("before_control")
  assert(calls == 2, "a detached handler kept firing")
end)

test("a listener detaches itself as well", function()
  local calls = 0
  local listener = trx.events.before_control(function()
    calls = calls + 1
  end)

  fake.fire("before_control")
  assert(listener:detach() == true, "the method must report the removal")
  fake.fire("before_control")
  assert(calls == 1, "a detached handler kept firing")
end)

test("detaching twice reports that there was nothing to remove", function()
  local listener = trx.events.after_control(function() end)
  assert(trx.events.detach(listener) == true)
  assert(
    trx.events.detach(listener) == false,
    "the second detach must report false"
  )
  assert(listener:detach() == false, "and so must the method")
end)

test(
  "a listener says which handler it is, and refuses to be edited",
  function()
    local listener = trx.events.after_control(function() end)
    assert(
      math.type(listener.id) == "integer",
      "a listener knows its own number"
    )
    raises(function()
      listener.id = 1
    end, "read-only")
    raises(function()
      listener.nonsense = 1
    end)
  end
)

test("the control events pass no arguments", function()
  local seen = "unset"
  trx.events.before_control(function(...)
    seen = select("#", ...)
  end)
  fake.fire("before_control")
  assert(seen == 0, "before_control handed the handler an argument")
end)

test("on_title_start passes no arguments", function()
  local seen = "unset"
  trx.events.on_title_start(function(...)
    seen = select("#", ...)
  end)
  fake.fire("on_title_start")
  assert(seen == 0, "on_title_start handed the handler an argument")
end)

test("on_pickup passes the item number", function()
  local seen = nil
  trx.events.on_pickup(function(item_num)
    seen = item_num
  end)
  fake.fire("on_pickup", 42)
  assert(seen == 42, "on_pickup did not receive the item number")
end)

test("on_game_start passes the savegame flag", function()
  local seen = nil
  trx.events.on_game_start(function(is_save)
    seen = is_save
  end)

  fake.fire("on_game_start", true)
  assert(seen == true, "is_save must be a boolean, not a truthy number")

  fake.fire("on_game_start", false)
  assert(seen == false, "a fresh start must report false")
end)

test("every handler attached to an event fires, in order", function()
  local order = {}
  trx.events.before_control(function()
    order[#order + 1] = "first"
  end)
  trx.events.before_control(function()
    order[#order + 1] = "second"
  end)
  fake.fire("before_control")
  assert(
    #order == 2 and order[1] == "first" and order[2] == "second",
    "attach order"
  )
end)

test("a handler only fires for its own event", function()
  local calls = 0
  trx.events.before_control(function()
    calls = calls + 1
  end)
  fake.fire("after_control")
  fake.fire("on_pickup", 1)
  assert(calls == 0, "a handler fired for someone else's event")
end)

test(
  "a handler that takes a detached one's place waits for the next event",
  function()
    local second_calls, replacement_calls = 0, 0
    local second_id

    trx.events.before_control(function()
      if second_id == nil then
        return
      end
      trx.events.detach(second_id)
      second_id = nil
      -- Detaching gives the handler's slot in the Lua registry back, and this
      -- attach takes it: the two are told apart by their listener id, not by
      -- the slot they happen to sit in.
      trx.events.before_control(function()
        replacement_calls = replacement_calls + 1
      end)
    end)
    second_id = trx.events.before_control(function()
      second_calls = second_calls + 1
    end)

    fake.fire("before_control")
    assert(second_calls == 0, "a handler detached mid-dispatch still fired")
    assert(
      replacement_calls == 0,
      "a handler attached mid-dispatch fired for that same event"
    )

    fake.fire("before_control")
    assert(replacement_calls == 1, "the new handler never fired")
  end
)

test("a level script's handlers are dropped when the level ends", function()
  local level_calls, global_calls = 0, 0

  trx.events.before_control(function()
    global_calls = global_calls + 1
  end)
  fake.as_level_script(function()
    trx.events.before_control(function()
      level_calls = level_calls + 1
    end)
  end)

  fake.fire("before_control")
  assert(
    level_calls == 1 and global_calls == 1,
    "both should fire while the level runs"
  )

  fake.end_level()
  fake.fire("before_control")
  assert(level_calls == 1, "a level handler outlived its level")
  assert(global_calls == 2, "a global handler was dropped with the level")
end)

-- A handler runs as the script that attached it, so a listener it attaches in
-- turn belongs where it does and goes when it does.
test("a level handler attaches as the level, not as the game", function()
  local nested_calls = 0

  fake.as_level_script(function()
    trx.events.before_control(function()
      trx.events.on_pickup(function()
        nested_calls = nested_calls + 1
      end)
    end)
  end)

  fake.fire("before_control")
  fake.fire("on_pickup", 0)
  assert(nested_calls == 1, "the nested handler never fired")

  fake.end_level()
  fake.fire("on_pickup", 0)
  assert(nested_calls == 1, "a nested handler outlived the level that made it")
end)

test("attaching something that is not a function raises", function()
  raises(function()
    trx.events.before_control(42)
  end)
  raises(function()
    trx.events.before_control()
  end)
end)

test("on_flip_effect claims its number and receives timer and item", function()
  local seen_timer, seen_item = nil, nil
  trx.events.on_flip_effect(62, function(timer, item_num)
    seen_timer, seen_item = timer, item_num
  end)

  assert(
    fake.fire("on_flip_effect", 62, 7, 5),
    "registering must claim the number"
  )
  assert(seen_timer == 7, "the handler did not receive the timer")
  assert(seen_item == 5, "the handler did not receive the item")

  assert(
    not fake.fire("on_flip_effect", 63, 9, 5),
    "a neighbouring number is not claimed"
  )
  assert(seen_timer == 7, "an unclaimed number must not dispatch")
end)

test("handlers on different effect numbers do not cross-fire", function()
  local seen_a, seen_b = 0, 0
  trx.events.on_flip_effect(10, function()
    seen_a = seen_a + 1
  end)
  trx.events.on_flip_effect(20, function()
    seen_b = seen_b + 1
  end)

  fake.fire("on_flip_effect", 10, 0, 0)
  assert(seen_a == 1 and seen_b == 0, "the wrong handler fired")
end)

test("a claim outlives its detached handler", function()
  local calls = 0
  local id = trx.events.on_flip_effect(30, function()
    calls = calls + 1
  end)
  trx.events.detach(id)

  assert(
    fake.fire("on_flip_effect", 30, 0, 0),
    "the claim must stay for the rest of the level"
  )
  assert(calls == 0, "a detached handler kept firing")
end)

test("a rejected callback does not claim the number", function()
  assert(not pcall(trx.events.on_flip_effect, 50, "not a function"))
  assert(
    not fake.fire("on_flip_effect", 50, 0, 0),
    "a failed attach left the number claimed"
  )
end)

test("a level script's claim clears when the level ends", function()
  trx.events.on_flip_effect(40, function() end)
  fake.as_level_script(function()
    trx.events.on_flip_effect(44, function() end)
  end)

  fake.end_level()
  assert(
    not fake.fire("on_flip_effect", 44, 0, 0),
    "a level claim survived the level"
  )
  assert(
    fake.fire("on_flip_effect", 40, 0, 0),
    "a global claim must survive a level change"
  )
end)

-- The sentinel the enum grew is what gives the bridge an upper bound to check
-- against. Reached here through the raw bridge: no hook hands it a bad type.
test("attaching to an event type the engine does not have raises", function()
  raises(function()
    trxc.events.attach(999, function() end)
  end, "unknown event type")
  raises(function()
    trxc.events.attach(-1, function() end)
  end, "unknown event type")
end)

test("the event type is not part of the surface", function()
  -- The hooks are the whole surface: neither the event type nor raw attach
  -- is reachable from a script. The types are reflected out of C, but they stay
  -- behind the hooks.
  assert(trx.events.EventType == nil, "EventType is still reachable")
  assert(trx.events.attach == nil, "raw attach must not be public")

  -- A hook is a plain function, not a table carrying its event type around.
  assert(
    type(trx.events.before_control) == "function",
    "a hook must be a function"
  )
  raises(function()
    return trx.events.before_control._type
  end)
end)

-- An event a module of the surface raises itself, which the zones are the first
-- of. It is an event type like any other from the declaration on.
test("a module can declare an event of its own", function()
  local thing = trxc.events.declare("test_thing")
  assert(
    trxc.events.declare("test_thing") == thing,
    "the same name must name the same event"
  )
  local other = trxc.events.declare("test_other")
  assert(other ~= thing, "two names are two events")

  local heard = {}
  local id = trxc.events.attach(thing, function(carried)
    heard[#heard + 1] = carried
  end)

  trxc.events.fire(thing, 7)
  trxc.events.fire(other, 8)
  assert(#heard == 1 and heard[1] == 7, "a listener hears its own event alone")

  assert(trxc.events.detach(id) == true, "detach takes a declared event's id")
  trxc.events.fire(thing, 9)
  assert(#heard == 1, "a detached listener kept hearing")
end)

test("an event type nobody declared is refused", function()
  raises(function()
    trxc.events.attach(9999, function() end)
  end, "unknown event type")
  raises(function()
    trxc.events.fire(9999)
  end, "unknown event type")
end)

test("a fired event carries the arguments it was given", function()
  local kind = trxc.events.declare("test_carried")
  local seen
  trxc.events.attach(kind, function(...)
    seen = table.pack(...)
  end)

  trxc.events.fire(kind, true, "gate", nil)
  assert(seen.n == 3, "the handler must be given as many as the fire was")
  assert(seen[1] == true and seen[2] == "gate" and seen[3] == nil)

  -- A whole number stays whole, and a fractional one keeps its fraction.
  trxc.events.fire(kind, 7, 1.5)
  assert(seen[1] == 7 and math.type(seen[1]) == "integer")
  assert(seen[2] == 1.5 and math.type(seen[2]) == "float")

  trxc.events.fire(kind)
  assert(seen.n == 0, "an event may carry nothing at all")
end)

test("an event carries four arguments, of the kinds it can hold", function()
  local kind = trxc.events.declare("test_carried_limits")
  trxc.events.fire(kind, 1, 2, 3, 4)

  raises(function()
    trxc.events.fire(kind, 1, 2, 3, 4, 5)
  end, "at most 4 arguments")
  raises(function()
    trxc.events.fire(kind, {})
  end, "carries no value of this type")
  raises(function()
    trxc.events.fire(kind, print)
  end, "carries no value of this type")
end)

-- A handler that raises is reported where a script author can see it: the log
-- alone leaves the game looking as if nothing happened.
test("a handler that raises is reported through the console", function()
  local before = fake.console_shows()
  local listener = trx.events.before_control(function()
    error("boom")
  end)
  fake.fire("before_control")
  listener:detach()

  assert(fake.console_shows() > before, "the console was told nothing")
end)

return h.report()
