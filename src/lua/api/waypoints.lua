local raw = trxc.waypoints
local api = trx.api

api.module("waypoints", {
  order = 38,
  description = [[
    Module for how far along a level's own progression Lara has got.

    TR4 marks the points of a level with flip effects, and its guides read
    them: Von Croy waits at a waypoint until Lara has reached it, says the
    line that belongs to it, and only then moves on. A level uses the same
    marks to tell a first visit from a return.

    Nothing about a waypoint is positional. It counts progress, and it lasts
    as long as the playthrough rather than the level, so it is saved with the
    game.
  ]],
})

api.number("waypoints.Num", {
  base = 0,
  description = "A waypoint's number, as the flip effect that marks it names it.",
})

api.property("waypoints.current", {
  type = "waypoints.Num",
  description = [[
    Where Lara has reached, or `nil` before she has reached anywhere. Setting
    it carries the furthest reached along with it where that is further on.
  ]],
  examples = {
    [[trx.events.on_cutscene_trigger(function(cutscene_num)
  if cutscene_num == 17 and trx.waypoints.current ~= 4 then
    return true
  end
  return false
end)]],
  },
  get = raw.get_current,
  set = raw.set_current,
})

api.property("waypoints.pad", {
  type = "waypoints.Num",
  description = [[
    The pad Lara crossed this frame, or `nil` on any frame she crossed none.

    It says where she is standing now rather than how far she has got, and
    it is meant to last the one frame: whoever sets it clears it again at the
    start of the next, which is `nil` here.

    Setting it carries `trx.waypoints.current` along with it, but leaves
    `trx.waypoints.highest` alone.
  ]],
  examples = {
    [[trx.events.before_control(function()
  trx.waypoints.pad = nil
end)]],
  },
  get = raw.get_pad,
  set = raw.set_pad,
})

api.property("waypoints.highest", {
  type = "waypoints.Num",
  description = [[
    The furthest Lara has ever reached, or `nil` before she has reached
    anywhere. It never falls, so a level that lets her walk back can still
    tell how far she got.
  ]],
  get = raw.get_highest,
})
