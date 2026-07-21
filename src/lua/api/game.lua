local raw = trxc.game
local api = trx.api

api.module("game", {
  order = 11,
  description = "Module for the game flow: which levels there are, and which one is being played.",
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
      type = "integer",
      writable = false,
      description = "The number the level goes by. Not its place in the table: levels the game "
        .. "flow skips do not count, and a gym level has no number at all and reads 0.",
    },
    type = {
      from = "type",
      type = "integer",
      writable = false,
      enum = "game.LevelType",
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
      type = "integer",
      writable = false,
      enum = "catalog.music",
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
})

local function level_list(table_type)
  local levels = {}
  for i = 1, raw.count_levels(table_type) do
    levels[i] = raw.get_level(table_type, i)
  end
  return levels
end

api.property("game.levels", {
  type = "table",
  description = "The levels of the game, in order, as a list of `trx.game.Level` counted from one.",
  get = function()
    return level_list(LevelTable.MAIN)
  end,
})

api.property("game.cutscenes", {
  type = "table",
  description = "The cutscenes, as a list of `trx.game.Level` counted from one.",
  get = function()
    return level_list(LevelTable.CUTSCENES)
  end,
})

api.property("game.demos", {
  type = "table",
  description = "The demos, as a list of `trx.game.Level` counted from one.",
  get = function()
    return level_list(LevelTable.DEMOS)
  end,
})

api.property("game.current_level", {
  type = "Level",
  description = "The level being played, or `nil` if none is.",
  get = raw.get_current_level,
})

api.property("game.gym", {
  type = "Level",
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
  description = "The TRX version string.",
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

api.define("game.play_level", {
  description = "Starts a level from `trx.game.levels`.",
  params = {
    {
      name = "num",
      type = "integer",
      description = "1-based position in `trx.game.levels`.",
    },
    {
      name = "opts",
      type = "table",
      optional = true,
      description = "`select`: start the level as the level-select screen does, rebuilding "
        .. "Lara's inventory to what she would carry on reaching it. Without it the level "
        .. "continues from the one in progress.",
    },
  },
  examples = { [[trx.game.play_level(1)]] },
  impl = raw.play_level,
})

api.define("game.play_cutscene", {
  description = "Plays a cutscene.",
  params = {
    {
      name = "num",
      type = "integer",
      description = "1-based position in `trx.game.cutscenes`.",
    },
  },
  impl = raw.play_cutscene,
})

api.define("game.play_demo", {
  description = "Plays a demo, and returns the one that started.",
  params = {
    {
      name = "num",
      type = "integer",
      optional = true,
      description = "1-based position in `trx.game.demos`. Omit to play the next demo in rotation.",
    },
  },
  returns = {
    {
      type = "Level",
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
