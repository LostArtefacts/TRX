-- How an inventory ring moves: it opens and closes, turns from one entry to
-- the next, swaps for the ring above or below it, and an entry comes forward
-- as the player picks it and goes back again. The inventory ring and the
-- globe both move this way.
--
-- Each motion is declared below as what moves where and how many ticks it
-- takes, so a mod can change one by replacing it in `M.MOTIONS`.
--
-- Usage:
--   local motion = require("common.ui.ring.motion")
--   local view = motion.new({ entries = { spec } })
--   view:start(motion.Motion.OPEN)
--   -- Every tick, which says when the motion ends:
--   if view:tick(motion.Spin.IDLE) then ... end
--   -- While painting:
--   trx.ui.primitive.world_model(object, {
--     camera = view:camera(),
--     pose = view:pose(1),
--   })

local M = {}

local DEG_90 = trx.math.DEG_90
local DEG_180 = 2 * DEG_90
local DEG_360 = 4 * DEG_90
local wrap = trx.math.wrap_angle

-- Rounds to the whole world units and angles that a model is placed in.
---@param value number
---@return integer
local function whole(value)
  return math.floor(value + 0.5)
end

-- Where the ring stands and how its camera sees it, in world units and
-- angles.
M.RING = {
  radius = 688,
  -- How far the camera stands beyond the ring's near edge, how high it
  -- starts and how high it comes down to, and how high the point it looks at
  -- stands.
  camera_gap = 598,
  camera_start = -1536,
  camera_height = -256,
  look_y = -96,
  fov = 80 * trx.math.DEG_1,
  -- How far the ring turns as it opens, and again as it closes.
  turn = -DEG_180,
  -- How many ticks the ring takes to turn from one entry to the next.
  turn_ticks = 12,
}

M.Motion = {
  -- The ring opens out of nothing, the camera coming down onto it.
  OPEN = "open",
  -- The ring folds into nothing, the camera going back up.
  CLOSE = "close",
  -- The ring folds into nothing as the camera tilts up, or down, away from
  -- it, for the ring above, or below, to come in.
  LEAVE_UP = "leave_up",
  LEAVE_DOWN = "leave_down",
  -- The ring opens out of nothing as the camera tilts back from the other
  -- side, after one of the motions above.
  ARRIVE = "arrive",
  -- The entry the ring rests on comes forward, and goes back.
  SELECT = "select",
  DESELECT = "deselect",
  -- The ring holds still, as it does while an entry that opens a ring of its
  -- own is picked.
  HOLD = "hold",
}

-- How the entry the ring rests on turns about its own axis on a tick. The
-- other entries come back to facing the ring's centre.
M.Spin = {
  -- Round and round, as it rests on the ring.
  IDLE = "idle",
  -- Back to facing the camera, as it comes forward and stays there.
  HELD = "held",
  -- Not at all, as the player holds a key that turns the ring.
  STILL = "still",
}

-- How high above the camera's view an entry reaches once the ring has folded
-- onto its centre, over every angle the entry turns through about its own
-- axis. The corners of its bounds give the reach, tilted as the ring holds
-- the entry.
local function elevation(entry, below)
  local bounds = entry.spec.bounds
  if bounds == nil then
    return 0
  end
  local ends = { bounds.min, bounds.max }
  local result = 0
  for corner = 0, 7 do
    local pos = trx.math.rotate({
      x = ends[(corner & 1) + 1].x,
      y = ends[((corner >> 1) & 1) + 1].y,
      z = ends[((corner >> 2) & 1) + 1].z,
    }, { x = wrap(entry.x_rot:get()), y = 0, z = 0 })
    local reach = math.sqrt(pos.x * pos.x + pos.z * pos.z)
    local depth =
      whole(math.max(M.RING.camera_gap - entry.z_trans:get() - reach, 1))
    result = math.max(
      result,
      trx.math.atan(depth, whole(below + entry.y_trans:get() + pos.y))
    )
  end
  return result
end

