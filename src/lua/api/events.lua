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
  order = 2,
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

-- The five level-lifecycle events share a signature; only the moment differs.
local function level_hook(event_type, description, examples)
  return {
    description = description,
    params = {
      {
        name = "callback",
        type = "function",
        params = {
          {
            name = "level_num",
            type = "integer",
            description = "Number of the level the event fired for.",
          },
        },
      },
    },
    returns = LISTENER_ID,
    examples = examples,
    impl = hook(event_type),
  }
end

api.define(
  "events.before_level_file",
  level_hook(
    types.BEFORE_LEVEL_FILE,
    "Happens prior to loading the level file.",
    {
      [[trx.events.before_level_file(function(level_num)
  -- handle pre-file-load setup
end)]],
    }
  )
)

api.define(
  "events.after_level_file",
  level_hook(
    types.AFTER_LEVEL_FILE,
    "Happens after the level finishes loading, prior to loading information from a savegame."
  )
)

api.define(
  "events.before_item_setup",
  level_hook(
    types.BEFORE_ITEM_SETUP,
    "Happens after level items exist, before they are initialized. Use this to set object or "
      .. "item properties that item initialization reads."
  )
)

api.define(
  "events.after_item_setup",
  level_hook(
    types.AFTER_ITEM_SETUP,
    "Happens after level items exist, after they are initialized."
  )
)

api.define(
  "events.after_level_state",
  level_hook(
    types.AFTER_LEVEL_STATE,
    "Happens after the level finishes loading, after loading information from a savegame. If the "
      .. "game is started normally, this duplicates `after_level_file`.",
    {
      [[trx.events.after_level_state(function(level_num)
  -- handle post-savegame state restore
end)]],
    }
  )
)

api.define("events.on_game_start", {
  description = "Happens after the level finishes loading and the game is about to start. Unlike "
    .. "`after_level_file` and `after_level_state`, this waits for the fade-to-black / cross-fade "
    .. "effects to finish, so it is the place to play sound effects and run game logic.",
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
          description = "1-based index of the item that was picked up.",
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
