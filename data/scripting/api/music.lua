local raw = trxc.music
local api = trx.api

api.module("music", {
  order = 6,
  description = "Module for playing and controlling the soundtrack.",
})

local PlayMode = api.enum("music.PlayMode", {
  backing = "MUSIC_PLAY_MODE",
  description = "How a track is played. Pass one as `opts.mode` to `trx.music.play`.",
  values = {
    ONCE = "Plays the track once. When it finishes, any active looped track resumes from its start.",
    LOOP = "Plays the track continuously. It becomes the ambient track.",
    NO_REPEAT = "Plays the track once, but does not retrigger it if it is already playing.",
    DELAY = "Marks the track for later playback rather than starting it now.",
    OVERLAY = "Plays the track on top of the current one.",
  },
})

api.define("music.get_track", {
  description = "The track currently playing.",
  returns = {
    type = "integer",
    nullable = true,
    enum = "catalog.music",
    description = "`nil` if nothing is playing.",
  },
  impl = raw.get_track,
})

api.define("music.play", {
  description = "Plays a track. Raises if the track or the mode is invalid.",
  params = {
    {
      name = "id",
      type = "integer",
      enum = "catalog.music",
      description = "Track to play.",
    },
    {
      name = "opts",
      type = "table",
      optional = true,
      description = "`mode`: a `trx.music.PlayMode`. Defaults to `ONCE`.",
    },
  },
  examples = {
    [[trx.music.play(1)
trx.music.play(2, { mode = trx.music.PlayMode.LOOP })]],
  },
  impl = function(id, opts)
    opts = opts or {}
    raw.play(id, opts.mode or PlayMode.ONCE)
  end,
})

api.define("music.pause", {
  description = "Pauses the music.",
  impl = raw.pause,
})

api.define("music.unpause", {
  description = "Resumes paused music.",
  impl = raw.unpause,
})

api.define("music.stop", {
  description = "Stops all music.",
  impl = raw.stop,
})