-- The angle the camera tilts away from a ring that leaves, which takes the
-- highest point any entry reaches past the edge of the screen. The camera
-- already looks below the ring's centre, which takes that much off.
local function leave_pitch(view)
  local screen = trx.ui.screen
  local aspect = screen.width / screen.height
  local half_fov = M.RING.fov * math.pi / DEG_360
  local focal = 4
    / 3
    / math.max(aspect, 16 / 10)
    * (240 / 200)
    / math.tan(half_fov)
    * aspect
  local half_view = math.atan(1 / focal) * DEG_180 / math.pi
  local below = -whole(view.camera_y:get())
  local aim = trx.math.atan(M.RING.camera_gap, below + M.RING.look_y)
  local highest = 0
  for _, entry in ipairs(view.entries) do
    highest = math.max(highest, elevation(entry, below))
  end
  return math.floor(half_view + highest - aim)
end

-- Each motion takes the view, the ticks it runs for, and the entry it moves,
-- which is the one the ring rests on.
M.MOTIONS = {
  [M.Motion.OPEN] = {
    ticks = 16,
    run = function(view, ticks)
      local rest = view:rest_angle()
      view.radius:set(0):to(M.RING.radius, ticks)
      view.camera_y:set(M.RING.camera_start):to(M.RING.camera_height, ticks)
      view.rot:set(rest - M.RING.turn):to(rest, ticks)
    end,
  },
  [M.Motion.CLOSE] = {
    ticks = 16,
    run = function(view, ticks)
      view.radius:to(0, ticks)
      view.camera_y:to(M.RING.camera_start, ticks)
      view.rot:to(view.rot:target() + M.RING.turn, ticks)
    end,
  },
  [M.Motion.LEAVE_UP] = {
    ticks = 12,
    run = function(view, ticks)
      view.left_by = leave_pitch(view)
      view.radius:to(0, ticks)
      view.pitch:to(view.base_pitch + view.left_by, ticks)
      view.rot:to(view.rot:target() + M.RING.turn, ticks)
    end,
  },
  [M.Motion.LEAVE_DOWN] = {
    ticks = 12,
    run = function(view, ticks)
      view.left_by = -leave_pitch(view)
      view.radius:to(0, ticks)
      view.pitch:to(view.base_pitch + view.left_by, ticks)
      view.rot:to(view.rot:target() + M.RING.turn, ticks)
    end,
  },
  [M.Motion.ARRIVE] = {
    ticks = 12,
    run = function(view, ticks)
      local rest = view:rest_angle()
      view.radius:set(0):to(M.RING.radius, ticks)
      view.pitch:set(view.base_pitch - view.left_by):to(view.base_pitch, ticks)
      view.rot:set(rest - M.RING.turn):to(rest, ticks)
    end,
  },
  [M.Motion.SELECT] = {
    ticks = 8,
    run = function(view, ticks, entry)
      local spec = entry.spec
      view.rot:to(view:nearest_rest_angle(), ticks)
      entry.x_rot_pt:to(spec.x_rot_pt_sel or 0, ticks)
      entry.x_rot:to(spec.x_rot_sel or 0, ticks)
      entry.y_trans:to(spec.y_trans_sel or 0, ticks)
      entry.z_trans:to(spec.z_trans_sel or 0, ticks)
    end,
  },
  [M.Motion.DESELECT] = {
    ticks = 8,
    run = function(view, ticks, entry)
      view.rot:to(view:nearest_rest_angle(), ticks)
      entry.x_rot_pt:to(0, ticks)
      entry.x_rot:to(entry.spec.x_rot_nosel or 0, ticks)
      entry.y_trans:to(0, ticks)
      entry.z_trans:to(0, ticks)
    end,
  },
  [M.Motion.HOLD] = {
    ticks = 8,
    run = function(view, ticks)
      view.rot:to(view:nearest_rest_angle(), ticks)
    end,
  },
}

-- A value that steps once a tick. Reading it while the interface is painted
-- gives the value part way between the last two steps, so that it moves
-- smoothly at any frame rate.
---@class common.ui.ring.Stepped
---@field private last number
---@field private now number
local Stepped = {}
Stepped.__index = Stepped

---@return common.ui.ring.Stepped
local function stepped(value)
  return setmetatable({ last = value, now = value }, Stepped)
end

function Stepped:get()
  return self.last + (self.now - self.last) * trx.ui.interpolation_rate()
end

-- Returns the value of the last step.
function Stepped:latest()
  return self.now
end

-- Takes the next step. Called once each tick.
function Stepped:step(value)
  self.last = self.now
  self.now = value
end

