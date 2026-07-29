local raw = trxc.assault
local api = trx.api

api.module("assault", {
  order = 14,
  title = "Assault course",
  description = "Module for controlling the Assault Course and Quad Bike timers in gym levels.",
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
    type = "integer",
    optional = true,
    default = Track.COURSE,
    enum = "assault.Track",
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
  returns = { type = "boolean" },
  impl = raw.is_running,
})

api.define("assault.is_visible", {
  description = "Whether the timer is shown on screen. It stays visible after `stop`.",
  params = { track_param() },
  returns = { type = "boolean" },
  impl = raw.is_visible,
})

api.property("assault.active_track", {
  type = "integer",
  enum = "assault.Track",
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
      type = "number",
      description = "Time in seconds. Must be greater than zero.",
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
      name = "record_id",
      type = "integer",
      description = "1-based position in the table.",
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
    type = "table",
    description = "List, counted from one, of `{ time = seconds, attempt_num = which attempt it "
      .. "was }`.",
  },
  examples = {
    [[for _, record in ipairs(trx.assault.stats.list_records(trx.assault.Track.QUAD)) do
  trx.log.info(("attempt %d: %.2fs"):format(record.attempt_num, record.time))
end]],
  },
  impl = raw.stats.list,
})
