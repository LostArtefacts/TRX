local raw = trxc.events
local api = trx.api

-- The event types, reflected out of ENUM_MAP as any other enum is, so no number
-- is written twice. Not declared with api.enum, and so not public: the named
-- hooks below are the whole surface, and a script never has to name a type.
local types = {}
for _, constant in ipairs(trxc.enum.values("LUA_EVENT_TYPE")) do
  types[constant.name] = constant.value
end

api.module("events", {
  order = 1,
  description = "Lua scripts can listen for game events by attaching a handler to one of the hooks "
    .. "below. Attaching returns a listener id, which `trx.events.detach` takes.\n\n"
    .. "A handler attached from a level script is detached automatically when the level ends; one "
    .. "attached from a global script lives for the whole session.\n\n"
    .. "An event that carries a default the script may take over says so in its description; a "
    .. "handler answers such an event by returning true, and the default then stands down. Every "
    .. "other event ignores what its handlers return.",
})

api.number("events.FlipEffectNum", {
  base = 0,
  description = "A flip effect number, as a level editor numbers them. Not the id space of "
    .. "`trx.rooms.flip_effect`, which takes `trx.catalog.flip_effects` names.",
})

-- What every hook hands back. The engine keys a listener by a number, and the
-- number is the module's business rather than a script's: a listener is worth
-- holding on to, comparing and detaching, and worth nothing else.
local Listener = api.type("events.Listener", {
  description = "An attached handler. Every hook hands one back, and holding it is what makes the "
    .. "handler detachable later. A listener is spent once detached, and a level change spends "
    .. "every one a level script attached.",

  fields = {
    id = {
      type = "integer",
      description = "The number the engine keys the handler by. Two listeners of the same handler "
        .. "carry the same one; it is never handed out twice within a session.",
      get = function(self)
        return rawget(self, "_id")
      end,
    },
  },

  methods = {
    detach = {
      description = "Stops the handler, which fires no more from here on. `trx.events.detach` does "
        .. "the same to a listener held elsewhere.",
      returns = {
        type = "boolean",
        description = "Whether the handler was still attached.",
      },
      impl = function(self)
        return raw.detach(rawget(self, "_id"))
      end,
    },
  },
})

-- A listener carries the engine's number and nothing a script can reach.
local function listener_of(id)
  return setmetatable({ _id = id }, Listener)
end

-- Each hook is a plain function closing over its event type.
local function hook(event_type)
  assert(event_type ~= nil, "events: the engine has no such event type")
  return function(callback)
    return listener_of(raw.attach(event_type, callback))
  end
end

-- trx.cutscenes is reached at call time, so its module need not load before
-- this one.
local function cutscene_hook(event_type)
  assert(event_type ~= nil, "events: the engine has no such event type")
  return function(callback)
    return listener_of(raw.attach(event_type, function(num, ...)
      callback(trx.cutscenes[num], ...)
    end))
  end
end

local LISTENER = {
  type = "events.Listener",
  description = "The attached handler.",
}

api.define("events.on_game_start", {
  description = [[
    Happens as a level starts running, before its first frame is drawn. By then
    the level file is loaded, its items are set up and any savegame state has
    been applied, so this is where a script sets object properties, declares
    allies, changes room state and plays sound effects. Every kind of level
    fires it: a played level, a cutscene and the attract demo alike. The title
    screen has `trx.events.on_title_start` instead.

    Which level is starting is `trx.game.current_level`, whose `trx.game.Level.num` and `trx.game.Level.type`
    say where it counts and what kind it is. A level script already knows both,
    which is why the handler is not handed them.
  ]],
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "is_save",
          type = "boolean",
          description = "Whether the level is being resumed from a savegame rather than started "
            .. "fresh. A cutscene and a demo are never resumed, and always report false.",
        },
      },
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_game_start(function(is_save)
  trx.log.info(trx.game.current_level.title .. " is up")
end)]],
  },
  impl = hook(types.GAME_START),
})