-- An entry on the ring and where its own motion has taken it.
---@class common.ui.ring.Entry
---@field spec table What the entry was declared with.
---@field x_rot_pt trx.ui.Tween
---@field x_rot trx.ui.Tween
---@field y_trans trx.ui.Tween
---@field z_trans trx.ui.Tween
---@field y_rot common.ui.ring.Stepped Its turn about its own axis, unwrapped.
---@field hand table How the player has turned it by hand, as a rotation that steps once a tick.
---@field turn_yaw number The hand turn about the camera's vertical axis that the next tick takes.
---@field turn_pitch number The hand turn about the camera's horizontal axis that the next tick takes.
---@field turned boolean Whether the player turns it by hand, which holds it where it was turned.

---@class common.ui.ring.View
---@field radius trx.ui.Tween
---@field camera_y trx.ui.Tween
---@field pitch trx.ui.Tween
---@field rot trx.ui.Tween
---@field entries common.ui.ring.Entry[]
---@field current integer
---@field base_pitch integer
---@field left_by integer
---@field private left integer
---@field private turning? { from: integer, to: integer, left: integer, start: number, by: number }
local View = {}
View.__index = View

-- The angle between one entry and the next.
function View:spacing()
  return DEG_360 // #self.entries
end

-- The angle the ring rests at for an entry, counted from 1.
---@param index? integer
---@return number
function View:rest_angle(index)
  index = index or self.current
  return -DEG_90 - (index - 1) * self:spacing()
end

-- The rest angle of the entry the ring rests on that lies nearest to where
-- the ring is heading, so that settling on it never takes a full turn.
---@return number
function View:nearest_rest_angle()
  local heading = self.rot:target()
  return heading + wrap(self:rest_angle() - heading)
end

-- Starts a motion, which ends a number of ticks later as `View:tick` says.
function View:start(name)
  local motion = M.MOTIONS[name]
  self.left = motion.ticks
  motion.run(self, motion.ticks, self.entries[self.current])
end

-- Whether a motion is under way.
function View:is_moving()
  return self.left > 0
end

-- Turns the ring to the next entry, by `1`, or to the one before, by `-1`.
-- The ring rests on it once the turn ends.
function View:turn_to(by)
  local count = #self.entries
  local to = (self.current - 1 + by) % count + 1
  local start = self.rot:target()
  self.turning = {
    from = self.current,
    to = to,
    left = M.RING.turn_ticks,
    start = start,
    by = -by * self:spacing(),
  }
  self.rot:to(start - by * self:spacing(), M.RING.turn_ticks)
end

-- Whether the ring is turning from one entry to the next.
function View:is_turning()
  return self.turning ~= nil
end

-- Moves an entry's turn about its own axis by up to a step towards its
-- target, two steps a tick.
local function spin(entry, step)
  local now = entry.y_rot:latest()
  local at = wrap(now)
  local to = at
  for _ = 1, 2 do
    to = step(to)
  end
  entry.y_rot:step(now + wrap(to - at))
end

local function towards_zero(by)
  return function(angle)
    if angle > 0 then
      return math.max(angle - by, 0)
    elseif angle < 0 then
      return math.min(angle + by, 0)
    end
    return angle
  end
end

local function towards(target)
  return function(angle)
    local delta = wrap(target - angle)
    if delta == 0 then
      return angle
    end
    return (delta > 0 and angle + 1024 or angle - 1024) & ~1023
  end
end

local function round_and_round(angle)
  return angle + 256
end

local function still(angle)
  return angle
end

-- A hand turn is kept as one rotation, a unit quaternion, so that each turn
-- the player makes is about the camera's axes however the entry was turned
-- before. The rotation is given to the pose as the engine's yaw, pitch and
-- roll, which it applies in that order.
local IDENTITY = { w = 1, x = 0, y = 0, z = 0 }

local function to_radians(angle)
  return angle * math.pi / 32768
end

local function to_angle(radians)
  return math.floor(radians * 32768 / math.pi + 0.5)
end

local function multiply(a, b)
  return {
    w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
    y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
    z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
  }
end

-- A turn by an engine angle about one axis.
local function about(axis, angle)
  local half = to_radians(angle) / 2
  local q = { w = math.cos(half), x = 0, y = 0, z = 0 }
  q[axis] = math.sin(half)
  return q
end

