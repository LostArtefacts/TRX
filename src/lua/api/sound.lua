local raw = trxc.sound
local api = trx.api

api.module("sound", {
  order = 20,
  description = "Module for playing sound effects.",
})

api.number("sound.SampleNum", {
  base = 0,
  description = "Sample number, in the numbering the loaded level carries. Not a "
    .. "`trx.catalog.samples` name, which is the sound bank's own.",
})

local PLAY_OPTS = {
  name = "opts",
  type = "table",
  optional = true,
  description = "How to play it.",
  fields = {
    {
      name = "pos",
      type = "math.Vec3",
      optional = true,
      description = "A world position to play from, which applies pan and volume. Omit to "
        .. "play at full volume.",
    },
  },
}

api.number("sound.StreamNum", {
  base = 1,
  description = "Which of the voices playing now, counted in the order the engine holds them.",
})

api.type("sound.Sample", {
  backing = "SOUND_SAMPLE_VIEW",
  description = "A sound sample the current level carries. Reach them through `trx.sound.samples`. "
    .. "A handle to a sample the loaded level does not carry goes stale, so `trx.sound.Sample:is_valid` answers "
    .. "whether it is still there.",

  fields = {
    num = {
      from = "id",
      type = "sound.SampleNum",
      writable = false,
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
      returns = {
        type = "boolean",
        description = "False once a level change has replaced the samples.",
      },
      description = "Whether the loaded level still carries this sample.",
    },
    play = {
      params = { PLAY_OPTS },
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
      type = "sound.SampleNum",
      writable = false,
      description = "The sample this voice is playing.",
    },
  },

  methods = {
    is_valid = {
      returns = {
        type = "boolean",
        description = "False once the voice has fallen silent.",
      },
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

local samples = api.container("sound.samples", {
  description = "The samples the current level carries. A level does not carry every number, "
    .. "so indexing one it lacks is `nil` and iterating passes it by.",
  key = { type = "sound.SampleNum" },
  value = { type = "sound.Sample", nullable = true },
  get = raw.sample_get,
  count = raw.sample_available_count,
  limit = raw.sample_limit,
})

api.container("sound.streams", {
  description = "The sound effects playing now. A slot that is silent still answers, with a "
    .. "stale handle.",
  key = { type = "sound.StreamNum" },
  value = { type = "sound.Stream", nullable = true },
  get = function(n)
    return raw.stream_get(n - 1)
  end,
  count = raw.stream_count,
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
    PLAY_OPTS,
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