api.define("events.on_title_start", {
  description = [[
    Happens when the title screen comes up, once its level is loaded and its
    items are set up. The handler takes no arguments.
    `trx.events.on_game_start` does not fire for the title level.

    A title that shows a picture rather than playing its level behind the menu
    does not run its logic, so `trx.events.before_control` and
    `trx.events.after_control` handlers attached here never fire there. This
    says the menu is up; it does not promise a scene playing behind it.
  ]],
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_title_start(function()
  trx.log.info("the menu is up")
end)]],
  },
  impl = hook(types.TITLE_START),
})

api.define("events.on_level_unload", {
  description = [[
    Happens as the engine lets go of a level, before the handlers a level script
    attached are detached and before the world the script was written against is
    taken apart. This is where a script hands back what it set up while the
    level it set it up in is still there to read. The handler takes no
    arguments.

    A level change fires it for the outgoing level, and so does leaving the game
    for the title screen or for the desktop. Re-running a level's script without
    changing level fires it for the run being replaced. The unload that opens
    the first level of a session has nothing to let go of and stays quiet.
  ]],
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_level_unload(function()
  trx.log.info("packing up")
end)]],
  },
  impl = hook(types.LEVEL_UNLOAD),
})

api.define("events.on_tick", {
  description = [[
    Happens once for every tick the game runs, whatever is on screen: while a
    level is played, while a menu is open, over a cutscene and through a fade.

    This is the clock a script keeps its own state on. It is not the world
    stepping - `trx.events.before_control` is that, and it happens only while
    a level is running - and it is not a frame reaching the screen, which
    happens twice as often while frames are interpolated.
  ]],
  params = {
    {
      name = "callback",
      type = "function",
      description = "Called once per tick.",
    },
  },
  returns = { type = "integer", description = "The listener id." },
  impl = hook(types.TICK),
})

api.define("events.on_ui_draw", {
  description = [[
    Fires once for each of the nine on-screen UI regions on every drawn frame.
    The callback receives the current `trx.ui.Region`.

    Use this event to reserve layout space, not to draw. Call
    `trx.ui.primitive.reserve` during this event, then draw into the assigned
    box later during `trx.events.on_ui_paint`. `trx.ui.regions.place` handles
    both steps for widgets.

    This event fires anywhere the game draws UI, including fades, FMVs, and
    normal gameplay. It follows the frame rate, not the game clock.
  ]],
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
    },
  },
  returns = LISTENER,
  examples = {
    [[local slot = nil

trx.events.on_ui_draw(function(region)
  if region == trx.ui.Region.TOP_CENTER then
    local w, h = trx.ui.primitive.measure_text("hello")
    slot = trx.ui.primitive.reserve(region, w, h)
  end
end)

trx.events.on_ui_paint(function()
  local x, y = trx.ui.primitive.slot_box(slot)
  if x ~= nil then
    trx.ui.primitive.text("hello", x, y)
  end
end)]],
  },
  impl = hook(types.UI_DRAW),
})

api.define("events.on_ui_paint", {
  description = [[
    Fires after UI layout and before drawing. Reservation boxes are available
    during this event.

    Use this event to draw into space reserved earlier during
    `trx.events.on_ui_draw`. Primitive drawing calls are available during this
    event only.
  ]],
  params = {
    {
      name = "callback",
      type = "function",
      description = "Called once per painted scene.",
    },
  },
  returns = { type = "integer", description = "The listener id." },
  impl = hook(types.UI_PAINT),
})

api.define("events.on_pickup", {
  description = "Happens just after Lara picks up an item.",
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "item_num",
          type = "items.Num",
          description = "The item that was picked up.",
        },
      },
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_pickup(function(item_num)
  trx.log.info(trx.items[item_num].object_id)
end)]],
  },
  impl = hook(types.PICKUP),
})

api.define("events.before_control", {
  description = "Happens on every logical game frame, before the main game logic runs. The handler "
    .. "takes no arguments.",
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
    },
  },
  returns = LISTENER,
  impl = hook(types.BEFORE_CONTROL),
})

api.define("events.after_control", {
  description = "Happens on every logical game frame, after the main game logic runs. The handler "
    .. "takes no arguments.",
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
    },
  },
  returns = LISTENER,
  impl = hook(types.AFTER_CONTROL),
})

