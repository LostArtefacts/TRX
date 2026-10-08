require("trx.signal")

local raw = trxc.game
local raw_inventory = trxc.inventory
local raw_stats = trxc.stats
local hooks = trxc.hooks
local h = require("trx.internal.helpers")

---@class (partial) trx
---@field game trx.game

---Module for the game flow: which levels there are, and which one is being
---played.
---@trx.module 11
---@class (exact) trx.game
---@trx.readonly current_level, cutscene_frame, cutscenes, demos, fmvs, gym,
---  is_loaded, is_ngplus, is_photo_mode, is_playable, is_playing,
---  is_suspended, levels, measured_fps, photo_mode_target, real_time,
---  tr_version, version
---@field levels trx.game.Level[] The levels of the game, in order, counted from one.
---@field cutscenes trx.game.Level[] The cutscene levels, counted from one. TR4's in-game cutscenes are a different thing, and live in `trx.cutscenes`.
---@field demos trx.game.Level[] The demos, counted from one.
---@field fmvs trx.game.FMV[] The movies the game flow declares, counted from one.
---@field current_level trx.game.Level? The level being played, or `nil` if none is.
---@field gym trx.game.Level? The gym level, or `nil` if this game has no gym.
---@field version integer Which Tomb Raider this build is: 1, 2, 3 or 4.
---@field is_loaded boolean Whether a level is loaded.
---@field measured_fps integer How many frames reached the screen in the last second, counted against the wall clock. Frames are drawn more often than the game ticks, so this is not the rate the game runs at.
---@field is_playing boolean Whether a level is running: Lara and the creatures move and the game answers to the player. It goes false while the inventory ring, the pause screen or photo mode holds the level still, and outside a level altogether.
---@field is_playable boolean Whether the game is loaded and taking input - not in a menu, and not in a cutscene.
---@field cutscene_frame integer? Which frame of the cutscene level on screen is being played, or `nil` unless one is. The scene's actors are items animating against this clock, so it is what a script names a moment of the scene by, rather than an actor's own animation frame.
---
---  This is the cutscene a level plays as a level of its own, as TR1 to TR3
---  do. `trx.cutscenes.Cutscene.frame_num` reports the TR4 kind.
---@field real_time number Seconds of wall-clock time since the game started, which keeps running while the game is held still. Use it to time something against the player's clock rather than against the frames the game has run.
---@field tr_version integer Which Tomb Raider the level being played belongs to: `1` through `4`. The games differ in what they draw and in what the player expects, so a script that dresses more than one reads this to tell them apart. Zero before a level is loaded.
---@field is_suspended boolean Whether a loaded level is held still: the pause screen, photo mode, or the inventory ring. Lara and the creatures do not move while it is true. It is false outside a level, which is what tells it apart from the opposite of `trx.game.is_playing`.
---@field is_photo_mode boolean Whether the player is in photo mode, where the camera is theirs to move and the game is held still.
---@field photo_mode_target trx.game.PhotoModeTarget What photo mode is steering. Outside photo mode, this is always the camera, which is where every session starts.
---@field is_ngplus boolean Whether this is a new game plus run, which is what the passport's bonus start sets. Lara keeps her weapons between levels and her ammunition does not run down.
local M = h.module("game")

---A length of time counted in the frames the engine runs the world at, which
---is what the engine measures its own timers in.
---@trx.unit game frames, in frames
---@alias trx.game.Frames integer

---A length of time in seconds, as a player would read it off a clock.
---@trx.unit in seconds
---@alias trx.game.Seconds number

-- The real version names this checkout, so the reference prints a sample
-- instead.

---What this build reports as its version: `1.9.3` for a release, and the tag
---with the commits since then for a development build.
---@type string
---@trx.sample "TRX 1.9.3-42-g0f4c2a1"
M.TRX_VERSION = h.const("game.TRX_VERSION", raw.TRX_VERSION)

---How many logical frames the game runs a second, which is the rate
---`trx.events.before_control` fires at. A script that counts frames divides by
---this to reach seconds.
---@type integer
---@trx.value 30
M.LOGIC_FPS = h.const("game.LOGIC_FPS", raw.LOGIC_FPS)

---The number a level goes by, which is what the player is shown and what a
---gameflow names. Not its place in a table: a level the game flow skips does
---not count, and a gym level has no number at all and reads 0.
---@trx.base 1
---@alias trx.game.LevelNum integer

