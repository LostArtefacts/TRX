-- The event API as a script actually sees it.
--
-- Everything under these assertions is real: trxc.events, the listener registry
-- in lua/events.c, and data/scripting/events.lua itself. `fake.fire()` calls the
-- same Lua_FireEvent* entrypoints the engine calls, so a handler here sees
-- exactly what a handler sees in the game.

local h = require("harness")
local test, raises = h.test, h.raises

test("a handler fires, and stops firing once detached", function()
  local calls = 0
  local id = trx.events.before_control(function()
    calls = calls + 1
  end)

  fake.fire("before_control")
  fake.fire("before_control")
  assert(calls == 2, "handler did not fire twice")

  assert(trx.events.detach(id) == true, "detach must report the removal")
  fake.fire("before_control")
  assert(calls == 2, "a detached handler kept firing")
end)

test("detaching twice reports that there was nothing to remove", function()
  local id = trx.events.after_control(function() end)
  assert(trx.events.detach(id) == true)
  assert(trx.events.detach(id) == false, "the second detach must report false")
  assert(trx.events.detach(99999) == false, "an id never handed out")
end)

test("the control events pass no arguments", function()
  -- They used to be fired with a meaningless 0. A handler must see nothing.
  local seen = "unset"
  trx.events.before_control(function(...)
    seen = select("#", ...)
  end)
  fake.fire("before_control")
  assert(seen == 0, "before_control handed the handler an argument")
end)

test("the level events pass the level number", function()
  local events = {
    "before_level_file",
    "after_level_file",
    "before_item_setup",
    "after_item_setup",
    "after_level_state",
  }
  for _, name in ipairs(events) do
    local seen = nil
    trx.events[name](function(level_num)
      seen = level_num
    end)
    fake.fire(name, 7)
    assert(seen == 7, name .. " did not receive the level number")
  end
end)

test("on_pickup passes the item number", function()
  local seen = nil
  trx.events.on_pickup(function(item_num)
    seen = item_num
  end)
  fake.fire("on_pickup", 42)
  assert(seen == 42, "on_pickup did not receive the item number")
end)

test("on_game_start passes the level number and the savegame flag", function()
  local seen_level, seen_save = nil, nil
  trx.events.on_game_start(function(level_num, is_save)
    seen_level, seen_save = level_num, is_save
  end)

  fake.fire("on_game_start", 3, true)
  assert(seen_level == 3, "wrong level number")
  assert(seen_save == true, "is_save must be a boolean, not a truthy number")

  fake.fire("on_game_start", 3, false)
  assert(seen_save == false, "a fresh start must report false")
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
  assert(#order == 2 and order[1] == "first" and order[2] == "second", "attach order")
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
  assert(level_calls == 1 and global_calls == 1, "both should fire while the level runs")

  fake.end_level()
  fake.fire("before_control")
  assert(level_calls == 1, "a level handler outlived its level")
  assert(global_calls == 2, "a global handler was dropped with the level")
end)

test("attaching something that is not a function raises", function()
  raises(function()
    trx.events.before_control(42)
  end)
  raises(function()
    trx.events.before_control()
  end)
end)

test("the event type is not part of the surface", function()
  -- The nine hooks are the whole API. EventType and the `_type` field on the
  -- callable proxies were internals that leaked; nothing may reach them.
  assert(trx.events.EventType == nil, "EventType is still reachable")
  assert(trx.events.attach == nil, "raw attach must not be public")

  -- A hook is a plain function, not a table carrying its event type around.
  assert(type(trx.events.before_control) == "function", "a hook must be a function")
  raises(function()
    return trx.events.before_control._type
  end)
end)

return h.report()