api.define("events.on_flip_effect", {
  description = "Claims a `trx.events.FlipEffectNum` and happens whenever a level runs it, "
    .. "whether from a floor trigger or an animation command. Place an ordinary flipeffect "
    .. "trigger in a level editor - pad, heavy, switch and antitrigger all work - pick one "
    .. "nothing uses, and handle it here from the level's script.\n\n"
    .. "A claimed number belongs to the script for the rest of the level: its stock engine effect "
    .. "does not run, even if the handler is later detached. Unclaimed numbers are unaffected.\n\n"
    .. "Unlike the other hooks, this happens at effect execution time, in the middle of a game "
    .. "frame.",
  params = {
    {
      name = "effect_num",
      type = "events.FlipEffectNum",
      description = "The one to claim.",
    },
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "timer",
          type = "integer",
          description = "A floor trigger's timer field, free for the level to use as a parameter. "
            .. "0 for an animation command, which carries no timer.",
        },
        {
          name = "item_num",
          type = "items.Num",
          description = "The item that ran the effect: Lara for a pad trigger, "
            .. "the activating object for a heavy trigger, the animating item for an animation "
            .. "command.",
        },
      },
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_flip_effect(62, function(timer, item_num)
  trx.log.info("flipeffect 62 ran with timer " .. timer)
end)]],
  },
  impl = function(effect_num, callback)
    return listener_of(raw.attach(types.FLIP_EFFECT, callback, effect_num))
  end,
})

api.define("events.on_room_change", {
  description = "Happens when an item changes rooms during play, which a cutscene or the attract "
    .. "demo is not. `trx.rooms.Room:on_enter` and `trx.rooms.Room:on_exit` are this same event, narrowed to "
    .. "one room.",
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "item",
          type = "items.Item",
          description = "The item that changed rooms.",
        },
        {
          name = "old_room_num",
          type = "rooms.Num",
          description = "-1 if it had none.",
        },
        {
          name = "new_room_num",
          type = "rooms.Num",
          description = "-1 if it left the world.",
        },
      },
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_room_change(function(item, old_room_num, new_room_num)
  trx.log.info(item.object_id .. " moved to room " .. new_room_num)
end)]],
  },
  impl = function(callback)
    return listener_of(
      raw.attach(
        types.ROOM_CHANGE,
        function(item_num, old_room_num, new_room_num)
          callback(trx.items[item_num], old_room_num, new_room_num)
        end
      )
    )
  end,
})

api.define("events.on_trigger", {
  description = "Happens every time a trigger is aimed at an item - a floor trigger in the level, "
    .. "the `/trigger` console command, or `trx.items.Item:trigger` from a script - of any kind, an "
    .. "antitrigger included. It is the raw trigger, not a state change: a floor pad fires it every "
    .. "frame Lara stands on it, and a partial trigger fires it too. A cutscene or the attract demo "
    .. "does not.\n\n"
    .. "The handler runs after the trigger has been applied, so the item already reflects it, and "
    .. "changes the handler makes to the item are not overwritten.\n\n"
    .. "`trx.items.Item:on_trigger` is this same event, narrowed to one item.",
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "item",
          type = "items.Item",
          description = "The item the trigger was aimed at.",
        },
        {
          name = "trigger",
          type = "items.Trigger",
          description = "What the trigger carried.",
        },
      },
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_trigger(function(item, trigger)
  if trigger.type == trx.items.TriggerType.ANTITRIGGER then
    trx.log.info(item.object_id .. " was antitriggered")
  end
end)]],
  },
  impl = function(callback)
    return listener_of(
      raw.attach(types.TRIGGER, function(item_num, kind, mask, timer, one_shot)
        callback(trx.items[item_num], {
          type = kind,
          mask = mask,
          timer = timer,
          one_shot = one_shot,
        })
      end)
    )
  end,
})

