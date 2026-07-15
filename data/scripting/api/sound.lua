local raw = trxc.sound
local api = trx.api

api.module("sound", {
  order = 7,
  description = "Module for playing sound effects.",
})

api.define("sound.is_available", {
  description = "Whether a sound sample exists in the current level.",
  params = {
    {
      name = "id",
      type = "integer",
      enum = "catalog.samples",
      description = "Sample to test.",
    },
  },
  returns = { type = "boolean" },
  impl = raw.is_available,
})

api.define("sound.play", {
  description = "Plays a sound effect. Raises if the sample is not available.",
  params = {
    {
      name = "id",
      type = "integer",
      enum = "catalog.samples",
      description = "Sample to play.",
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
      enum = "catalog.samples",
      description = "Sample to stop.",
    },
  },
  impl = raw.stop,
})

api.define("sound.stop_all", {
  description = "Stops every sound effect currently playing.",
  impl = raw.stop_all,
})
