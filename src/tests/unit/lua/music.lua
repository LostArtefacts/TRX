-- The music API as a script actually sees it.

local h = require("harness")
local test, raises = h.test, h.raises

test("play defaults to playing the track once", function()
  trx.music.play(fake.TRACK)
  local calls = fake.calls()
  assert(calls.play_count == 1, "play did not reach the engine")
  assert(calls.last_track == fake.TRACK)
  assert(calls.last_mode == trx.music.PlayMode.ONCE, "the default mode must be ONCE")
end)

test("opts.mode selects the play mode", function()
  trx.music.play(fake.TRACK, { mode = trx.music.PlayMode.LOOP })
  assert(fake.calls().last_mode == trx.music.PlayMode.LOOP)

  trx.music.play(fake.TRACK, { mode = trx.music.PlayMode.OVERLAY })
  assert(fake.calls().last_mode == trx.music.PlayMode.OVERLAY)
end)

test("an empty opts table still plays once", function()
  trx.music.play(fake.TRACK, {})
  assert(fake.calls().last_mode == trx.music.PlayMode.ONCE)
end)

test("play does not leak a global", function()
  -- The resolved mode is a local. A missing `local` here would put a global
  -- named `mode` into every script's environment, on every call.
  trx.music.play(fake.TRACK)
  assert(mode == nil, "trx.music.play leaked a global")
end)

test("a track that will not play raises", function()
  raises(function()
    trx.music.play(fake.MISSING_TRACK)
  end)
end)

test("get_track reports what is playing, and nil when nothing is", function()
  assert(trx.music.get_track() == nil, "nothing plays to begin with")

  trx.music.play(fake.TRACK)
  assert(trx.music.get_track() == fake.TRACK)

  trx.music.stop()
  assert(trx.music.get_track() == nil, "a stopped soundtrack must report nil")
end)

test("pause, unpause and stop reach the engine", function()
  trx.music.pause()
  trx.music.unpause()
  trx.music.stop()
  local calls = fake.calls()
  assert(calls.pause_count == 1)
  assert(calls.unpause_count == 1)
  assert(calls.stop_count == 1)
end)

test("play_track is not part of the surface", function()
  -- It was an alias of play, and one name for one thing is enough.
  assert(trx.music.play_track == nil)
end)

return h.report()
