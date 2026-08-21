local raw = trxc.game
local api = trx.api

api.module("game", {
  order = 10,
  description = "Module for the game flow: which levels there are, and which one is being played.",
})

api.unit("game.Frames", {
  description = [[
    A length of time counted in the frames the engine runs the world at, which
    is what the engine measures its own timers in.
  ]],
  spellings = { "game frames", "in frames" },
})

api.unit("game.Seconds", {
  type = "number",
  description = "A length of time in seconds, as a player would read it off a clock.",
  spellings = { "in seconds" },
})

api.const("game.LOGIC_FPS", {
  value = raw.LOGIC_FPS,
  type = "integer",
  description = "How many logical frames the game runs a second, which is the rate "
    .. "`trx.events.before_control` fires at. A script that counts frames divides by this to reach "
    .. "seconds.",
})

api.number("game.LevelNum", {
  base = 1,
  description = "The number a level goes by, which is what the player is shown and what a "
    .. "gameflow names. Not its place in a table: a level the game flow skips does not "
    .. "count, and a gym level has no number at all and reads 0.",
})

local LevelTable = api.enum("game.LevelTable", {
  backing = "GF_LEVEL_TABLE_TYPE",
  description = "One of the lists of levels the game flow declares.",
  values = {
    TITLE = "The title screen.",
    MAIN = "The levels of the game proper.",
    CUTSCENES = "The cutscenes.",
    DEMOS = "The demos that play when the title screen is left alone.",
  },
})

local LevelType = api.enum("game.LevelType", {
  backing = "GF_LEVEL_TYPE",
  description = "What kind of level it is.",
  values = {
    TITLE = "The title screen.",
    NORMAL = "An ordinary level.",
    CUTSCENE = "A cutscene.",
    DEMO = "A demo.",
    GYM = "Lara's home, which has no level number.",
    BONUS = "A bonus level, played once the game is finished.",
    DUMMY = "Not a level. Kept only because old savegames refer to it.",
    CURRENT = "Not a level. Kept only because old savegames refer to it.",
  },
})

api.type("game.Level", {
  backing = "GF_LEVEL",
  description = "A level, as the game flow file declares it. Everything on it is read-only: a "
    .. "level is what the game flow says it is.",

  fields = {
    num = {
      from = "num",
      type = "game.LevelNum",
      writable = false,
    },
    key = {
      from = "key",
      type = "string",
      writable = false,
      description = [[What the level is called, taken from the name of the file it loads: `wall.tr2` reads
back as `wall`. Lower case, regardless of the case on disk, and `nil` for a level that loads no file
of its own. <!--noref: wall.tr2, wall-->

This is the name to write into a table of per-level data. `trx.game.Level.num` is a position and
moves as soon as a game flow gains a level, and `trx.game.Level.path` is wherever the file sits on
this install.]],
    },
    type = {
      from = "type",
      type = "game.LevelType",
      writable = false,
      description = "What kind of level it is.",
    },
    title = {
      from = "title",
      type = "string",
      writable = false,
      description = "The level's name, as shown to the player.",
    },
    path = {
      from = "path",
      type = "string",
      writable = false,
      description = "Path to the level file.",
    },
    script_path = {
      from = "script_path",
      type = "string",
      writable = false,
      description = "Path to the Lua script that runs when the level loads, or `nil` if it has none.",
    },
    lara_outfit = {
      from = "lara_outfit",
      type = "string",
      writable = false,
      description = "The outfit Lara starts the level in.",
    },
    music_track = {
      from = "music_track",
      type = "catalog.music",
      writable = false,
      description = "The track that plays when the level starts.",
    },
    water_particles = {
      from = "water_particles",
      type = "boolean",
      writable = false,
      description = "Whether water particles are visible in the level's water.",
    },
    unobtainable_pickups = {
      from = "unobtainable.pickups",
      type = "integer",
      writable = false,
      description = "Pickups the stats screen must not hold against the player, because they "
        .. "cannot be got.",
    },
    unobtainable_kills = {
      from = "unobtainable.kills",
      type = "integer",
      writable = false,
      description = "Kills the stats screen must not hold against the player.",
    },
    unobtainable_ally_kills = {
      from = "unobtainable.ally_kills",
      type = "integer",
      writable = false,
      description = "Ally kills the stats screen must not hold against the player.",
    },
    unobtainable_secrets = {
      from = "unobtainable.secrets",
      type = "integer",
      writable = false,
      description = "Secrets the stats screen must not hold against the player.",
    },
  },

  extensions = {
    inventory = {
      type = "inventory.Inventory",
      description = "What the level keeps for Lara's return, or "
        .. "`nil` for a level that keeps nothing: the title screen and the cutscenes. It is what "
        .. "she will arrive there with rather than what she is carrying now, which is "
        .. "`trx.inventory` itself.",
      impl = function(level)
        return trxc.inventory.get(level.num)
      end,
    },
    stats = {
      type = "stats.Stats",
      description = "What the level keeps count of, or `nil` for a level "
        .. "that counts nothing: the title screen and the cutscenes. The level being played is "
        .. "also `trx.stats` itself.",
      impl = function(level)
        return trxc.stats.get(level.num)
      end,
    },
  },
})

