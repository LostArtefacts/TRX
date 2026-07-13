-- The sound API as a script actually sees it.

local h = require("harness")
local test, raises = h.test, h.raises

test("is_available answers for the samples the level has", function()
  assert(trx.sound.is_available(fake.SAMPLE) == true)
  assert(trx.sound.is_available(fake.MISSING_SAMPLE) == false)
end)

test("play with no position plays at full volume", function()
  trx.sound.play(fake.SAMPLE)
  local calls = fake.calls()
  assert(calls.play_count == 1, "play did not reach the engine")
  assert(calls.last_sample == fake.SAMPLE)
  assert(calls.had_pos == false, "no position means no position, not the origin")
end)

test("opts.pos plays the sound in the world", function()
  trx.sound.play(fake.SAMPLE, { pos = { x = 100, y = 200, z = 50 } })
  local calls = fake.calls()
  assert(calls.had_pos == true, "the position did not reach the engine")
  assert(calls.last_x == 100 and calls.last_y == 200 and calls.last_z == 50)
end)

test("an unavailable sample raises rather than playing silence", function()
  raises(function()
    trx.sound.play(fake.MISSING_SAMPLE)
  end)
  assert(fake.calls().play_count == 0, "an unavailable sample must not reach the engine")
end)

test("stop names the sample, stop_all names none", function()
  trx.sound.stop(fake.SAMPLE)
  local calls = fake.calls()
  assert(calls.stop_count == 1)
  assert(calls.last_stopped_sample == fake.SAMPLE)

  trx.sound.stop_all()
  assert(fake.calls().stop_all_count == 1)
end)

return h.report()