---One of the lists of levels the game flow declares.
---@enum trx.game.LevelTable
local LevelTable = {
  ---The title screen.
  TITLE = h.IntegerConstant,
  ---The levels of the game proper.
  MAIN = h.IntegerConstant,
  ---The cutscenes.
  CUTSCENES = h.IntegerConstant,
  ---The demos that play when the title screen is left alone.
  DEMOS = h.IntegerConstant,
}
M.LevelTable = h.enum("game.LevelTable", "GF_LEVEL_TABLE_TYPE", LevelTable)

---What kind of level it is.
---@enum trx.game.LevelType
local LevelType = {
  ---The title screen.
  TITLE = h.IntegerConstant,
  ---An ordinary level.
  NORMAL = h.IntegerConstant,
  ---A cutscene.
  CUTSCENE = h.IntegerConstant,
  ---A demo.
  DEMO = h.IntegerConstant,
  ---Lara's home, which has no level number.
  GYM = h.IntegerConstant,
  ---A bonus level, played once the game is finished.
  BONUS = h.IntegerConstant,
  ---Not a level. Kept only because old savegames refer to it.
  DUMMY = h.IntegerConstant,
  ---Not a level. Kept only because old savegames refer to it.
  CURRENT = h.IntegerConstant,
}
M.LevelType = h.enum("game.LevelType", "GF_LEVEL_TYPE", LevelType)

---A level, as the game flow file declares it. Everything on it is read-only:
---a level is what the game flow says it is.
---@class (exact) trx.game.Level
---@trx.readonly key, lara_outfit, music_track, num, path, script_path, title,
---  type, unobtainable_ally_kills, unobtainable_kills, unobtainable_pickups,
---  unobtainable_secrets, water_particles
---@field num trx.game.LevelNum
---@field key string? What the level is called, taken from the name of the file it loads: `wall.tr2` reads back as `wall`. Lower case, regardless of the case on disk, and `nil` for a level that loads no file of its own. <!--noref: wall.tr2, wall-->
---
---  This is the name to write into a table of per-level data.
---  `trx.game.Level.num` is a position and moves as soon as a game flow gains
---  a level, and `trx.game.Level.path` is wherever the file sits on this
---  install.
---@field type trx.game.LevelType What kind of level it is.
---@field title string The level's name, as shown to the player.
---@field path string Path to the level file.
---@field script_path string? Path to the Lua script that runs when the level loads, or `nil` if it has none.
---@field lara_outfit string The outfit Lara starts the level in.
---@field music_track trx.catalog.music The track that plays when the level starts.
---@field water_particles boolean Whether water particles are visible in the level's water.
---@field unobtainable_pickups integer Pickups the stats screen must not hold against the player, because they cannot be got.
---@field unobtainable_kills integer Kills the stats screen must not hold against the player.
---@field unobtainable_ally_kills integer Ally kills the stats screen must not hold against the player.
---@field unobtainable_secrets integer Secrets the stats screen must not hold against the player.
---@field inventory trx.inventory.Inventory? What the level keeps for Lara's return, or `nil` for a level that keeps nothing: the title screen and the cutscenes. It is what she will arrive there with rather than what she is carrying now, which is `trx.inventory` itself.
---@field stats trx.stats.Stats? What the level keeps count of, or `nil` for a level that counts nothing: the title screen and the cutscenes. The level being played is also `trx.stats` itself.
local Level = h.handle("game.Level", "GF_LEVEL", {
  fields = {
    num = "num",
    key = "key",
    type = "type",
    title = "title",
    path = "path",
    script_path = "script_path",
    lara_outfit = "lara_outfit",
    music_track = "music_track",
    water_particles = "water_particles",
    unobtainable_pickups = "unobtainable.pickups",
    unobtainable_kills = "unobtainable.kills",
    unobtainable_ally_kills = "unobtainable.ally_kills",
    unobtainable_secrets = "unobtainable.secrets",
  },
  extensions = {
    inventory = function(level)
      return raw_inventory.get(level.num)
    end,
    stats = function(level)
      return raw_stats.get(level.num)
    end,
  },
})

local function level_list(table_type)
  local levels = {}
  for i = 1, raw.count_levels(table_type) do
    levels[i] = raw.get_level(table_type, i)
  end
  return levels
end

