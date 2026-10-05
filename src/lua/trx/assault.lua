local raw = trxc.assault
local h = require("trx.internal.helpers")

---@class trx
---@field assault trx.assault

---Module for controlling the Assault Course and Quad Bike timers in gym
---levels.
---@trx.module 19 Assault course
---@class (exact) trx.assault
---@trx.readonly active_track
---@field active_track trx.assault.Track The track Lara is currently running, or `nil` if none.
local M = h.module("assault")

---Where a time sits in the table of best times, fastest first.
---@trx.base 1
---@alias trx.assault.RecordNum integer

---Which attempt at a track it was, counted in the order they were made.
---@trx.base 1
---@alias trx.assault.AttemptNum integer

---One of a track's best times.
---@trx.record
---@class trx.assault.Record
---@field time trx.game.Seconds The time it took.
---@field attempt_num trx.assault.AttemptNum

---A timed gym track.
---@enum trx.assault.Track
local Track = {
  ---Lara's assault course.
  COURSE = h.IntegerConstant,
  ---The quad bike circuit.
  QUAD = h.IntegerConstant,
}
M.Track = h.enum("assault.Track", "GYM_TRACK_TYPE", Track)

-- Every track-taking function defaults to the assault course, which is what a
-- script means when it does not say.

---Starts the timer and clears its state. Raises outside a gym level.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@type fun(track?: trx.assault.Track)
M.start = raw.start

---Stops the timer, leaving it on screen. Raises outside a gym level.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@type fun(track?: trx.assault.Track)
M.stop = raw.stop

---Stops the timer as completing the track does, rather than as an abort.
---Raises outside a gym level.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@type fun(track?: trx.assault.Track)
M.finish = raw.finish

---Stops the timer and clears its state. Raises outside a gym level.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@type fun(track?: trx.assault.Track)
M.reset = raw.reset

---Whether the timer is counting. False outside a gym level.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@return boolean # True from the start of a run until it is finished or stopped.
---@type fun(track?: trx.assault.Track): boolean
M.is_running = raw.is_running

---Whether the timer is shown on screen. It stays visible after
---`trx.assault.stop`.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@return boolean # True while the timer is drawn, counting or not.
---@type fun(track?: trx.assault.Track): boolean
M.is_visible = raw.is_visible

-- The timings the digits are drawn from. Every one reads 0 outside a gym level
-- rather than raising, so that a widget can read one each frame.

---How long the current run has taken.
---
---This is the level clock, which is what a gym level times its tracks with,
---so it takes no track.
---@return trx.game.Frames # The time on the clock, counting up while the timer runs.
---@type fun(): trx.game.Frames
M.get_time = raw.get_time

---The fastest time the track has on record.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@return trx.game.Frames # The best time, or 0 where the track has none.
---@type fun(track?: trx.assault.Track): trx.game.Frames
M.get_best_time = raw.get_best_time

---The penalty the run has taken for missed pads.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@return trx.game.Frames # The penalty, added to the time when the run is filed.
---@type fun(track?: trx.assault.Track): trx.game.Frames
M.get_penalty = raw.get_penalty

---The penalty the run has taken for missed targets.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@return trx.game.Frames # The penalty, added to the time when the run is filed.
---@type fun(track?: trx.assault.Track): trx.game.Frames
M.get_target_penalty = raw.get_target_penalty

---How much longer a penalty stays on screen.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@return trx.game.Frames # The time left, and 0 where no penalty is shown.
---@type fun(track?: trx.assault.Track): trx.game.Frames
M.get_penalty_timer = raw.get_penalty_timer

---How long the last lap took.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@return trx.game.Frames # The lap time, and 0 before a lap is finished.
---@type fun(track?: trx.assault.Track): trx.game.Frames
M.get_lap_time = raw.get_lap_time

---How much longer the lap times stay on screen.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@return trx.game.Frames # The time left, and 0 where no lap time is shown.
---@type fun(track?: trx.assault.Track): trx.game.Frames
M.get_lap_timer = raw.get_lap_timer

h.properties(M, "assault", {
  active_track = { get = raw.get_active_track },
})

---A track's record table, as shown on the stats screen. Each track keeps its
---own. The records are stored in the player's profile, so writing to them
---outlives the level, and they can be read outside a gym level.
---@class (exact) trx.assault.stats
M.stats = h.namespace("assault.stats")

---Files a new record, inserting it in time order and bumping the attempt
---count.
---
---```lua
---trx.assault.stats.add_record(30.0)
---```
---@param time trx.game.Seconds Must be greater than zero.
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@return boolean # `false` if the table is full and the time is slower than every record in it.
---@type fun(time: trx.game.Seconds, track?: trx.assault.Track): boolean
M.stats.add_record = raw.stats.record

---Removes a record, closing the gap behind it.
---@param record_num trx.assault.RecordNum
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@return boolean # `false` if there is no record at that position.
---@type fun(record_num: trx.assault.RecordNum, track?: trx.assault.Track): boolean
M.stats.remove_record = raw.stats.remove

---The records, fastest first.
---
---```lua
---for _, record in ipairs(trx.assault.stats.list_records(trx.assault.Track.QUAD)) do
---  trx.log.info(("attempt %d: %.2fs"):format(record.attempt_num, record.time))
---end
---```
---@param track? trx.assault.Track
---@trx.default track trx.assault.Track.COURSE
---@return trx.assault.Record[]
---@type fun(track?: trx.assault.Track): trx.assault.Record[]
M.stats.list_records = raw.stats.list
