local raw = trxc.waypoints
local h = require("trx.internal.helpers")

---@class (partial) trx
---@field waypoints trx.waypoints

---Module for how far along a level's own progression Lara has got.
---
---TR4 marks the points of a level with flip effects, and its guides read
---them: Von Croy waits at a waypoint until Lara has reached it, says the
---line that belongs to it, and only then moves on. A level uses the same
---marks to tell a first visit from a return.
---
---Nothing about a waypoint is positional. It counts progress, and it lasts
---as long as the playthrough rather than the level, so it is saved with the
---game.
---@trx.module 39
---@class (exact) trx.waypoints
---@trx.readonly highest
---@field current trx.waypoints.Num? Where Lara has reached, or `nil` before she has reached anywhere. Setting it carries the furthest reached along with it where that is further on.
---@field pad trx.waypoints.Num? The pad Lara crossed this frame, or `nil` on any frame she crossed none.
---
---  It says where she is standing now rather than how far she has got, and it
---  is meant to last the one frame: whoever sets it clears it again at the
---  start of the next, which is `nil` here.
---
---  Setting it carries `trx.waypoints.current` along with it, but leaves
---  `trx.waypoints.highest` alone.
---@field highest trx.waypoints.Num? The furthest Lara has ever reached, or `nil` before she has reached anywhere. It never falls, so a level that lets her walk back can still tell how far she got.
local M = h.module("waypoints")

---A waypoint's number, as the flip effect that marks it names it.
---@trx.base 0
---@alias trx.waypoints.Num integer

h.properties(M, "waypoints", {
  -- trx.events.on_cutscene_trigger(function(cutscene_num)
  --   if cutscene_num == 17 and trx.waypoints.current ~= 4 then
  --     return true
  --   end
  --   return false
  -- end)
  current = {
    get = raw.get_current,
    set = raw.set_current,
  },
  -- trx.events.before_control(function()
  --   trx.waypoints.pad = nil
  -- end)
  pad = {
    get = raw.get_pad,
    set = raw.set_pad,
  },
  highest = {
    get = raw.get_highest,
  },
})
