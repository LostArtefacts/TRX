local raw = trxc.cutscenes
local api = trx.api

api.module("cutscenes", {
  order = 13,
  description = [[
    Module for TR4's in-game cutscenes, the animated scenes stored in
    `cutseq.pak` <!--noref: cutseq.pak--> and started by a cutscene trigger. A cutscene plays once:
    the engine remembers which ones have run, and a script may consult or
    rewrite that memory. The cutscene levels of TR1-TR3, which the game flow
    lists and `/cut` plays, are a different thing: see `trx.game.cutscenes`.
  ]],
})

api.number("cutscenes.Num", {
  base = 0,
  description = "Cutscene number, as a cutscene trigger names it.",
})

api.define("cutscenes.play", {
  description = "Plays a cutscene, fading the scene out first. Does nothing if one is already "
    .. "playing or the game has no cutscene data.",
  params = {
    { name = "num", type = "cutscenes.Num" },
  },
  examples = { [[trx.cutscenes.play(28)]] },
  impl = raw.play,
})

api.property("cutscenes.current", {
  type = "cutscenes.Num",
  description = "Number of the cutscene playing, or `nil` if none is.",
  get = raw.get_current,
})

api.property("cutscenes.is_playing", {
  type = "boolean",
  description = "Whether a cutscene is on screen.",
  get = raw.is_playing,
})

api.define("cutscenes.is_played", {
  description = "Whether a cutscene trigger naming this number has already been answered.",
  params = {
    { name = "num", type = "cutscenes.Num" },
  },
  returns = {
    type = "boolean",
    description = "True once it has run, which is what keeps its trigger from firing again.",
  },
  impl = raw.is_played,
})

api.define("cutscenes.set_played", {
  description = [[
    Marks a cutscene as played or unplayed. Marking one as played keeps its
    trigger from firing; unmarking one lets it run again.

    A trigger may name a number the game has no cutscene for - TR4 uses 32 to
    ask for a full-motion video - and the engine remembers those the same way,
    so `trx.events.on_cutscene_trigger` hears about each of them once. This is what clears
    that memory, and it takes any number a trigger may carry, not only the ones
    `trx.cutscenes.play` accepts.
  ]],
  params = {
    { name = "num", type = "cutscenes.Num" },
    {
      name = "played",
      type = "boolean",
      description = "Whether it counts as played.",
    },
  },
  examples = { [[trx.cutscenes.set_played(7, true)]] },
  impl = raw.set_played,
})

api.define("cutscenes.forget_played", {
  description = "Forgets every cutscene, so all of them may run again.",
  impl = raw.forget_played,
})

api.define("cutscenes.set_lara_return", {
  description = [[
    Places Lara where the next cutscene to end leaves her. A cutscene stands
    her at its own origin while it plays and puts her back where it found her
    afterwards; this says to put her somewhere else instead, as the original
    game does for the scenes that carry her along.

    It holds for one cutscene, whether named before `trx.cutscenes.play` or while the scene
    runs, and is forgotten once she has been placed.
  ]],
  params = {
    { name = "pos", type = "math.Vec3", description = "World position." },
    {
      name = "rot",
      type = "math.Angle",
      optional = true,
      description = "Facing angle. Defaults to `0`.",
    },
  },
  examples = {
    [[trx.events.on_cutscene_start(function(num)
  if num == 12 then
    trx.cutscenes.set_lara_return({ x = 38912, y = 2048, z = 51200 })
  end
end)]],
  },
  impl = raw.set_lara_return,
})

api.property("cutscenes.fov", {
  type = "math.Angle",
  description = "Field of view a cutscene plays at. TR4 uses 11488, against 14560 for ordinary "
    .. "play.",
  get = raw.get_fov,
  set = raw.set_fov,
})

api.property("cutscenes.letterbox", {
  type = "number",
  description = "Depth of each cinematic bar, as a fraction of the screen height. `0` removes "
    .. "them.",
  get = raw.get_letterbox,
  set = raw.set_letterbox,
})
