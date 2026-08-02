local raw = trxc.sound
local api = trx.api

api.module("sound", {
  order = 17,
  description = "Module for playing sound effects.",
})

api.type("sound.Sample", {
  backing = "SOUND_SAMPLE_VIEW",
  description = "A sound sample the current level carries. Reach them through `trx.sound.samples`. "
    .. "A handle to a sample the loaded level does not carry goes stale, so `trx.sound.Sample:is_valid` answers "
    .. "whether it is still there.",

  fields = {
    num = {
      from = "id",
      type = "integer",
      writable = false,
      description = "The number the level gives this sample.",
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
      returns = {
        type = "sound.Stream",
        nullable = true,
        description = "The voice it started, or `nil` if none did.",
      },
      description = "Plays this sample.",
    },
    stop = {
      description = "Stops every voice playing this sample.",
    },
  },
})

api.type("sound.Stream", {
  backing = "SOUND_STREAM_VIEW",
  description = "One of the sound effects playing now. Reach them through `trx.sound.streams`. A "
    .. "handle to a voice that has fallen silent goes stale, so check `trx.sound.Stream:is_valid` first.",

  fields = {
    sample_num = {
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
  description = "Plays a sound effect by catalog id, mapping it to the level's own sample. A game "
    .. "that does not carry the sample plays nothing.",
  params = {
    {
      name = "id",
      type = "catalog.samples",
      description = "Sample to play. To reach a sample by the level's own slot, play it through a "
        .. "handle: `trx.sound.samples[slot]:play()`.",
    },
    {
      name = "opts",
      type = "table",
      optional = true,
      description = "`pos`: a `{ x =, y =, z = }` world position to play from, which applies pan "
        .. "and volume. Omit to play at full volume.",
    },
  },
  returns = {
    type = "sound.Stream",
    nullable = true,
    description = "The voice it started, or `nil` if none did.",
  },
  examples = {
    [[trx.sound.play(trx.catalog.samples.LARA_NO)
trx.sound.play(trx.catalog.samples.LARA_NO, { pos = { x = 100, y = 200, z = 50 } })]],
  },
  impl = function(id, opts)
    local slot = trx.catalog.to_slot(trx.catalog.Context.SAMPLES, id)
    local sample = slot ~= nil and samples[slot] or nil
    return sample ~= nil and sample:play(opts) or nil
  end,
})

api.define("sound.stop", {
  description = "Stops a sound effect by catalog id.",
  params = {
    {
      name = "id",
      type = "catalog.samples",
      description = "Sample to stop. To reach a sample by the level's own slot, stop it through a "
        .. "handle: `trx.sound.samples[slot]:stop()`.",
    },
  },
  impl = function(id)
    local slot = trx.catalog.to_slot(trx.catalog.Context.SAMPLES, id)
    local sample = slot ~= nil and samples[slot] or nil
    if sample ~= nil then
      sample:stop()
    end
  end,
})

api.define("sound.stop_all", {
  description = "Stops every sound effect currently playing.",
  impl = raw.stop_all,
})
