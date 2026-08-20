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

-- Maps each handle to the cutscene number it stands for. A handle is an empty
-- table, so the number is reachable only through this map.
local nums = setmetatable({}, { __mode = "k" })
local handles = {}
local Cutscene

local function num_of(self)
  return nums[self]
end

-- Returns one handle per number, so that two ways of reaching the same scene
-- give the same value and compare equal.
local function handle_of(num)
  if
    type(num) ~= "number"
    or num % 1 ~= 0
    or num < 0
    or num >= raw.MAX_TRIGGERS
  then
    return nil
  end
  local handle = handles[num]
  if handle == nil then
    handle = setmetatable({}, Cutscene)
    nums[handle] = num
    handles[num] = handle
  end
  return handle
end

-- Narrows one of the module's events to a single scene. trx.events is reached
-- at call time, so its module need not load before this one.
local function cutscene_hook(event_name)
  return function(self, callback)
    return trx.events[event_name](function(fired, ...)
      if fired == self then
        callback(self, ...)
      end
    end)
  end
end

local CUTSCENE_LISTENER = {
  type = "events.Listener",
  description = "The attached handler.",
}

Cutscene = api.type("cutscenes.Cutscene", {
  description = [[
    One of the scenes a cutscene trigger can name. A number the pak holds no
    scene for is still one of these, because the engine remembers it as played
    the same way; `trx.cutscenes.Cutscene:play` is what such a number has
    nothing to do.
  ]],

  fields = {
    num = {
      type = "cutscenes.Num",
      description = "Which scene this is.",
      get = num_of,
    },

    is_played = {
      type = "boolean",
      description = "Whether a trigger naming this number has already been "
        .. "answered. True keeps its trigger from firing; writing false lets "
        .. "it run again.",
      get = function(self)
        return raw.is_played(num_of(self))
      end,
      set = function(self, value)
        raw.set_played(num_of(self), value and true or false)
      end,
    },

    is_playing = {
      type = "boolean",
      description = "Whether this scene is the one on screen.",
      get = function(self)
        return raw.get_current() == num_of(self)
      end,
    },

    frame_num = {
      type = "cutscenes.FrameNum",
      nullable = true,
      description = "Which frame of this scene is on screen, or `nil` unless "
        .. "it is the one playing.",
      get = function(self)
        if raw.get_current() ~= num_of(self) then
          return nil
        end
        return raw.get_frame_num()
      end,
    },
  },

  methods = {
    play = {
      description = "Plays this scene. Does nothing if one is already playing "
        .. "or the game holds no scene for this number.",
      params = {
        {
          name = "opts",
          type = "table",
          optional = true,
          description = "How to play it.",
          fields = {
            {
              name = "fade",
              type = "boolean",
              optional = true,
              default = true,
              description = "Whether to fade the scene out before the first "
                .. "frame. A cutscene that opens a level passes false: the "
                .. "original game holds the screen black rather than showing "
                .. "the level for a moment first, and the scene's own fade in "
                .. "follows either way.",
            },
          },
        },
      },
      examples = { [[trx.cutscenes[28]:play()]] },
      impl = function(self, opts)
        local fade = true
        if opts ~= nil and opts.fade ~= nil then
          fade = opts.fade
        end
        raw.play(num_of(self), fade)
      end,
    },

    on_start = {
      description = "Happens when this scene's first frame is about to show. "
        .. "`trx.events.on_cutscene_start`, narrowed to this cutscene.",
      params = {
        {
          name = "callback",
          type = "function",
          description = "What to run when it happens.",
          params = {
            {
              name = "cutscene",
              type = "cutscenes.Cutscene",
              description = "This cutscene.",
            },
          },
        },
      },
      returns = CUTSCENE_LISTENER,
      impl = cutscene_hook("on_cutscene_start"),
    },

    on_frame = {
      description = [[
        Happens on every frame of this scene, before the frame is posed.

        A cutscene has no items to listen to. Its actors are animation
        tracks, so the frame number is the only thing a script can act on.

        `trx.events.on_cutscene_frame`, narrowed to this cutscene.
      ]],
      params = {
        {
          name = "callback",
          type = "function",
          description = "What to run when it happens.",
          params = {
            {
              name = "cutscene",
              type = "cutscenes.Cutscene",
              description = "This cutscene.",
            },
            {
              name = "frame_num",
              type = "cutscenes.FrameNum",
              description = "The frame about to be posed.",
            },
          },
        },
      },
      returns = CUTSCENE_LISTENER,
      examples = {
        [[trx.cutscenes[5]:on_frame(function(cutscene, frame_num)
  if frame_num == 1350 then
    -- something happens here
  end
end)]],
      },
      impl = cutscene_hook("on_cutscene_frame"),
    },

    on_end = {
      description = "Happens once this scene has finished and what it "
        .. "interrupted is back. `trx.events.on_cutscene_end`, narrowed to "
        .. "this cutscene.",
      params = {
        {
          name = "callback",
          type = "function",
          description = "What to run when it happens.",
          params = {
            {
              name = "cutscene",
              type = "cutscenes.Cutscene",
              description = "This cutscene.",
            },
          },
        },
      },
      returns = CUTSCENE_LISTENER,
      impl = cutscene_hook("on_cutscene_end"),
    },
  },
})

api.container("cutscenes", {
  description = "Indexing the module reaches a cutscene by the number a trigger names it with. "
    .. "`#trx.cutscenes` is how many the game can play, and `pairs()` walks those in order; the "
    .. "numbers past them are reachable as well, because the engine remembers any of them as "
    .. "played.",
  key = {
    type = "cutscenes.Num",
    description = "The number a cutscene trigger names.",
  },
  value = { type = "cutscenes.Cutscene", nullable = true },
  examples = {
    [[trx.cutscenes[30]:on_frame(function(cutscene, frame_num)
  trx.log.info("frame " .. frame_num .. " of " .. cutscene.num)
end)]],
  },
  get = handle_of,
  count = raw.get_count,
})

api.define("cutscenes.play", {
  deprecated = "Call `trx.cutscenes.Cutscene:play` instead.",
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
  impl = raw.play,
})

api.property("cutscenes.current", {
  type = "cutscenes.Cutscene",
  nullable = true,
  description = "The cutscene playing, or `nil` if none is.",
  get = function()
    return handle_of(raw.get_current())
  end,
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
    [[trx.cutscenes[5]:on_frame(function(cutscene, frame_num)
  if frame_num == 1350 then
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
  deprecated = "Read `trx.cutscenes.Cutscene.is_played` instead.",
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
  deprecated = "Write `trx.cutscenes.Cutscene.is_played` instead.",
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
