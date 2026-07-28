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
    .. "attached from a global script lives for the whole session.",
})

-- Each hook is a plain function closing over its event type.
local function hook(event_type)
  assert(event_type ~= nil, "events: the engine has no such event type")
  return function(callback)
    return raw.attach(event_type, callback)
  end
end

local LISTENER_ID = {
  type = "integer",
  description = "Listener id. Pass it to `trx.events.detach` to stop listening.",
}

api.define("events.on_game_start", {
  description = [[
    Happens as a level starts running, before its first frame is drawn. By then
    the level file is loaded, its items are set up and any savegame state has
    been applied, so this is where a script sets object properties, declares
    allies, changes room state and plays sound effects. Every kind of level
    fires it: a played level, a cutscene and the attract demo alike. The title
    screen has `on_title_start` instead.
  ]],
  params = {
    {
      name = "callback",
      type = "function",
      params = {
        {
          name = "level_num",
          type = "integer",
          description = "Number of the level being started.",
        },
        {
          name = "is_save",
          type = "boolean",
          description = "Whether the level is being resumed from a savegame rather than started fresh.",
        },
      },
    },
  },
  returns = LISTENER_ID,
  impl = hook(types.GAME_START),
})

api.define("events.on_title_start", {
  description = [[
    Happens when the title screen's scene starts playing behind the menu, once
    its level is loaded and its items are set up. The handler takes no
    arguments. `on_game_start` does not fire for the title level.
  ]],
  params = { { name = "callback", type = "function" } },
  returns = LISTENER_ID,
  examples = {
    [[trx.events.on_title_start(function()
  trx.log.info("the menu is up")
end)]],
  },
  impl = hook(types.TITLE_START),
})

api.define("events.on_pickup", {
  description = "Happens just after Lara picks up an item.",
  params = {
    {
      name = "callback",
      type = "function",
      params = {
        {
          name = "item_num",
          type = "integer",
          description = "0-based index of the item that was picked up.",
        },
      },
    },
  },
  returns = LISTENER_ID,
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
  params = { { name = "callback", type = "function" } },
  returns = LISTENER_ID,
  impl = hook(types.BEFORE_CONTROL),
})

api.define("events.after_control", {
  description = "Happens on every logical game frame, after the main game logic runs. The handler "
    .. "takes no arguments.",
  params = { { name = "callback", type = "function" } },
  returns = LISTENER_ID,
  impl = hook(types.AFTER_CONTROL),
})

api.define("events.on_flip_effect", {
  description = "Claims a flip effect number and happens whenever a level runs it, whether from a "
    .. "floor trigger or an animation command. Place an ordinary flipeffect trigger in a level "
    .. "editor - pad, heavy, switch and antitrigger all work - pick an unused effect number, and "
    .. "handle it here from the level's script.\n\n"
    .. "A claimed number belongs to the script for the rest of the level: its stock engine effect "
    .. "does not run, even if the handler is later detached. Unclaimed numbers are unaffected.\n\n"
    .. "Unlike the other hooks, this happens at effect execution time, in the middle of a game "
    .. "frame.",
  params = {
    {
      name = "effect_num",
      type = "integer",
      description = "The flip effect number to claim, as the level editor numbers them. This is "
        .. "not the id space of `trx.rooms.flip_effect`, which takes `trx.catalog.flip_effects` "
        .. "names.",
    },
    {
      name = "callback",
      type = "function",
      params = {
        {
          name = "timer",
          type = "integer",
          description = "A floor trigger's timer field, free for the level to use as a parameter. "
            .. "0 for an animation command, which carries no timer.",
        },
        {
          name = "item_num",
          type = "integer",
          description = "0-based index of the item that ran the effect: Lara for a pad trigger, "
            .. "the activating object for a heavy trigger, the animating item for an animation "
            .. "command.",
        },
      },
    },
  },
  returns = LISTENER_ID,
  examples = {
    [[trx.events.on_flip_effect(62, function(timer, item_num)
  trx.log.info("flipeffect 62 ran with timer " .. timer)
end)]],
  },
  impl = function(effect_num, callback)
    return raw.attach(types.FLIP_EFFECT, callback, effect_num)
  end,
})

api.define("events.on_room_change", {
  description = "Happens when an item changes rooms during play, which a cutscene or the attract "
    .. "demo is not. `trx.rooms.Room:on_enter` and `:on_exit` are this same event, narrowed to "
    .. "one room.",
  params = {
    {
      name = "callback",
      type = "function",
      params = {
        {
          name = "item",
          type = "Item",
          description = "The `trx.items.Item` that changed rooms.",
        },
        {
          name = "old_room",
          type = "integer",
          description = "0-based number of the room it left, or -1 if it had none.",
        },
        {
          name = "new_room",
          type = "integer",
          description = "0-based number of the room it entered, or -1 if it left the world.",
        },
      },
    },
  },
  returns = LISTENER_ID,
  examples = {
    [[trx.events.on_room_change(function(item, old_room, new_room)
  trx.log.info(item.object_id .. " moved to room " .. new_room)
end)]],
  },
  impl = function(callback)
    return raw.attach(types.ROOM_CHANGE, function(item_num, old_room, new_room)
      callback(trx.items[item_num], old_room, new_room)
    end)
  end,
})

