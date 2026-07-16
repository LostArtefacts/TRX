local raw = trxc.sound
local api = trx.api

api.module("sound", {
  order = 7,
  description = "Module for playing sound effects.",
})

api.type("sound.Sample", {
  backing = "SOUND_SAMPLE_VIEW",
  description = "A sound sample the current level carries. Reach them through `trx.sound.samples`. "
    .. "A handle to a sample the loaded level does not carry goes stale, so `is_valid()` answers "
    .. "whether it is still there.",

  fields = {
    id = {
      from = "id",
      type = "integer",
      writable = false,
      description = "The sample's id, in the level's own numbering.",
    },
    volume = {
      from = "volume",
      type = "integer",
      writable = false,
      description = "The sample's base volume.",
    },
    range = {
      from = "range",
      type = "integer",
      writable = false,
      description = "How far the sample carries.",
    },
    randomness = {
      from = "randomness",
      type = "integer",
      writable = false,
      description = "How much the sample's playback is randomized.",
    },
    pitch = {
      from = "pitch",
      type = "integer",
      writable = false,
      description = "The sample's base pitch.",
    },
  },

  methods = {
    is_valid = {
      returns = { type = "boolean" },
      description = "Whether the loaded level still carries this sample.",
    },
    play = {
      params = {
        {
          name = "opts",
          type = "table",
          optional = true,
          description = "`pos`: a `{ x =, y =, z = }` world position to play from, which applies "
            .. "pan and volume. Omit to play at full volume.",
        },
      },
      description = "Plays this sample.",
    },
  },
})

api.type("sound.Stream", {
  backing = "SOUND_STREAM_VIEW",
  description = "One of the sound effects playing now. Reach them through `trx.sound.streams`. A "
    .. "handle to a voice that has fallen silent goes stale, so check `is_valid()` first.",

  fields = {
    sample_id = {
      from = "sample_id",
      type = "integer",
      writable = false,
      description = "The sample this voice is playing, in the level's own numbering.",
    },
  },

  methods = {
    is_valid = {
      returns = { type = "boolean" },
      description = "Whether this voice is still playing.",
    },
    pause = {
      description = "Pauses this voice.",
    },
    unpause = {
      description = "Resumes this voice.",
    },
    stop = {
      description = "Stops this voice.",
    },
  },
})

-- One lazy view apiece: indexing and iterating reach into C a handle at a time,
-- so neither builds a list up front.
local samples = setmetatable({}, {
  __index = function(_, id)
    if type(id) ~= "number" then
      return nil
    end
    return raw.sample_get(id)
  end,
  __len = function()
    return raw.sample_available_count()
  end,
  __pairs = function()
    local limit = raw.sample_limit()
    local id = -1
    return function()
      id = id + 1
      while id < limit do
        local sample = raw.sample_get(id)
        if sample ~= nil then
          return id, sample
        end
        id = id + 1
      end
    end
  end,
})

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

api.property("sound.samples", {
  type = "table",
  description = "The samples the current level carries, as `trx.sound.Sample` handles keyed by id: "
    .. "`trx.sound.samples[99]` is sample 99, or `nil` if the level has no such sample. `#` counts "
    .. "them, iterating walks them, and both reach one handle at a time.",
  get = function()
    return samples
  end,
})

api.property("sound.streams", {
  type = "table",
  description = "The sound effects playing now, as `trx.sound.Stream` handles. A slot that is "
    .. "silent still answers, with a stale handle. Indexing and iterating reach one handle at a "
    .. "time.",
  get = function()
    return streams
  end,
})

api.define("sound.play", {
  description = "Plays a sound effect. Raises if the sample is not available.",
  params = {
    {
      name = "id",
      type = "integer",
      description = "A sample id in the level's own numbering, as `trx.sound.samples` is keyed by. "
        .. "For a catalog name, convert it with `trx.catalog.to_slot(trx.catalog.Context.SAMPLES, "
        .. "id)` first.",
    },
    {
      name = "opts",
      type = "table",
      optional = true,
      description = "`pos`: a `{ x =, y =, z = }` world position to play from, which applies pan "
        .. "and volume. Omit to play at full volume.",
    },
  },
  examples = {
    [[trx.sound.play(99)
trx.sound.play(99, { pos = { x = 100, y = 200, z = 50 } })]],
  },
  impl = raw.play,
})

api.define("sound.stop", {
  description = "Stops a sound effect.",
  params = {
    {
      name = "id",
      type = "integer",
      description = "A sample id in the level's own numbering, as `trx.sound.samples` is keyed by.",
    },
  },
  impl = raw.stop,
})

api.define("sound.stop_all", {
  description = "Stops every sound effect currently playing.",
  impl = raw.stop_all,
})
