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

api.number("cutscenes.FrameNum", {
  base = 0,
  description = "A frame's number within the cutscene it belongs to.",
})

api.define("cutscenes.play", {
  description = "Plays a cutscene, fading the scene out first. Does nothing if one is already "
    .. "playing or the game has no cutscene data.",
  params = {
    { name = "num", type = "cutscenes.Num" },
    {
      name = "fade",
      type = "boolean",
      optional = true,
      description = "Whether to fade the scene out before the first frame. Defaults to true. "
        .. "A cutscene that opens a level passes false: the original game holds the screen "
        .. "black rather than showing the level for a moment first, and the scene's own fade "
        .. "in follows either way.",
    },
  },
  examples = { [[trx.cutscenes.play(28)]] },
  impl = raw.play,
})

api.property("cutscenes.current", {
  type = "cutscenes.Num",
  description = "Number of the cutscene playing, or `nil` if none is.",
  get = raw.get_current,
})

api.property("cutscenes.frame_num", {
  type = "cutscenes.FrameNum",
  description = [[
    Which frame of the running cutscene is on screen, or `nil` if none is
    running. A cutscene's actors are animation tracks rather than items, so
    nothing in it can be triggered or listened to; naming a frame is how a
    script acts part-way through one, as the original game does.
  ]],
  examples = {
    [[trx.events.after_control(function()
  if trx.cutscenes.current == 5 and trx.cutscenes.frame_num == 1350 then
    -- something happens here
  end
end)]],
  },
  get = raw.get_frame_num,
})

api.property("cutscenes.is_playing", {
  type = "boolean",
  description = "Whether a cutscene is on screen.",
  get = raw.is_playing,
})

api.property("cutscenes.count", {
  type = "integer",
  description = "How many cutscenes this game can play. `0` where it has none, which is every "
    .. "game but TR4 and a TR4 install with no `cutseq.pak` <!--noref: cutseq.pak--> beside its "
    .. "levels.",
  get = raw.get_count,
})

api.number("cutscenes.ActorNum", {
  base = 0,
  description = [[
    Which of a cutscene's actors. Actor `0` is Lara, who is posed rather than
    drawn as an actor; the cast a scene brings with it starts at `1`.
  ]],
})

api.number("cutscenes.NodeNum", {
  base = 0,
  description = "Which of an actor's meshes, the root being the first.",
})

api.property("cutscenes.actor_count", {
  type = "integer",
  description = "How many actors the running cutscene has, or `0` if none is running.",
  get = raw.get_actor_count,
})

api.define("cutscenes.set_actor_visible", {
  description = [[
    Whether an actor is drawn. A scene brings its whole cast on from its first
    frame, so an actor who is only due later is hidden until then, as the
    original game hides one.

    It lasts as long as the cutscene, and every actor starts out visible.
  ]],
  params = {
    { name = "actor", type = "cutscenes.ActorNum" },
    {
      name = "visible",
      type = "boolean",
      description = "Whether the actor is drawn.",
    },
  },
  examples = {
    [[trx.events.on_cutscene_start(function(num)
  if num == 9 then
    trx.cutscenes.set_actor_visible(3, false)
  end
end)]],
  },
  impl = raw.set_actor_visible,
})

api.define("cutscenes.set_node_mesh", {
  description = [[
    Draws another object's mesh in place of the one an actor's node carries.
    This is how a talking head goes on a body: the speech-head objects hold a
    mouth in each shape, and swapping between them while a line plays is what
    the original game animates speech with.

    Raises if this level does not carry the object.
  ]],
  params = {
    { name = "actor", type = "cutscenes.ActorNum" },
    { name = "node", type = "cutscenes.NodeNum" },
    {
      name = "object",
      type = "catalog.objects",
      description = "The object to take a mesh from.",
    },
    {
      name = "mesh_num",
      type = "integer",
      optional = true,
      description = "Which of that object's meshes. Defaults to `0`.",
    },
  },
  examples = {
    [[trx.cutscenes.set_node_mesh(1, 21, trx.catalog.objects.actor_1_speech_head_1)]],
  },
  impl = raw.set_node_mesh,
})

api.define("cutscenes.clear_node_mesh", {
  description = "Takes the override back off, leaving the mesh the actor's own object gives "
    .. "that node.",
  params = {
    { name = "actor", type = "cutscenes.ActorNum" },
    { name = "node", type = "cutscenes.NodeNum" },
  },
  impl = raw.clear_node_mesh,
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

api.define("cutscenes.set_lara_shadow_bounds", {
  description = [[
    Gives Lara's shadow another box for the running cutscene. A scene holds one
    box for the whole of it, rather than the box her pose would make, so that
    her shadow keeps a steady size while the scene moves her; this names a
    different one. A wide box is how the original game makes her shadow read as
    the jeep's when she arrives at Karnak.

    It holds for one cutscene, whether named before `trx.cutscenes.play` or
    while the scene runs, and the next scene starts from the ordinary box
    again.
  ]],
  params = {
    {
      name = "bounds",
      type = "math.Box",
      description = "The box, in Lara's own frame.",
    },
  },
  examples = {
    [[trx.events.on_cutscene_start(function(num)
  if num == 12 then
    trx.cutscenes.set_lara_shadow_bounds({
      min_x = -600, min_y = -777, min_z = -600,
      max_x = 600, max_y = 1, max_z = 600,
    })
  end
end)]],
  },
  impl = raw.set_lara_shadow_bounds,
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