-- Goes part of the way from one rotation to another, the short way round.
local function slerp(a, b, t)
  local dot = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z
  if dot < 0 then
    b = { w = -b.w, x = -b.x, y = -b.y, z = -b.z }
    dot = -dot
  end
  local ka, kb = 1 - t, t
  if dot < 0.9995 then
    local theta = math.acos(dot)
    local sin = math.sin(theta)
    ka = math.sin((1 - t) * theta) / sin
    kb = math.sin(t * theta) / sin
  end
  local q = {
    w = ka * a.w + kb * b.w,
    x = ka * a.x + kb * b.x,
    y = ka * a.y + kb * b.y,
    z = ka * a.z + kb * b.z,
  }
  local length = math.sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z)
  return {
    w = q.w / length,
    x = q.x / length,
    y = q.y / length,
    z = q.z / length,
  }
end

-- Adds a turn about the camera's axes to a hand turn. The pitch runs the
-- other way from a pose's turn about X, as it did when the engine turned the
-- entry.
local function turn_hand(hand, yaw, pitch)
  if yaw == 0 and pitch == 0 then
    return hand
  end
  return multiply(multiply(about("y", yaw), about("x", -pitch)), hand)
end

-- The engine's yaw, pitch and roll that turn as a rotation does.
local function to_rot(q)
  local m02 = 2 * (q.x * q.z + q.w * q.y)
  local m10 = 2 * (q.x * q.y + q.w * q.z)
  local m11 = 1 - 2 * (q.x * q.x + q.z * q.z)
  local m12 = 2 * (q.y * q.z - q.w * q.x)
  local m22 = 1 - 2 * (q.x * q.x + q.y * q.y)
  local pitch = math.asin(math.max(-1, math.min(1, -m12)))
  return {
    x = wrap(to_angle(pitch)),
    y = wrap(to_angle(math.atan(m02, m22))),
    z = wrap(to_angle(math.atan(m10, m11))),
  }
end

-- How far a hand turn comes back each tick once the player lets go.
local UNTURN_RATE = 1 - (1 - 0.15) ^ 2

-- Runs a tick: turns each entry about its own axis, the one the ring rests on
-- as `how` says, counts down the motion under way and the turn from one entry
-- to the next. Returns whether the motion ended on this tick.
function View:tick(how)
  how = how or M.Spin.IDLE
  for index, entry in ipairs(self.entries) do
    local hand = turn_hand(entry.hand.now, entry.turn_yaw, entry.turn_pitch)
    entry.turn_yaw = 0
    entry.turn_pitch = 0
    if index ~= self.current then
      spin(entry, towards_zero(256))
    elseif self.turning ~= nil then
      spin(entry, towards_zero(512))
    elseif how == M.Spin.HELD then
      if entry.turned then
        spin(entry, still)
      else
        spin(entry, towards(entry.spec.y_rot_sel or 0))
        hand = slerp(hand, IDENTITY, UNTURN_RATE)
      end
    elseif how == M.Spin.IDLE then
      spin(entry, round_and_round)
    else
      spin(entry, still)
    end
    entry.hand.last = entry.hand.now
    entry.hand.now = hand
  end

  local turning = self.turning
  if turning ~= nil then
    turning.left = turning.left - 1
    if turning.left == 0 then
      self.current = turning.to
      self.turning = nil
    end
  end

  if self.left == 0 then
    return false
  end
  self.left = self.left - 1
  return self.left == 0
end

-- How far an entry stands from being the one the ring rests on, from `0` for
-- that one to `1` for any other, which shades it as the game shades it. The
-- shade moves from one entry to the next as the ring turns.
function View:shade(index)
  local turning = self.turning
  if turning == nil then
    return index == self.current and 0 or 1
  end
  local done = (self.rot:get() - turning.start) / turning.by
  done = math.max(0, math.min(1, done))
  if index == turning.from then
    return done
  elseif index == turning.to then
    return 1 - done
  end
  return 1
end

-- Turns the entry the ring rests on by hand, as the player examines it, by
-- angles about the camera's vertical and horizontal axes. The turn takes
-- effect on the next tick, so that it moves smoothly between ticks. It stays
-- where it was turned until `View:let_go`.
function View:turn_by_hand(yaw, roll)
  local entry = self.entries[self.current]
  entry.turned = true
  entry.turn_yaw = entry.turn_yaw + yaw
  entry.turn_pitch = entry.turn_pitch + roll
end

-- Lets go of the entry the ring rests on, which then turns back from where
-- the player turned it while it is held.
function View:let_go()
  self.entries[self.current].turned = false
end