---The number an FMV goes by, which is its place in the list the game flow
---declares.
---@trx.base 1
---@alias trx.game.FMVNum integer

---A movie, as the game flow file declares it. Everything on it is read-only.
---@class (exact) trx.game.FMV
---@trx.readonly is_credit, is_intro, is_legal, num, path
---@field num trx.game.FMVNum
---@field path string Path to the movie file.
---@field is_legal boolean Whether the movie is a legal notice, which the Legal screen setting hides.
---@field is_credit boolean Whether the movie is part of the credits, which the Credits setting hides.
---@field is_intro boolean Whether the movie opens the game.
local FMV = h.handle("game.FMV", "GF_FMV", {
  fields = {
    num = "num",
    path = "path",
    is_legal = "is_legal",
    is_credit = "is_credit",
    is_intro = "is_intro",
  },
})

---The signals the game's own state speaks through, for a script that would
---rather hear about a change than ask after one. Each is read once a frame, so
---what listens runs on a change rather than on a frame.
---@class (exact) trx.game.signals
---@trx.readonly is_photo_mode, is_playable, is_playing, is_suspended
---@field is_playing trx.signal.Signal Says when a level starts being played, and when it stops.
---@field is_suspended trx.signal.Signal Says when the game is held still, and when it runs on again.
---@field is_photo_mode trx.signal.Signal Says when photo mode opens and closes.
---@field is_playable trx.signal.Signal Says when the kind of level running changes.
M.signals = h.namespace("game.signals")

local GAME_SIGNALS = {
  {
    "is_playing",
    function()
      return trx.game.is_playing
    end,
  },
  {
    "is_suspended",
    function()
      return trx.game.is_suspended
    end,
  },
  {
    "is_photo_mode",
    function()
      return trx.game.is_photo_mode
    end,
  },
  {
    "is_playable",
    function()
      return trx.game.is_playable
    end,
  },
}

local signal_props = {}
for _, entry in ipairs(GAME_SIGNALS) do
  local name, read = entry[1], entry[2]
  local held = nil
  signal_props[name] = {
    get = function()
      if held == nil then
        held = trx.signal.polled(read)
      end
      return held
    end,
  }
end
h.properties(M.signals, "game.signals", signal_props)

---What the player's movement keys steer while photo mode is open.
---@enum trx.game.PhotoModeTarget
local PhotoModeTarget = {
  ---The camera.
  CAMERA = h.IntegerConstant,
  ---Lara herself.
  LARA = h.IntegerConstant,
}
M.PhotoModeTarget =
  h.enum("game.PhotoModeTarget", "PHOTO_MODE", PhotoModeTarget)

h.properties(M, "game", {
  levels = {
    get = function()
      return level_list(M.LevelTable.MAIN)
    end,
  },
  cutscenes = {
    get = function()
      return level_list(M.LevelTable.CUTSCENES)
    end,
  },
  demos = {
    get = function()
      return level_list(M.LevelTable.DEMOS)
    end,
  },
  fmvs = {
    get = function()
      local result = {}
      for i = 1, raw.count_fmvs() do
        result[i] = raw.get_fmv(i)
      end
      return result
    end,
  },
  current_level = { get = raw.get_current_level },
  gym = {
    get = function()
      local level = raw.get_level(M.LevelTable.MAIN, 0)
      if level ~= nil and level.type == M.LevelType.GYM then
        return level
      end
      return nil
    end,
  },
  version = { get = raw.get_version },
  is_loaded = { get = raw.is_loaded },
  measured_fps = { get = raw.measured_fps },
  is_playing = { get = raw.is_playing },
  is_playable = { get = raw.is_playable },
  cutscene_frame = { get = raw.cutscene_frame },
  real_time = { get = raw.real_time },
  tr_version = { get = raw.tr_version },
  is_suspended = { get = raw.is_suspended },
  is_photo_mode = { get = raw.is_photo_mode },
  photo_mode_target = { get = raw.photo_mode_target },
  is_ngplus = { get = raw.is_ngplus },
})

---Where a demo sits in the table of demos.
---@trx.base 1
---@alias trx.game.DemoNum integer

---@class (exact) trx.game.play_level.opts
---@field select? boolean Start the level as the level-select screen does, rebuilding Lara's inventory to what she would carry on reaching it. Without it the level continues from the one in progress.
---@field ng_plus? boolean Whether to start the bonus game mode.
---@field from_save? table The save to take Lara's progress from, as `{ slot_num = 1, pool = trx.savegame.Pool.NORMAL }`. Raises without `select`. The death counter and the restart file then use this save. Without it, `select` builds Lara's inventory as if the game had been played from the first level. <!--noref: select-->

