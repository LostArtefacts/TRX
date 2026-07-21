-- /music, dispatched through the console. The fake soundtrack has one track,
-- id 5, so this pins that `stop`, a track number, `0` and an unknown id route to
-- the right call - telling the keyword and the number apart is the any_of at
-- work.

local h = require("harness")
local test = h.test
local R = trx.console.Result

local function music(args)
  return fake.run("music", args or "")
end

test("no argument lists the tracks", function()
  assert(music("") == R.OK)
  local c = fake.calls()
  assert(c.play_count == 0)
  assert(c.log_count >= 1, "the listing was written to the console")
end)

test("status reports, without playing or stopping", function()
  assert(music("status") == R.OK)
  local c = fake.calls()
  assert(c.play_count == 0 and c.stop_count == 0)
  assert(c.log_count >= 1, "the status line was written")
end)

test("stop stops the soundtrack", function()
  assert(music("stop") == R.OK)
  assert(fake.calls().stop_count == 1)
end)

test("a track number plays it", function()
  assert(music(tostring(fake.TRACK)) == R.OK)
  local c = fake.calls()
  assert(c.play_count == 1, "the track did not play")
  assert(c.last_track == fake.TRACK)
end)

test("zero stops the soundtrack", function()
  assert(music("0") == R.OK)
  assert(fake.calls().stop_count == 1)
end)

test("negative one stops the soundtrack", function()
  assert(music("-1") == R.OK)
  assert(fake.calls().stop_count == 1)
end)

test("an unknown track plays nothing, and stops nothing", function()
  assert(music(tostring(fake.MISSING_TRACK)) == R.OK)
  local c = fake.calls()
  assert(c.play_count == 0)
  assert(c.stop_count == 0, "a bad id does not stop the soundtrack")
end)

test("a word that is neither keyword nor number is refused", function()
  assert(music("nonsense") == R.BAD_INVOCATION)
  assert(fake.calls().play_count == 0)
end)

return h.report()
