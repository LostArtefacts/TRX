-- How the shipped ring moves its entries, and its camera as it leaves for
-- another ring.

local h = require("harness")
local test = h.test

local motion = require("common.ui.ring.motion")

-- Where the hand turn stands among the steps of an entry's pose.
local HAND_STEP = 7

local function new_view()
  return motion.new({ entries = { {} } })
end

local function near(a, b)
  return math.abs(a.x - b.x) <= 8
    and math.abs(a.y - b.y) <= 8
    and math.abs(a.z - b.z) <= 8
end

test("a hand turn moves smoothly between ticks", function()
  local view = new_view()
  local hand = view.entries[1].hand
  view:turn_by_hand(1000, 0)
  view:tick(motion.Spin.HELD)
  -- The pose is drawn part way between the last two steps.
  assert(hand.last.w == 1, "the last step moved")
  assert(hand.now.w < 1, "the next step did not move")
end)

test("each hand turn is about the camera's axes", function()
  local view = new_view()
  local yaw, pitch = trx.math.DEG_90, 4096
  view:turn_by_hand(yaw, 0)
  view:tick(motion.Spin.HELD)
  view:turn_by_hand(0, pitch)
  view:tick(motion.Spin.HELD)
  view:tick(motion.Spin.HELD)
  local rot = view:pose(1)[HAND_STEP].turn
  local v = { x = 1000, y = 2000, z = 3000 }
  local turned = trx.math.rotate(v, { x = 0, y = yaw, z = 0 })
  local expected = trx.math.rotate(turned, { x = -pitch, y = 0, z = 0 })
  local got = trx.math.rotate(v, rot)
  assert(near(got, expected), ("%d %d %d"):format(got.x, got.y, got.z))
end)

test("a turn of nothing leaves the entry settling on its own", function()
  local view = new_view()
  local entry = view.entries[1]
  view:turn_by_hand(0, 0)
  assert(not entry.turned, "an empty turn held the entry")
end)

test("an entry turned by hand stops spinning between ticks", function()
  local view = new_view()
  local entry = view.entries[1]
  view:tick(motion.Spin.IDLE)
  view:turn_by_hand(1000, 0)
  view:tick(motion.Spin.HELD)
  local y_rot = entry.y_rot --[[@as table]]
  assert(y_rot.last == y_rot.now, "the spin went on between ticks")
end)

test("an entry let go turns back while the ring spins", function()
  local view = new_view()
  local hand = view.entries[1].hand
  view:turn_by_hand(2 * trx.math.DEG_90 - 1, 0)
  view:tick(motion.Spin.HELD)
  view:let_go()
  view:tick(motion.Spin.HELD)
  for _ = 1, 30 do
    view:tick(motion.Spin.IDLE)
  end
  assert(
    math.abs(hand.now.w) > 0.9999,
    ("the turn stayed at %f"):format(hand.now.w)
  )
end)

test("a ring wholly below the camera leaves by a small tilt", function()
  local view = motion.new({
    entries = {
      {
        bounds = {
          min = { x = -64, y = -288, z = -64 },
          max = { x = 64, y = -1, z = 64 },
        },
      },
    },
  })
  view.camera_y:set(motion.RING.camera_height)
  view:start(motion.Motion.LEAVE_UP)
  assert(
    view.left_by > 0 and view.left_by < trx.math.DEG_90,
    ("the ring left by %d"):format(view.left_by)
  )
end)

return h.report()