---Starts a level from `trx.game.levels`.
---
---```lua
---trx.game.play_level(1)
---```
---@param level_num trx.game.LevelNum
---@param opts? trx.game.play_level.opts How to start it.
---@type fun(level_num: trx.game.LevelNum, opts?: trx.game.play_level.opts)
M.play_level = raw.play_level

---Plays a cutscene.
---@param cutscene_num trx.cutscenes.Num
---@type fun(cutscene_num: trx.cutscenes.Num)
M.play_cutscene = raw.play_cutscene

---Plays a demo, and returns the one that started.
---@param demo_num? trx.game.DemoNum Omit to play the next demo in rotation.
---@return trx.game.Level? # The demo that started, or `nil` if the game has no demos.
---@type fun(demo_num?: trx.game.DemoNum): trx.game.Level?
M.play_demo = raw.play_demo

---Plays a movie, and returns once it has finished. The game resumes where it
---left off.
---
---```lua
---trx.game.play_fmv(1)
---```
---@param fmv_num trx.game.FMVNum
---@type fun(fmv_num: trx.game.FMVNum)
M.play_fmv = raw.play_fmv

---Starts the gym. Raises if this game has no gym.
---@type fun()
M.play_gym = raw.play_gym

---Ends the current level, as though Lara had reached its exit.
---@type fun()
M.end_level = raw.end_level

---@class (exact) trx.game.start_new_game.settings
---@field ng_plus? boolean Whether to start the bonus game mode. `false` by default.

---Starts a new game at the first level.
---
---The new game uses the selected game mode and clears previous progress.
---
---```lua
---trx.game.start_new_game({ ng_plus = true })
---```
---@param settings? trx.game.start_new_game.settings The new game settings.
function M.start_new_game(settings)
  settings = settings or {}
  raw.start_new_game(settings.ng_plus)
end

---Starts the level being played again from its beginning.
---
---Raises outside a level, and in a cutscene or a demo.
---@type fun()
M.restart_level = raw.restart_level

---Whether the current level can be restarted. It cannot outside a level, in a
---cutscene or a demo, or where the save the game runs from does not allow a
---restart.
---@return boolean # Whether the level can be restarted.
function M.can_restart_level()
  return raw.is_restartable_level() and trx.savegame.restart_available()
end

---Leaves the current game and returns to the title screen.
---@type fun()
M.exit_to_title = raw.exit_to_title

---Closes the game.
---@type fun()
M.exit_game = raw.exit_game

---Decides whether a bonus level opens as the level before it ends. Without a
---check, a bonus level opens once every secret of the main levels is found,
---and the game returns to the title screen otherwise.
---
---The check is asked before the bonus level starts to load, so a level that
---stays shut shows nothing of itself, not even its loading screen. It is not
---asked about a level started from level select or the console. Set it from
---the game script: it stays set until it is replaced or cleared, whichever
---level is being played.
---
---```lua
----- the fourth bonus level opens once the three before it are cleared out
---trx.game.set_bonus_check(function(level, unlocked)
---  if level.num ~= 12 then
---    return unlocked
---  end
---  for i = 9, 11 do
---    local secrets = trx.game.levels[i].stats.secrets
---    if secrets.count < secrets.max then
---      return false
---    end
---  end
---  return true
---end)
---```
---@param check? fun(level: trx.game.Level, unlocked: boolean): boolean? The function to ask. It is handed the bonus level and the engine's own answer, and returns whether the level opens; returning nothing leaves the engine's answer. Omit it to go back to the engine's own answer.
---@trx.arg check.level The bonus level about to start.
---@trx.arg check.unlocked Whether every secret of the main levels has been found, which is what the engine would decide on its own.
function M.set_bonus_check(check)
  if check ~= nil and type(check) ~= "function" then
    error("the bonus check must be a function", 2)
  end
  hooks.set("bonus_check", check)
end

---Takes a screenshot. Without a path, writes one to the screenshots folder in
---the player's configured format; with a path, writes to that file.
---@param path? string File to write to.
---@type fun(path?: string)
M.screenshot = raw.screenshot

local _ = { Level, FMV }
