-- The music API as a script actually sees it.

local h = require("harness")
local test, raises = h.test, h.raises

test("a track plays once by default, and hands back its stream", function()
  local stream = trx.music.tracks[fake.TRACK]:play()
  local calls = fake.calls()
  assert(calls.play.count == 1, "play did not reach the engine")
  assert(calls.play.track == fake.TRACK)
  assert(
    calls.play.mode == trx.music.PlayMode.ONCE,
    "the default mode must be ONCE"
  )
  assert(stream ~= nil and stream:is_valid(), "play hands back the stream")
  assert(stream.track_num == fake.TRACK)
end)

test("opts.mode selects the play mode", function()
  trx.music.tracks[fake.TRACK]:play({ mode = trx.music.PlayMode.LOOP })
  assert(fake.calls().play.mode == trx.music.PlayMode.LOOP)

  trx.music.tracks[fake.TRACK]:play({ mode = trx.music.PlayMode.OVERLAY })
  assert(fake.calls().play.mode == trx.music.PlayMode.OVERLAY)
end)

test("an empty opts table still plays once", function()
  trx.music.tracks[fake.TRACK]:play({})
  assert(fake.calls().play.mode == trx.music.PlayMode.ONCE)
end)

test("get_track reports what is playing, and nil when nothing is", function()
  assert(trx.music.current_track == nil, "nothing plays to begin with")

  trx.music.tracks[fake.TRACK]:play()
  assert(trx.music.current_track.num == fake.TRACK)

  trx.music.stop()
  assert(
    trx.music.current_track == nil,
    "a stopped soundtrack must report nil"
  )
end)

test("pause, unpause and stop reach the engine", function()
  trx.music.pause()
  trx.music.unpause()
  trx.music.stop()
  local calls = fake.calls()
  assert(calls.pause.count == 1)
  assert(calls.unpause.count == 1)
  assert(calls.stop.count == 1)
end)

test("play is the only name for playing a track", function()
  assert(trx.music.play_track == nil)
end)

test("looped_track reports the ambient track, nil when none", function()
  assert(trx.music.looped_track == nil, "no ambient track to begin with")

  fake.set_looped(fake.TRACK)
  assert(trx.music.looped_track.num == fake.TRACK)
end)

test("tracks is keyed by id, nil for a track the level lacks", function()
  assert(trx.music.tracks[fake.TRACK].num == fake.TRACK)
  assert(trx.music.tracks[fake.MISSING_TRACK] == nil)
  assert(#trx.music.tracks == 1, "the fake soundtrack has exactly one track")
end)

test("iterating tracks walks the available ones by id", function()
  local seen = {}
  for id, track in pairs(trx.music.tracks) do
    seen[id] = track.num
  end
  assert(seen[fake.TRACK] == fake.TRACK)
  assert(next(seen, next(seen)) == nil, "only one track is available")
end)

test("a track plays itself and resolves its path", function()
  trx.music.tracks[fake.TRACK]:play({ mode = trx.music.PlayMode.LOOP })
  local calls = fake.calls()
  assert(calls.play.track == fake.TRACK)
  assert(calls.play.mode == trx.music.PlayMode.LOOP)
  assert(trx.music.tracks[fake.TRACK]:path() == "music/track05.flac")
end)

test("streams holds a place for every slot, main first", function()
  local streams = trx.music.streams
  assert(#streams == 4, "one main slot and three overlay slots")
end)

test("a playing stream reports its track, mode and timestamp", function()
  fake.set_stream(0, fake.TRACK, trx.music.PlayMode.LOOP, 12.5)
  local main = trx.music.streams[1]
  assert(main:is_valid(), "slot 0 is playing")
  assert(main.track_num == fake.TRACK)
  assert(main.mode == trx.music.PlayMode.LOOP)
  assert(main.timestamp == 12.5)
end)

test("a silent slot is stale, and reading it raises", function()
  local overlay = trx.music.streams[2]
  assert(not overlay:is_valid(), "nothing plays on the first overlay")
  raises(function()
    return overlay.track_num
  end)
end)

test("pause, unpause, seek and stop reach the slot", function()
  fake.set_stream(1, fake.TRACK, trx.music.PlayMode.OVERLAY, 0.0)
  local overlay = trx.music.streams[2]

  overlay:pause()
  overlay:unpause()
  assert(overlay:seek(3.5) == true, "seek takes on a playing slot")
  overlay:stop()

  local calls = fake.calls()
  assert(calls.stream_pause.count == 1 and calls.stream_pause.slot == 1)
  assert(calls.stream_unpause.count == 1)
  assert(calls.stream_seek.count == 1 and calls.stream_seek.timestamp == 3.5)
  assert(calls.stream_stop.count == 1 and calls.stream_stop.slot == 1)
end)

test("a method on a stale stream raises", function()
  local overlay = trx.music.streams[3]
  raises(function()
    overlay:stop()
  end)
end)

return h.report()