local function level_list(table_type)
  local levels = {}
  for i = 1, raw.count_levels(table_type) do
    levels[i] = raw.get_level(table_type, i)
  end
  return levels
end

api.property("game.levels", {
  type = "game.Level",
  list = true,
  description = "The levels of the game, in order, counted from one.",
  get = function()
    return level_list(LevelTable.MAIN)
  end,
})

api.property("game.cutscenes", {
  type = "game.Level",
  list = true,
  description = "The cutscene levels, counted from one. TR4's in-game cutscenes are a different "
    .. "thing, and live in `trx.cutscenes`.",
  get = function()
    return level_list(LevelTable.CUTSCENES)
  end,
})

api.property("game.demos", {
  type = "game.Level",
  list = true,
  description = "The demos, counted from one.",
  get = function()
    return level_list(LevelTable.DEMOS)
  end,
})

api.property("game.current_level", {
  type = "game.Level",
  description = "The level being played, or `nil` if none is.",
  get = raw.get_current_level,
})

api.property("game.gym", {
  type = "game.Level",
  description = "The gym level, or `nil` if this game has no gym.",
  get = function()
    local level = raw.get_level(LevelTable.MAIN, 0)
    if level ~= nil and level.type == LevelType.GYM then
      return level
    end
    return nil
  end,
})

api.property("game.version", {
  type = "integer",
  description = "Which Tomb Raider this build is: 1, 2, 3 or 4.",
  get = raw.get_version,
})

api.property("game.trx_version", {
  type = "string",
  description = "What this build reports as its version: `1.9.3` for a release, and the tag with "
    .. "the commits since it for a development build.",
  get = raw.get_trx_version,
})

api.property("game.is_loaded", {
  type = "boolean",
  description = "Whether a level is loaded.",
  get = raw.is_loaded,
})

api.property("game.is_playable", {
  type = "boolean",
  description = "Whether the game is loaded and taking input - not in a menu, and not in a cutscene.",
  get = raw.is_playable,
})

api.property("game.is_ngplus", {
  type = "boolean",
  description = "Whether this is a new game plus run, which is what the passport's bonus start "
    .. "sets. Lara keeps her weapons between levels and her ammunition does not run down.",
  get = raw.is_ngplus,
})

api.number("game.DemoNum", {
  base = 1,
  description = "Where a demo sits in the table of demos.",
})

api.define("game.play_level", {
  description = "Starts a level from `trx.game.levels`.",
  params = {
    {
      name = "level_num",
      type = "game.LevelNum",
    },
    {
      name = "opts",
      type = "table",
      optional = true,
      description = "How to start it.",
      fields = {
        {
          name = "select",
          type = "boolean",
          optional = true,
          description = "Start the level as the level-select screen does, rebuilding Lara's "
            .. "inventory to what she would carry on reaching it. Without it the level "
            .. "continues from the one in progress.",
        },
      },
    },
  },
  examples = { [[trx.game.play_level(1)]] },
  impl = raw.play_level,
})

api.define("game.play_cutscene", {
  description = "Plays a cutscene.",
  params = {
    {
      name = "cutscene_num",
      type = "cutscenes.Num",
    },
  },
  impl = raw.play_cutscene,
})

api.define("game.play_demo", {
  description = "Plays a demo, and returns the one that started.",
  params = {
    {
      name = "demo_num",
      type = "game.DemoNum",
      optional = true,
      description = "Omit to play the next demo in rotation.",
    },
  },
  returns = {
    {
      type = "game.Level",
      nullable = true,
      description = "The demo that started, or `nil` if the game has no demos.",
    },
  },
  impl = raw.play_demo,
})

api.define("game.play_gym", {
  description = "Starts the gym. Raises if this game has no gym.",
  impl = raw.play_gym,
})

api.define("game.end_level", {
  description = "Ends the current level, as though Lara had reached its exit.",
  impl = raw.end_level,
})

api.define("game.exit_to_title", {
  description = "Leaves the current game and returns to the title screen.",
  impl = raw.exit_to_title,
})

api.define("game.exit_game", {
  description = "Closes the game.",
  impl = raw.exit_game,
})

api.define("game.screenshot", {
  description = "Takes a screenshot. Without a path, writes one to the screenshots folder in the "
    .. "player's configured format; with a path, writes to that file.",
  params = {
    {
      name = "path",
      type = "string",
      optional = true,
      description = "File to write to.",
    },
  },
  impl = raw.screenshot,
})
