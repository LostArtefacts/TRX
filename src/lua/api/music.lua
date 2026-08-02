local raw = trxc.music
local api = trx.api

api.module("music", {
  order = 18,
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

api.type("music.Stream", {
  backing = "MUSIC_STREAM_VIEW",
  description = "One of the soundtrack's playing streams: the main stream, or an overlay. "
    .. "Reach them through `trx.music.streams`. A handle to a slot that is not playing goes "
    .. "stale, so reading a field or calling a method on it raises; check `trx.music.Stream:is_valid` first.",

  fields = {
    track_num = {
      from = "track_id",
      type = "integer",
      writable = false,
      description = "The track this stream is playing, in the level's own numbering.",
    },
    mode = {
      from = "mode",
      type = "music.PlayMode",
      writable = false,
      description = "How the track is playing.",
    },
    timestamp = {
      from = "timestamp",
      type = "number",
      writable = false,
      description = "How far into the track the stream is, in seconds.",
    },
  },

  methods = {
    is_valid = {
      returns = { type = "boolean" },
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
          type = "number",
          description = "Where to seek to, in seconds.",
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
      type = "integer",
      writable = false,
      description = "The number the level gives this track.",
    },
  },

  methods = {
    is_valid = {
      returns = { type = "boolean" },
      description = "Whether the loaded level still carries this track.",
    },
    play = {
      params = {
        {
          name = "opts",
          type = "table",
          optional = true,
          description = "`mode`: a `trx.music.PlayMode`. Defaults to `ONCE`.",
        },
      },
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
local streams = setmetatable({}, {
  __index = function(_, n)
    if type(n) ~= "number" then
      return nil
    end
    return raw.stream_get(n - 1)
  end,
  __len = function()
    return raw.stream_count()
  end,
  __pairs = function()
    local count = raw.stream_count()
    local i = 0
    return function()
      i = i + 1
      if i <= count then
        return i, raw.stream_get(i - 1)
      end
    end
  end,
})

local tracks = setmetatable({}, {
  __index = function(_, id)
    if type(id) ~= "number" then
      return nil
    end
    return raw.track_get(id)
  end,
  __len = function()
    return raw.track_available_count()
  end,
  __pairs = function()
    local limit = raw.track_limit()
    local id = -1
    return function()
      id = id + 1
      while id < limit do
        local track = raw.track_get(id)
        if track ~= nil then
          return id, track
        end
        id = id + 1
      end
    end
  end,
})

api.property("music.streams", {
  type = "table",
  description = "The soundtrack's streams as `trx.music.Stream` handles: `[1]` is the main "
    .. "stream, `[2]` onwards the overlay slots. A slot that is not playing still answers, with a "
    .. "stale handle. Indexing and iterating reach one handle at a time.",
  get = function()
    return streams
  end,
})

api.property("music.tracks", {
  type = "table",
  description = "The tracks the current level carries, as `trx.music.Track` handles keyed by id: "
    .. "`trx.music.tracks[5]` is track 5, or `nil` if the level has no such track. `#` counts them, "
    .. "iterating walks them, and both reach one handle at a time.",
  get = function()
    return tracks
  end,
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
    {
      name = "opts",
      type = "table",
      optional = true,
      description = "`mode`: a `trx.music.PlayMode`. Defaults to `ONCE`.",
    },
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
