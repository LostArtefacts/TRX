-- The sound API as a script actually sees it.

local h = require("harness")
local test, raises = h.test, h.raises

test("samples is keyed by id, nil for a sample the level lacks", function()
  assert(trx.sound.samples[fake.SAMPLE].id == fake.SAMPLE)
  assert(trx.sound.samples[fake.MISSING_SAMPLE] == nil)
  assert(#trx.sound.samples == 1, "the fake level has exactly one sample")
end)

test("a sample reports its definition and plays itself", function()
  local sample = trx.sound.samples[fake.SAMPLE]
  assert(sample.volume == fake.SAMPLE_VOLUME)

  local voice = sample:play()
  local calls = fake.calls()
  assert(calls.play_count == 1)
  assert(calls.last_sample == fake.SAMPLE)
  assert(voice ~= nil and voice:is_valid(), "play hands back the voice")
  assert(voice.sample_id == fake.SAMPLE)
end)

test("play with no position plays at full volume", function()
  trx.sound.samples[fake.SAMPLE]:play()
  local calls = fake.calls()
  assert(calls.play_count == 1, "play did not reach the engine")
  assert(
    calls.had_pos == false,
    "no position means no position, not the origin"
  )
end)

test("opts.pos plays the sound in the world", function()
  trx.sound.samples[fake.SAMPLE]:play({ pos = { x = 100, y = 200, z = 50 } })
  local calls = fake.calls()
  assert(calls.had_pos == true, "the position did not reach the engine")
  assert(calls.last_x == 100 and calls.last_y == 200 and calls.last_z == 50)
end)

-- A missing coordinate used to read as zero, so a typo put the sound at the
-- edge of the world rather than saying so.
test("a position missing a coordinate raises", function()
  local sample = trx.sound.samples[fake.SAMPLE]
  raises(function()
    sample:play({ pos = { y = 200 } })
  end, "x")
  raises(function()
    sample:play({ pos = { x = 1, y = 2, z = "far" } })
  end, "z")
  assert(fake.calls().play_count == 0)
end)

test("a sample stops itself", function()
  trx.sound.samples[fake.SAMPLE]:stop()
  local calls = fake.calls()
  assert(calls.stop_count == 1)
  assert(calls.last_stopped_sample == fake.SAMPLE)
end)

test("streams reaches the playing voices and controls them", function()
  fake.set_stream(1, fake.SAMPLE)
  local voice = trx.sound.streams[2]
  assert(voice:is_valid(), "slot 1 is playing")
  assert(voice.sample_id == fake.SAMPLE)

  voice:pause()
  voice:unpause()
  voice:stop()
  local calls = fake.calls()
  assert(calls.slot_pause_count == 1 and calls.slot_pause_slot == 1)
  assert(calls.slot_unpause_count == 1)
  assert(calls.slot_stop_count == 1 and calls.slot_stop_slot == 1)
end)

test("a silent voice is stale, and reading it raises", function()
  local voice = trx.sound.streams[1]
  assert(not voice:is_valid(), "nothing plays on the first slot")
  raises(function()
    return voice.sample_id
  end)
end)

test("stop_all names none", function()
  trx.sound.stop_all()
  assert(fake.calls().stop_all_count == 1)
end)

return h.report()