local function visibility_hook(event_type, method, verb, examples)
  return {
    description = "Happens when an item becomes "
      .. verb
      .. " during play - drawn and in the "
      .. "world, taking part in collision and targeting. It is the change that fires, not the "
      .. "state: an item already "
      .. verb
      .. " does not fire it again, and only a live level "
      .. "does, not a cutscene or the attract demo.\n\n"
      .. "`trx.items.Item:on_"
      .. method
      .. "` is this same event, narrowed to one item.",
    params = {
      {
        name = "callback",
        type = "function",
        description = "What to run when it happens.",
        params = {
          {
            name = "item",
            type = "items.Item",
            description = "The item that became " .. verb .. ".",
          },
        },
      },
    },
    returns = LISTENER,
    examples = examples,
    impl = function(callback)
      return listener_of(raw.attach(event_type, function(item_num)
        callback(trx.items[item_num])
      end))
    end,
  }
end

-- The item-lifecycle hooks share a shape: one Item argument, and a per-item
-- narrowing under trx.items.Item:on_*. Only the wording and the type differ.
local function item_lifecycle_hook(
  event_type,
  description,
  item_desc,
  examples
)
  return {
    description = description,
    params = {
      {
        name = "callback",
        type = "function",
        description = "What to run when it happens.",
        params = {
          {
            name = "item",
            type = "items.Item",
            description = item_desc,
          },
        },
      },
    },
    returns = LISTENER,
    examples = examples,
    impl = function(callback)
      return listener_of(raw.attach(event_type, function(item_num)
        callback(trx.items[item_num])
      end))
    end,
  }
end

api.define(
  "events.on_show",
  visibility_hook(types.SHOW, "show", "visible", {
    [[trx.events.on_show(function(item)
  trx.log.info(item.object_id .. " appeared")
end)]],
  })
)

api.define(
  "events.on_hide",
  visibility_hook(types.HIDE, "hide", "hidden", {
    [[trx.events.on_hide(function(item)
  trx.log.info(item.object_id .. " vanished")
end)]],
  })
)

api.define(
  "events.on_finish",
  item_lifecycle_hook(
    types.FINISH,
    "Happens when an item finishes its run during play - a trap that has sprung, a switch thrown, "
      .. "a one-shot object spent. It is the change that fires, once, and only a live level does, "
      .. "not a cutscene or the attract demo.\n\n"
      .. "`trx.items.Item:on_finish` is this same event, narrowed to one item.",
    "The item that finished.",
    {
      [[trx.events.on_finish(function(item)
  trx.log.info(item.object_id .. " finished its run")
end)]],
    }
  )
)

api.define(
  "events.on_enter_sim",
  item_lifecycle_hook(
    types.ENTER_SIM,
    "Happens when an item starts being simulated during play - its control routine begins running "
      .. "each frame. Every path that starts an item fires it: a trigger, a switch, a respawn, a "
      .. "cheat. A trigger also fires `trx.events.on_activate`, which this does not.\n\n"
      .. "`trx.items.Item:on_enter_sim` is this same event, narrowed to one item.",
    "The item that started being simulated.",
    {
      [[trx.events.on_enter_sim(function(item)
  trx.log.info(item.object_id .. " started running")
end)]],
    }
  )
)

api.define(
  "events.on_leave_sim",
  item_lifecycle_hook(
    types.LEAVE_SIM,
    "Happens when an item stops being simulated during play - its control routine no longer runs. "
      .. "It keeps its place and its state; it merely stops.\n\n"
      .. "`trx.items.Item:on_leave_sim` is this same event, narrowed to one item.",
    "The item that stopped being simulated.",
    {
      [[trx.events.on_leave_sim(function(item)
  trx.log.info(item.object_id .. " stopped running")
end)]],
    }
  )
)

api.define(
  "events.on_activate",
  item_lifecycle_hook(
    types.ACTIVATE,
    "Happens when an item is activated through the lifecycle front door during play - the path a "
      .. "level trigger takes. Switches, respawns and cheats start an item without it, firing only "
      .. "`trx.events.on_enter_sim`; watch that one for a start of any cause.\n\n"
      .. "`trx.items.Item:on_activate` is this same event, narrowed to one item.",
    "The item that was activated.",
    {
      [[trx.events.on_activate(function(item)
  trx.log.info(item.object_id .. " was activated")
end)]],
    }
  )
)

