-- The assault course API as a script actually sees it.
--
-- trx.assault.stats is an api.namespace that is not callable, and every verb
-- takes an optional track, so this is where the default-track path is pinned.

local h = require("harness")
local test, raises = h.test, h.raises

test("a verb with no track means the assault course", function()
  trx.assault.start()
  local calls = fake.calls()
  assert(calls.start.count == 1, "start did not reach the track manager")
  assert(
    calls.start.track == trx.assault.Track.COURSE,
    "the default track must be the course"
  )
end)

test("a verb takes the track it is given", function()
  trx.assault.start(trx.assault.Track.QUAD)
  assert(fake.calls().start.track == trx.assault.Track.QUAD)

  trx.assault.stop(trx.assault.Track.COURSE)
  local calls = fake.calls()
  assert(calls.stop.count == 1)
  assert(calls.stop.track == trx.assault.Track.COURSE)
end)

test("finish is not stop", function()
  -- Completing the track and abandoning it are different endings; the surface
  -- could reach only the second one.
  trx.assault.finish()
  local calls = fake.calls()
  assert(calls.finish.count == 1, "finish did not reach the track manager")
  assert(calls.stop.count == 0, "finish must not be a stop")
end)

test("reset clears the timer", function()
  trx.assault.reset()
  assert(fake.calls().reset.count == 1)
end)

test("the timers do not exist outside a gym level", function()
  fake.set_in_gym(false)
  for _, verb in ipairs({ "start", "stop", "reset", "finish" }) do
    raises(function()
      trx.assault[verb]()
    end)
  end
  assert(
    fake.calls().start.count == 0,
    "a verb outside the gym must not reach the engine"
  )
end)

test(
  "is_running and is_visible answer without raising outside a gym",
  function()
    fake.set_in_gym(false)
    assert(trx.assault.is_running() == false)
    assert(trx.assault.is_visible() == false)
  end
)

test("is_running follows the timer", function()
  assert(trx.assault.is_running() == false)
  fake.set_running(trx.assault.Track.COURSE, true)
  assert(trx.assault.is_running() == true)
  -- Per track, not global.
  assert(trx.assault.is_running(trx.assault.Track.QUAD) == false)
end)

test(
  "is_visible is not is_running: a stopped timer stays on screen",
  function()
    fake.set_visible(trx.assault.Track.COURSE, true)
    assert(trx.assault.is_visible() == true)
    assert(trx.assault.is_running() == false)
  end
)

test("active_track is nil when Lara is running nothing", function()
  assert(trx.assault.active_track == nil, "no track means nil, not zero")

  -- QUAD is 0 in the C enum, and a bridge testing for truth would report it as
  -- nothing.
  fake.set_active_track(trx.assault.Track.QUAD)
  assert(
    trx.assault.active_track == trx.assault.Track.QUAD,
    "QUAD is 0 and must survive"
  )

  fake.set_active_track(trx.assault.Track.COURSE)
  assert(trx.assault.active_track == trx.assault.Track.COURSE)
end)

test("active_track is read-only", function()
  raises(function()
    trx.assault.active_track = trx.assault.Track.QUAD
  end, "read-only")
end)

test(
  "records go in fastest first, and carry the attempt they came from",
  function()
    assert(trx.assault.stats.add_record(30.0) == true)
    assert(trx.assault.stats.add_record(20.0) == true)
    assert(trx.assault.stats.add_record(40.0) == true)

    local records = trx.assault.stats.list_records()
    assert(#records == 3, "wrong number of records")
    assert(records[1].time == 20.0, "records must be sorted by time")
    assert(records[2].time == 30.0)
    assert(records[3].time == 40.0)

    -- The attempt number counts entries filed, not the position in the table.
    assert(records[1].attempt_num == 2, "the 20s run was the second attempt")
    assert(records[2].attempt_num == 1)
    assert(records[3].attempt_num == 3)
  end
)

test("a record is written through to the player's profile", function()
  trx.assault.stats.add_record(30.0)
  assert(fake.calls().config_write.count == 1, "the record was not persisted")
end)

test("removing a record closes the gap behind it", function()
  trx.assault.stats.add_record(10.0)
  trx.assault.stats.add_record(20.0)
  trx.assault.stats.add_record(30.0)

  assert(trx.assault.stats.remove_record(2) == true)

  local records = trx.assault.stats.list_records()
  assert(#records == 2)
  assert(records[1].time == 10.0)
  assert(records[2].time == 30.0, "the gap was not closed")
end)

test("removing a record that is not there reports false", function()
  assert(
    trx.assault.stats.remove_record(1) == false,
    "there is nothing to remove"
  )

  trx.assault.stats.add_record(10.0)
  assert(trx.assault.stats.remove_record(2) == false, "slot 2 is empty")
  assert(trx.assault.stats.remove_record(1) == true)
end)

test("records are 1-based, and out of range raises", function()
  raises(function()
    trx.assault.stats.remove_record(0)
  end)
  raises(function()
    trx.assault.stats.remove_record(11)
  end)
end)

test("a time of zero or less raises", function()
  raises(function()
    trx.assault.stats.add_record(0)
  end)
  raises(function()
    trx.assault.stats.add_record(-1)
  end)
end)

-- Every track keeps its own record table. Filing one against the quad bike used
-- to land in the assault course's.
test("each track keeps its own records", function()
  local COURSE, QUAD = trx.assault.Track.COURSE, trx.assault.Track.QUAD

  trx.assault.stats.add_record(30.0)
  trx.assault.stats.add_record(12.5, QUAD)

  local course = trx.assault.stats.list_records(COURSE)
  assert(#course == 1 and course[1].time == 30.0, "the course record")

  local quad = trx.assault.stats.list_records(QUAD)
  assert(
    #quad == 1 and quad[1].time == 12.5,
    "the quad record went somewhere else"
  )

  -- Omitting the track still means the course, as it does everywhere else.
  assert(#trx.assault.stats.list_records() == 1)
  assert(trx.assault.stats.list_records()[1].time == 30.0)

  assert(trx.assault.stats.remove_record(1, QUAD) == true)
  assert(
    #trx.assault.stats.list_records(QUAD) == 0,
    "the quad record was not removed"
  )
  assert(
    #trx.assault.stats.list_records(COURSE) == 1,
    "the course record went with it"
  )
end)

-- The records are in the player's profile, not in the level.
test("records can be read outside a gym level", function()
  trx.assault.stats.add_record(30.0)
  fake.set_in_gym(false)

  assert(#trx.assault.stats.list_records() == 1)
  assert(trx.assault.stats.add_record(10.0) == true)
end)

-- Which game this is decides whether a track has a record table at all.
test("a track this game has no records for raises", function()
  fake.set_has_stats(trx.assault.Track.QUAD, false)
  raises(function()
    trx.assault.stats.list_records(trx.assault.Track.QUAD)
  end, "unavailable")
  raises(function()
    trx.assault.stats.add_record(10.0, trx.assault.Track.QUAD)
  end, "unavailable")

  -- And the course is untouched by it.
  assert(trx.assault.stats.add_record(10.0) == true)
end)

test("stats is a table, not a function", function()
  assert(type(trx.assault.stats) == "table")
end)

return h.report()