api.define("events.on_trigger", {
  description = "Happens every time a trigger is aimed at an item - a floor trigger in the level, "
    .. "the `/trigger` console command, or `item:trigger` from a script - of any kind, an "
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
      params = {
        {
          name = "item",
          type = "Item",
          description = "The `trx.items.Item` the trigger was aimed at.",
        },
        {
          name = "trigger",
          type = "table",
          description = "What the trigger carried: `type` (an `items.TriggerType`), `mask` (the "
            .. "code bits it set, `1` to `31`), `timer` (in seconds), and `one_shot`.",
        },
      },
    },
  },
  returns = LISTENER_ID,
  examples = {
    [[trx.events.on_trigger(function(item, trigger)
  if trigger.type == trx.items.TriggerType.ANTITRIGGER then
    trx.log.info(item.object_id .. " was antitriggered")
  end
end)]],
  },
  impl = function(callback)
    return raw.attach(
      types.TRIGGER,
      function(item_num, kind, mask, timer, one_shot)
        callback(trx.items[item_num], {
          type = kind,
          mask = mask,
          timer = timer,
          one_shot = one_shot,
        })
      end
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
        params = {
          {
            name = "item",
            type = "Item",
            description = "The `trx.items.Item` that became " .. verb .. ".",
          },
        },
      },
    },
    returns = LISTENER_ID,
    examples = examples,
    impl = function(callback)
      return raw.attach(event_type, function(item_num)
        callback(trx.items[item_num])
      end)
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
        params = {
          {
            name = "item",
            type = "Item",
            description = item_desc,
          },
        },
      },
    },
    returns = LISTENER_ID,
    examples = examples,
    impl = function(callback)
      return raw.attach(event_type, function(item_num)
        callback(trx.items[item_num])
      end)
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
    "The `trx.items.Item` that finished.",
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
      .. "cheat. A trigger also fires `on_activate`, which this does not.\n\n"
      .. "`trx.items.Item:on_enter_sim` is this same event, narrowed to one item.",
    "The `trx.items.Item` that started being simulated.",
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
    "The `trx.items.Item` that stopped being simulated.",
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
      .. "`on_enter_sim`; watch that one for a start of any cause.\n\n"
      .. "`trx.items.Item:on_activate` is this same event, narrowed to one item.",
    "The `trx.items.Item` that was activated.",
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
    "The `trx.items.Item` that was deactivated.",
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
    "The `trx.items.Item` being removed. Valid only for the duration of the handler.",
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
    "The `trx.items.Item` that entered the world.",
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
    "The `trx.items.Item` that left the world.",
    {
      [[trx.events.on_leave_world(function(item)
  trx.log.info(item.object_id .. " left the world")
end)]],
    }
  )
)

api.define("events.on_hit", {
  description = [[Happens when an item takes damage, Lara included. It is the raw damage that
fires, before the item's hit points are clamped, so a fatal blow reports the whole amount the
attacker dealt. A death that does not go through damage - a script writing `hit_points`, or
`destroy()` - does not report.

`trx.items.Item:on_hit` is this same event, narrowed to one item.]],
  params = {
    {
      name = "callback",
      type = "function",
      params = {
        {
          name = "item",
          type = "Item",
          description = "The `trx.items.Item` that took the damage.",
        },
        {
          name = "damage",
          type = "integer",
          description = "Hit points taken, before clamping to zero.",
        },
      },
    },
  },
  returns = LISTENER_ID,
  examples = {
    [[trx.events.on_hit(function(item, damage)
  trx.log.info(item.object_id .. " lost " .. damage .. " hit points")
end)]],
  },
  impl = function(callback)
    return raw.attach(types.HIT, function(item_num, damage)
      callback(trx.items[item_num], damage)
    end)
  end,
})

api.define("events.on_kill", {
  description = [[Happens when damage takes an item's hit points to zero, Lara included. It is the
same blow `on_hit` reports, which fires first. A death that does not go through damage - a script
writing `hit_points`, or `destroy()` - does not report.

Some bosses fall and get back up: Willard is knocked out, Natla plays dead before her second
stage, and the dragon lies still until Lara takes the dagger. Each stage brings their hit points
to zero, so they report once per stage rather than once per boss, and the dragon reports once
more for the dagger that ends it.

`trx.items.Item:on_kill` is this same event, narrowed to one item.]],
  params = {
    {
      name = "callback",
      type = "function",
      params = {
        {
          name = "item",
          type = "Item",
          description = "The `trx.items.Item` that was brought down.",
        },
      },
    },
  },
  returns = LISTENER_ID,
  examples = {
    [[trx.events.on_kill(function(item)
  trx.log.info(item.object_id .. " is down")
end)]],
  },
  impl = function(callback)
    return raw.attach(types.KILL, function(item_num)
      callback(trx.items[item_num])
    end)
  end,
})

api.define("events.detach", {
  description = "Removes a previously attached handler, which stops firing immediately.",
  params = {
    {
      name = "listener_id",
      type = "integer",
      description = "The id attach returned.",
    },
  },
  returns = {
    type = "boolean",
    description = "Whether a handler with that id was attached. `false` means it had already been "
      .. "detached, or the id was never handed out.",
  },
  examples = {
    [[local id = trx.events.before_control(function()
  -- handle control loop event
end)
trx.events.detach(id)]],
  },
  impl = raw.detach,
})