api.define(
  "events.on_deactivate",
  item_lifecycle_hook(
    types.DEACTIVATE,
    "Happens when a running item is deactivated through the lifecycle front door during play - the "
      .. "path an antitrigger takes. It fires only when the item was actually running.\n\n"
      .. "`trx.items.Item:on_deactivate` is this same event, narrowed to one item.",
    "The item that was deactivated.",
    {
      [[trx.events.on_deactivate(function(item)
  trx.log.info(item.object_id .. " was deactivated")
end)]],
    }
  )
)

api.define(
  "events.on_destroy",
  item_lifecycle_hook(
    types.DESTROY,
    "Happens as an item is removed from the game during play - a creature cleared away, a pickup "
      .. "taken, an object that has run its course. The item can still be read from the handler, "
      .. "which runs before the removal completes, but a handle kept past the handler goes stale.\n\n"
      .. "`trx.items.Item:on_destroy` is this same event, narrowed to one item.",
    "The item being removed. Valid only for the duration of the handler.",
    {
      [[trx.events.on_destroy(function(item)
  trx.log.info(item.object_id .. " was removed")
end)]],
    }
  )
)

api.define(
  "events.on_enter_world",
  item_lifecycle_hook(
    types.ENTER_WORLD,
    "Happens when an item enters the world during play - a runtime spawn, such as a creature an "
      .. "emitter releases or an item a script creates. The level's own items do not fire it as "
      .. "they load; only an arrival during a live level counts.\n\n"
      .. "`trx.items.Item:on_enter_world` is this same event, narrowed to one item.",
    "The item that entered the world.",
    {
      [[trx.events.on_enter_world(function(item)
  trx.log.info(item.object_id .. " entered the world")
end)]],
    }
  )
)

api.define(
  "events.on_leave_world",
  item_lifecycle_hook(
    types.LEAVE_WORLD,
    "Happens when an item leaves the world during play - unlinked from its room, no longer drawn "
      .. "or collidable. It need not be destroyed; a destroyed item leaves the world on its way "
      .. "out, and fires this first.\n\n"
      .. "`trx.items.Item:on_leave_world` is this same event, narrowed to one item.",
    "The item that left the world.",
    {
      [[trx.events.on_leave_world(function(item)
  trx.log.info(item.object_id .. " left the world")
end)]],
    }
  )
)

api.define("events.on_hit", {
  description = [[
    Happens when an item takes damage, Lara included. It is the raw damage that
    fires, before the item's hit points are clamped, so a fatal blow reports
    the whole amount the attacker dealt. A death that does not go through
    damage - a script writing `trx.items.Item.hit_points`, or
    `trx.items.Item:destroy` - does not report.

    `trx.items.Item:on_hit` is this same event, narrowed to one item.
  ]],
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "item",
          type = "items.Item",
          description = "The item that took the damage.",
        },
        {
          name = "damage",
          type = "integer",
          description = "Hit points taken, before clamping to zero.",
        },
      },
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_hit(function(item, damage)
  trx.log.info(item.object_id .. " lost " .. damage .. " hit points")
end)]],
  },
  impl = function(callback)
    return listener_of(raw.attach(types.HIT, function(item_num, damage)
      callback(trx.items[item_num], damage)
    end))
  end,
})

api.define("events.on_kill", {
  description = [[
    Happens when damage takes an item's hit points to zero, Lara included. It
    is the same blow `trx.events.on_hit` reports, which fires first. A death
    that does not go through damage - a script writing
    `trx.items.Item.hit_points`, or `trx.items.Item:destroy` - does not report.

    Some bosses fall and get back up: Willard is knocked out, Natla plays dead
    before her second stage, and the dragon lies still until Lara takes the
    dagger. Each stage brings their hit points to zero, so they report once per
    stage rather than once per boss, and the dragon reports once more for the
    dagger that ends it.

    `trx.items.Item:on_kill` is this same event, narrowed to one item.
  ]],
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "item",
          type = "items.Item",
          description = "The item that was brought down.",
        },
      },
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_kill(function(item)
  trx.log.info(item.object_id .. " is down")
end)]],
  },
  impl = function(callback)
    return listener_of(raw.attach(types.KILL, function(item_num)
      callback(trx.items[item_num])
    end))
  end,
})