-- The camera that sees the ring, for `trx.ui.primitive.world_model`. Its
-- pitch tilts the point it looks at about the camera.
function View:camera()
  local radius = whole(self.radius:get())
  local pos = {
    x = 0,
    y = whole(self.camera_y:get()),
    z = radius + M.RING.camera_gap,
  }
  local dy = M.RING.look_y - pos.y
  local dz = radius - pos.z
  local length = math.sqrt(dy * dy + dz * dz)
  local pitch = trx.math.atan(-dz, -dy) + self.pitch:get()
  local across = math.cos(pitch * math.pi / DEG_180) * length
  return {
    pos = pos,
    target = {
      x = 0,
      y = whole(pos.y - math.sin(pitch * math.pi / DEG_180) * length),
      z = whole(pos.z - across),
    },
    fov = M.RING.fov,
  }
end

-- The steps that place an entry on the ring, for
-- `trx.ui.primitive.world_model`, followed by any steps of the caller's own.
---@param index integer The entry, counted from 1.
---@param extra? trx.ui.PoseStep[]
---@return trx.ui.PoseStep[]
function View:pose(index, extra)
  local entry = self.entries[index]
  local spec = entry.spec
  local x_rot_pt = wrap(entry.x_rot_pt:get())
  local base = spec.base_rot or {}
  local steps = {
    {
      turn = {
        x = 0,
        y = wrap(self.rot:get() + (index - 1) * self:spacing()),
        z = 0,
      },
    },
    { move = { x = whole(self.radius:get()), y = 0, z = 0 } },
    { turn = { x = 0, y = DEG_90, z = 0 } },
    { turn = { x = x_rot_pt, y = 0, z = 0 } },
    {
      move = {
        x = 0,
        y = whole(entry.y_trans:get() + (spec.y_offset or 0)),
        z = whole(entry.z_trans:get()),
      },
    },
    { turn = { x = wrap(-x_rot_pt), y = 0, z = 0 } },
    {
      turn = to_rot(
        slerp(entry.hand.last, entry.hand.now, trx.ui.interpolation_rate())
      ),
    },
    { turn = { x = x_rot_pt, y = 0, z = 0 } },
    {
      turn = {
        x = wrap(entry.x_rot:get() + (base.x or 0)),
        y = wrap(entry.y_rot:get() + (base.y or 0)),
        z = wrap(base.z or 0),
      },
    },
  }
  if spec.scale ~= nil and spec.scale ~= 1 then
    steps[#steps + 1] = { scale = spec.scale }
  end
  for _, step in ipairs(extra or {}) do
    steps[#steps + 1] = step
  end
  return steps
end

---@return common.ui.ring.Entry
local function new_entry(spec)
  return {
    spec = spec,
    x_rot_pt = trx.ui.tween(0),
    x_rot = trx.ui.tween(spec.x_rot_nosel or 0),
    y_trans = trx.ui.tween(0),
    z_trans = trx.ui.tween(0),
    y_rot = stepped(0),
    hand = { last = IDENTITY, now = IDENTITY },
    turn_yaw = 0,
    turn_pitch = 0,
    turned = false,
  }
end

---@class common.ui.ring.View.settings
---@field entries table[] What each entry on the ring holds, in its order: how far it comes forward and how it tilts when picked (`y_trans_sel`, `z_trans_sel`, `x_rot_sel`, `x_rot_pt_sel`, `y_rot_sel`), how it tilts at rest (`x_rot_nosel`), how it is drawn (`scale`, `y_offset`, `base_rot`), and the box its model fills (`bounds`, as `{ min = vec, max = vec }`).
---@field current? integer The entry the ring rests on, counted from 1. The first by default.
---@field pitch? integer How far the camera tilts from looking at the ring's centre. Not at all by default.

-- Creates a ring that has not opened yet, with its camera high above it.
---@param settings common.ui.ring.View.settings
---@return common.ui.ring.View
function M.new(settings)
  local entries = {}
  for i, spec in ipairs(settings.entries) do
    entries[i] = new_entry(spec)
  end
  local pitch = settings.pitch or 0
  return setmetatable({
    radius = trx.ui.tween(0),
    camera_y = trx.ui.tween(M.RING.camera_start),
    pitch = trx.ui.tween(pitch),
    rot = trx.ui.tween(0),
    entries = entries,
    current = settings.current or 1,
    base_pitch = pitch,
    left_by = 0,
    left = 0,
  }, View)
end

return M
