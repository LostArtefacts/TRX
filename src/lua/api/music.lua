local raw = trxc.music
local api = trx.api

api.module("music", {
  order = 22,
  description = "Module for playing and controlling the soundtrack.",
})

api.number("music.TrackNum", {
  base = 0,
  description = "Track number, in the numbering the loaded level carries. Not a "
    .. "`trx.catalog.music` name, which is the soundtrack's own.",
})

local PlayMode = api.enum("music.PlayMode", {
  backing = "MUSIC_PLAY_MODE",
  description = "How a track is played. Pass one as `trx.music.play.opts.mode`.",
  values = {
    ONCE = "Plays the track once. When it finishes, any active looped track resumes from its start.",
    LOOP = "Plays the track continuously. It becomes the ambient track.",
    NO_REPEAT = "Plays the track once, but does not retrigger it if it is already playing.",
    DELAY = "Marks the track for later playback rather than starting it now.",
    OVERLAY = "Plays the track on top of the current one.",
  },
})

local PLAY_OPTS = {
  name = "opts",
  type = "table",
  optional = true,
  description = "How to play it.",
  fields = {
    {
      name = "mode",
      type = "music.PlayMode",
      optional = true,
      description = "Plays once by default.",
    },
  },
}

api.number("music.StreamNum", {
  base = 1,
  description = "Which of the soundtrack's slots: 1 is the main stream, 2 onwards the overlays.",
})

api.type("music.Stream", {
  backing = "MUSIC_STREAM_VIEW",
  description = "One of the soundtrack's playing streams: the main stream, or an overlay. "
    .. "Reach them through `trx.music.streams`. A handle to a slot that is not playing goes "
    .. "stale, so reading a field or calling a method on it raises; check `trx.music.Stream:is_valid` first.",

  fields = {
    track_num = {
      from = "track_id",
      type = "music.TrackNum",
      writable = false,
      description = "The track this stream is playing.",
    },
    mode = {
      from = "mode",
      type = "music.PlayMode",
      writable = false,
      description = "How the track is playing.",
    },
    timestamp = {
      from = "timestamp",
      type = "game.Seconds",
      writable = false,
      description = "How far into the track the stream is.",
    },
  },

  methods = {
    is_valid = {
      returns = {
        type = "boolean",
        description = "False once the slot has gone quiet.",
      },
      description = "Whether the slot is still playing. A stream that has finished, or been "
        .. "stopped, leaves its handle stale.",
    },
    pause = {
      description = "Pauses this stream.",
    },
    unpause = {
      description = "Resumes this stream.",
    },
    seek = {
      params = {
        {
          name = "timestamp",
          type = "game.Seconds",
          description = "Where to seek to.",
        },
      },
      returns = { type = "boolean", description = "Whether the seek took." },
      description = "Seeks this stream to a timestamp.",
    },
    stop = {
      description = "Stops this stream. Stopping the main stream lets a deferred ambient loop "
        .. "resume; an overlay just ends.",
    },
  },
})

api.type("music.Track", {
  backing = "MUSIC_TRACK_VIEW",
  description = "A track the current level carries. Reach them through `trx.music.tracks`, or "
    .. "as `trx.music.current_track`. A handle to a track the loaded level does not carry goes "
    .. "stale, so `trx.music.Track:is_valid` answers whether it is still there.",

  fields = {
    num = {
      from = "id",
      type = "music.TrackNum",
      writable = false,
    },
  },

  methods = {
    is_valid = {
      returns = {
        type = "boolean",
        description = "False once a level change has replaced the tracks.",
      },
      description = "Whether the loaded level still carries this track.",
    },
    play = {
      params = { PLAY_OPTS },
      returns = {
        type = "music.Stream",
        nullable = true,
        description = "The stream it started, or `nil` if none did.",
      },
      description = "Plays this track.",
    },
    path = {
      returns = {
        type = "string",
        nullable = true,
        description = "`nil` when there is no file, e.g. a CD-audio soundtrack.",
      },
      description = "Resolves the track's file path.",
    },
  },
})

-- One lazy view apiece: indexing and iterating reach into C a handle at a time,
-- so neither builds a list up front.
api.container("music.streams", {
  description = "The soundtrack's streams: `[1]` is the main stream, `[2]` onwards the overlay "
    .. "slots. A slot that is not playing still answers, with a stale handle.",
  key = { type = "music.StreamNum" },
  value = { type = "music.Stream", nullable = true },
  get = function(n)
    return raw.stream_get(n - 1)
  end,
  count = raw.stream_count,
})

local tracks = api.container("music.tracks", {
  description = "The tracks the current level carries. A level does not carry every number, so "
    .. "indexing one it lacks is `nil` and iterating passes it by.",
  key = { type = "music.TrackNum" },
  value = { type = "music.Track", nullable = true },
  get = raw.track_get,
  count = raw.track_available_count,
  limit = raw.track_limit,
})

api.property("music.current_track", {
  type = "music.Track",
  description = "The track playing now, or `nil` when nothing plays.",
  get = function()
    local id = raw.get_track()
    return id ~= nil and raw.track_get(id) or nil
  end,
})

api.property("music.looped_track", {
  type = "music.Track",
  description = "The ambient track that resumes once the current one-shot finishes, or `nil` "
    .. "when none is set.",
  get = function()
    local id = raw.get_looped_track()
    return id ~= nil and raw.track_get(id) or nil
  end,
})

api.define("music.play", {
  description = "Plays a track by catalog id, mapping it to the level's own track. A game that "
    .. "does not carry the track plays nothing.",
  params = {
    {
      name = "id",
      type = "catalog.music",
      description = "Track to play. To reach a track by the level's own slot, play it through a "
        .. "handle: `trx.music.tracks[slot]:play()`.",
    },
    PLAY_OPTS,
  },
  returns = {
    type = "music.Stream",
    nullable = true,
    description = "The stream it started, or `nil` if none did.",
  },
  examples = {
    [[trx.music.play(trx.catalog.music.SECRET)
trx.music.play(trx.catalog.music.SECRET, { mode = trx.music.PlayMode.LOOP })]],
  },
  impl = function(id, opts)
    opts = opts or {}
    local slot = trx.catalog.to_slot(trx.catalog.Context.MUSIC, id)
    local track = slot ~= nil and tracks[slot] or nil
    return track ~= nil and track:play({ mode = opts.mode or PlayMode.ONCE })
      or nil
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
