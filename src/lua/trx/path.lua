local raw = trxc.path
local h = require("trx.internal.helpers")

---@class (partial) trx
---@field path trx.path

---Filesystem paths for Lua scripts. A path is a value rather than text, so
---joining one uses `/` and its parts are properties. Scripts can read and write
---under the game's own directories, and nowhere else.
---@trx.module 35 Paths
---@class (exact) trx.path
---@trx.readonly cache_dir, config_dir, games_dir, legacy_saves_dir, saves_dir,
---  screenshots_dir, trx_dir
---@field trx_dir trx.path.Path? The `%trx_dir%` directory, or `nil` where the game keeps none.
---@field config_dir trx.path.Path? The `%config_dir%` directory, or `nil` where the game keeps none.
---@field cache_dir trx.path.Path? The `%cache_dir%` directory, or `nil` where the game keeps none.
---@field games_dir trx.path.Path? The `%games_dir%` directory, or `nil` where the game keeps none.
---@field screenshots_dir trx.path.Path? The `%screenshots_dir%` directory, or `nil` where the game keeps none.
---@field saves_dir trx.path.Path? The `%saves_dir%` directory, or `nil` where the game keeps none.
---@field legacy_saves_dir trx.path.Path? The `%legacy_saves_dir%` directory, or `nil` where the game keeps none.
local M = h.module("path")

-- Both are defined below the class, which they build and recognise.
local make, raw_of

local function joined(a, b)
  local left, right = raw_of(a), raw_of(b)
  if left == nil or right == nil then
    error("trx.path: a path joins a path or text, and nothing else", 2)
  end
  if right == "" then
    return make(left)
  end
  if left == "" or right:sub(1, 1) == "/" or right:sub(1, 1) == "\\" then
    return make(right)
  end
  return make((left:gsub("[/\\]+$", "")) .. "/" .. right)
end

---A filesystem path. Joining one with `/` appends a child segment, and its
---parts are available as properties.
---
---A path only points to a location. It does not say whether a file is present
---until `trx.path.Path.exists` checks it.
---
---```lua
---local kept = trx.path.config_dir / "mymod" / "state.json"
---trx.log.info(tostring(kept))
---if kept:exists() then
---  trx.log.info(kept:read_text())
---end
---```
---@class (exact) trx.path.Path
---@trx.readonly name, parent, stem, suffix
---@operator div(trx.path.Path|string): trx.path.Path
---@operator concat(string): string
---@trx.operator div Appends a child segment, as `config_dir / "mymod" / "state.json"`. An absolute path on the right replaces the left side.
---@trx.operator tostring The path as the text the engine would open.
---@trx.operator eq Two paths are equal when their filesystem text is equal.
---@trx.operator concat A path joins text as itself, whichever side of the `..` it is on.
---@field parent trx.path.Path The directory the path sits in.
---@field name string The final component of the path, with its extension.
---@field stem string The final component of the path, without its extension.
---@field suffix string The extension at the end of the final component, leading `.` and all, or the empty string where there is none.
local Path = h.class("path.Path", {
  fields = {
    parent = {
      get = function(self)
        return make(raw.parent(rawget(self, "_raw")))
      end,
    },
    name = {
      get = function(self)
        return raw.name(rawget(self, "_raw"))
      end,
    },
    stem = {
      get = function(self)
        return raw.stem(rawget(self, "_raw"))
      end,
    },
    suffix = {
      get = function(self)
        return raw.name(rawget(self, "_raw")):match("%.[^.]*$") or ""
      end,
    },
  },
  operators = {
    div = joined,
    tostring = function(self)
      return rawget(self, "_raw")
    end,
    eq = function(a, b)
      return raw_of(a) == raw_of(b)
    end,
    concat = function(a, b)
      return tostring(a) .. tostring(b)
    end,
  },
})

function make(text)
  local path = h.new(Path)
  rawset(path, "_raw", text)
  return path
end

function raw_of(value)
  if type(value) == "string" then
    return value
  end
  if getmetatable(value) == Path then
    return rawget(value, "_raw")
  end
  return nil
end

---Reads the file as text, or returns `nil` where no file is present. Raises
---where the path is outside the directories a script may reach.
---@return string? # The text, or `nil` for a file that is not there.
function Path:read_text()
  return raw.read_text(rawget(self, "_raw"))
end

---Writes text into the file, making the directories it sits in and writing
---over an existing file. Raises where the path is outside the directories a
---script may reach.
---@param text string What to write.
function Path:write_text(text)
  raw.write_text(rawget(self, "_raw"), text)
end

---Whether a script may read or write there. Scripts reach the game's own
---directories and nothing else, so the rest of the player's disk is closed to
---them.
---@return boolean # Whether reading and writing are allowed.
function Path:is_reachable()
  return raw.is_reachable(rawget(self, "_raw"))
end

---Whether anything is at the path now. Raises where the path is outside the
---directories a script may reach.
---@return boolean # Whether a file or directory is present.
function Path:exists()
  return raw.exists(rawget(self, "_raw"))
end

---Creates a path from text, which the engine opens as it stands. Every
---`%token%` in the text is expanded first, so `"%config_dir%/mymod"` says the
---same thing as `trx.path.config_dir / "mymod"`.
---
---```lua
---local kept = trx.path.new("%config_dir%/mymod/state.json")
---```
---@param text string The path as text.
---@return trx.path.Path # The path.
function M.new(text)
  return make(raw.expand(text))
end

---Every kind of file `trx.path.resolve` may be asked for.
---
---```lua
---for _, kind in ipairs(trx.path.kinds()) do
---  trx.log.info(kind)
---end
---```
---@return table # The file kinds, as a list of strings.
---@type fun(): table
M.kinds = raw.kinds

---Works out where the engine would find one of its own files, searching in the
---order it searches: a mod's own copy first, then the game the mod sits on,
---then the configuration directory. If no file is found, this returns `nil`.
---
---This is how a script reads a file the game ships without knowing which of
---those directories supplies it. `trx.path.kinds` lists what may be asked for.
---
---```lua
---local weapons = trx.path.resolve("common_config", "weapons.json5")
---if weapons ~= nil then
---  trx.log.info("weapons come from " .. tostring(weapons))
---end
---```
---@param kind string Which kind of file, such as `common_config` or `level_file`. <!--noref: common_config--><!--noref: level_file-->
---@param name string The file to look for, such as `weapons.json5`. <!--noref: weapons.json5-->
---@return trx.path.Path? # The file path, or `nil`.
function M.resolve(kind, name)
  local found = raw.resolve(kind, name)
  return found ~= nil and make(found) or nil
end

local roots = {}
for _, name in ipairs(raw.roots()) do
  roots[name] = {
    get = function()
      local dir = raw.root(name)
      return dir ~= nil and make(dir) or nil
    end,
  }
end
h.properties(M, "path", roots)
