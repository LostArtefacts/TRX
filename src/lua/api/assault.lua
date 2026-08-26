local raw = trxc.assault
local api = trx.api

api.module("assault", {
  order = 18,
  title = "Assault course",
  description = "Module for controlling the Assault Course and Quad Bike timers in gym levels.",
})

api.number("assault.RecordNum", {
  base = 1,
  description = "Where a time sits in the table of best times, fastest first.",
})

api.number("assault.AttemptNum", {
  base = 1,
  description = "Which attempt at a track it was, counted in the order they were made.",
})

api.type("assault.Record", {
  record = true,
  description = "One of a track's best times.",
  fields = {
    time = { type = "game.Seconds", description = "The time it took." },
    attempt_num = { type = "assault.AttemptNum" },
  },
})

local Track = api.enum("assault.Track", {
  backing = "GYM_TRACK_TYPE",
  description = "A timed gym track.",
  values = {
    COURSE = "Lara's assault course.",
    QUAD = "The quad bike circuit.",
  },
})

-- Every track-taking function defaults to the assault course, which is what a
-- script means when it does not say.
local function track_param()
  return {
    name = "track",
    type = "assault.Track",
    optional = true,
    default = Track.COURSE,
  }
end

api.define("assault.start", {
  description = "Starts the timer and clears its state. Raises outside a gym level.",
  params = { track_param() },
  impl = raw.start,
})

api.define("assault.stop", {
  description = "Stops the timer, leaving it on screen. Raises outside a gym level.",
  params = { track_param() },
  impl = raw.stop,
})

api.define("assault.finish", {
  description = "Stops the timer as completing the track does, rather than as an abort. Raises "
    .. "outside a gym level.",
  params = { track_param() },
  impl = raw.finish,
})

api.define("assault.reset", {
  description = "Stops the timer and clears its state. Raises outside a gym level.",
  params = { track_param() },
  impl = raw.reset,
})

api.define("assault.is_running", {
  description = "Whether the timer is counting. False outside a gym level.",
  params = { track_param() },
  returns = {
    type = "boolean",
    description = "True from the start of a run until it is finished or stopped.",
  },
  impl = raw.is_running,
})

api.define("assault.is_visible", {
  description = "Whether the timer is shown on screen. It stays visible after `trx.assault.stop`.",
  params = { track_param() },
  returns = {
    type = "boolean",
    description = "True while the timer is drawn, counting or not.",
  },
  impl = raw.is_visible,
})

-- The timings the digits are drawn from. Every one reads 0 outside a gym level
-- rather than raising, so that a widget can read one each frame.
local function timing(name, description, returns)
  api.define("assault." .. name, {
    description = description,
    params = { track_param() },
    returns = { type = "game.Frames", description = returns },
    impl = raw[name],
  })
end

api.define("assault.get_time", {
  description = [[
How long the current run has taken.

This is the level clock, which is what a gym level times its tracks with, so it
takes no track.]],
  returns = {
    type = "game.Frames",
    description = "The time on the clock, counting up while the timer runs.",
  },
  impl = raw.get_time,
})

timing(
  "get_best_time",
  "The fastest time the track has on record.",
  "The best time, or 0 where the track has none."
)

timing(
  "get_penalty",
  "The penalty the run has taken for missed pads.",
  "The penalty, added to the time when the run is filed."
)

timing(
  "get_target_penalty",
  "The penalty the run has taken for missed targets.",
  "The penalty, added to the time when the run is filed."
)

timing(
  "get_penalty_timer",
  "How much longer a penalty stays on screen.",
  "The time left, and 0 where no penalty is shown."
)

timing(
  "get_lap_time",
  "How long the last lap took.",
  "The lap time, and 0 before a lap is finished."
)

timing(
  "get_lap_timer",
  "How much longer the lap times stay on screen.",
  "The time left, and 0 where no lap time is shown."
)

api.property("assault.active_track", {
  type = "assault.Track",
  description = "The track Lara is currently running, or `nil` if none.",
  get = raw.get_active_track,
})

api.namespace("assault.stats", {
  description = "A track's record table, as shown on the stats screen. Each track keeps its own. "
    .. "The records are stored in the player's profile, so writing to them outlives the level, and "
    .. "they can be read outside a gym level.",
})

api.define("assault.stats.add_record", {
  description = "Files a new record, inserting it in time order and bumping the attempt count.",
  params = {
    {
      name = "time",
      type = "game.Seconds",
      description = "Must be greater than zero.",
    },
    track_param(),
  },
  returns = {
    type = "boolean",
    description = "`false` if the table is full and the time is slower than every record in it.",
  },
  examples = { [[trx.assault.stats.add_record(30.0)]] },
  impl = raw.stats.record,
})

api.define("assault.stats.remove_record", {
  description = "Removes a record, closing the gap behind it.",
  params = {
    {
      name = "record_num",
      type = "assault.RecordNum",
    },
    track_param(),
  },
  returns = {
    type = "boolean",
    description = "`false` if there is no record at that position.",
  },
  impl = raw.stats.remove,
})

api.define("assault.stats.list_records", {
  description = "The records, fastest first.",
  params = { track_param() },
  returns = {
    type = "assault.Record",
    list = true,
  },
  examples = {
    [[for _, record in ipairs(trx.assault.stats.list_records(trx.assault.Track.QUAD)) do
  trx.log.info(("attempt %d: %.2fs"):format(record.attempt_num, record.time))
end]],
  },
  impl = raw.stats.list,
})