api.define("events.on_cutscene_trigger", {
  description = [[
    Happens when a cutscene trigger fires, before the engine acts on it. A
    handler answers the trigger by returning true - having played a cutscene of
    its own, run something else, or decided nothing should run. If no handler
    answers, the engine plays the cutscene the trigger names.

    A trigger Lara stands on fires every frame, so this happens only for a
    cutscene that has not run yet and while none is playing. Asking counts as
    running it, however it ended, so the same handler is not asked again on
    the next frame. Clear the mark by writing `trx.cutscenes.Cutscene.is_played` to hear
    about one again.

    The number a trigger names need not be one the game has a cutscene for -
    TR4 uses 32 to ask for a full-motion video. Those reach a handler too, and
    the engine has nothing of its own to do about them.
  ]],
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "cutscene_num",
          type = "cutscenes.Num",
          description = "The number the trigger names, which the game need not have a cutscene "
            .. "for.",
        },
      },
    },
  },
  returns = LISTENER,
  examples = {
    [[-- only in the throne room; a flyby stands in for it elsewhere
trx.events.on_cutscene_trigger(function(cutscene_num)
  if cutscene_num ~= 27 then
    return false
  end
  if trx.lara.item.room_num == 55 then
    trx.cutscenes[27]:play()
  else
    trx.camera.play_flyby(3)
  end
  return true
end)]],
  },
  impl = hook(types.CUTSCENE_TRIGGER),
})

api.define("events.on_cutscene_start", {
  description = "Happens when a TR4 cutscene's first frame is about to show, after the fade out.",
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "cutscene",
          type = "cutscenes.Cutscene",
          description = "The cutscene starting.",
        },
      },
    },
  },
  returns = LISTENER,
  impl = cutscene_hook(types.CUTSCENE_START),
})

api.define("events.on_cutscene_frame", {
  description = [[
    Happens on every frame of a TR4 cutscene, before the frame is posed.

    A cutscene has no items to listen to. Its actors are animation tracks, so
    the frame number is the only thing a script can act on. The original game
    keys its own cutscene events to frame numbers as well.
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
          description = "The cutscene the frame belongs to.",
        },
        {
          name = "frame_num",
          type = "cutscenes.FrameNum",
        },
      },
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_cutscene_frame(function(cutscene, frame_num)
  if cutscene.num == 5 and frame_num == 1350 then
    -- something happens here
  end
end)]],
  },
  impl = cutscene_hook(types.CUTSCENE_FRAME),
})

api.define("events.on_cutscene_end", {
  description = "Happens once a TR4 cutscene has finished and the scene it interrupted is back. "
    .. "This is where a script decides what follows.",
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "cutscene",
          type = "cutscenes.Cutscene",
          description = "The cutscene that finished.",
        },
      },
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_cutscene_end(function(cutscene)
  trx.log.info("cutscene " .. cutscene.num .. " finished")
end)]],
  },
  impl = cutscene_hook(types.CUTSCENE_END),
})

api.define("events.on_flyby_end", {
  description = [[
Happens when a flyby sequence reaches its last camera and hands the view back.
A sequence that a cutscene or the player interrupts does not fire it.]],
  params = {
    {
      name = "callback",
      type = "function",
      description = "What to run when it happens.",
      params = {
        {
          name = "sequence_num",
          type = "camera.SequenceNum",
        },
      },
    },
  },
  returns = LISTENER,
  examples = {
    [[trx.events.on_flyby_end(function(sequence_num)
  trx.camera.play_flyby(sequence_num)
end)]],
  },
  impl = hook(types.FLYBY_END),
})

api.define("events.detach", {
  description = "Removes a previously attached handler, which stops firing immediately. "
    .. "`trx.events.Listener:detach` does the same to one held in hand.",
  params = {
    {
      name = "listener",
      type = "events.Listener",
      description = "What the hook handed back when the handler was attached.",
    },
  },
  returns = {
    type = "boolean",
    description = "Whether the handler was still attached. `false` means it had already been "
      .. "detached, or the level it belonged to has ended.",
  },
  examples = {
    [[local listener = trx.events.before_control(function()
  -- handle control loop event
end)
trx.events.detach(listener)]],
  },
  impl = function(listener)
    return listener:detach()
  end,
})
