local raw = trxc.path
local api = trx.api

api.module("path", {
  order = 34,
  title = "Paths",
  description = "Naming a place on disk. A path is a value rather than text, so joining one is "
    .. "`/` and the parts of it are read off it, and the places the engine keeps its own files "
    .. "are named here. `trx.json` takes one wherever it takes a file.",
})

local Path

local function make(text)
  local path = setmetatable({}, Path)
  rawset(path, "_raw", text)
  return path
end

local function raw_of(value)
  if type(value) == "string" then
    return value
  end
  if getmetatable(value) == Path then
    return rawget(value, "_raw")
  end
  return nil
end

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

Path = api.type("path.Path", {
  description = [[
    A filesystem path. Joining one with `/` appends a child segment, and its
    parts are available as properties.

    A path only points to a location. It does not say whether a file is
    present until `trx.path.Path.exists` checks it.
  ]],
  examples = {
    [[local kept = trx.path.config_dir / "mymod" / "state.json"
trx.log.info(tostring(kept))
if kept:exists() then
  trx.log.info(kept:read_text())
end]],
  },
  fields = {
    parent = {
      type = "path.Path",
      description = "The directory the path sits in.",
      get = function(self)
        return make(raw.parent(rawget(self, "_raw")))
      end,
    },
    name = {
      type = "string",
      description = "The final component of the path, with its extension.",
      get = function(self)
        return raw.name(rawget(self, "_raw"))
      end,
    },
    stem = {
      type = "string",
      description = "The final component of the path, without its extension.",
      get = function(self)
        return raw.stem(rawget(self, "_raw"))
      end,
    },
    suffix = {
      type = "string",
      description = "The extension at the end of the final component, leading `.` and all, "
        .. "or the empty string where there is none.",
      get = function(self)
        return raw.name(rawget(self, "_raw")):match("%.[^.]*$") or ""
      end,
    },
  },
  methods = {
    exists = {
      description = "Whether anything is at the path now.",
      returns = {
        type = "boolean",
        description = "Whether a file or directory is present.",
      },
      impl = function(self)
        return raw.exists(rawget(self, "_raw"))
      end,
    },
  },
  operators = {
    div = {
      description = 'Appends a child segment, as `config_dir / "mymod" / "state.json"`. '
        .. "An absolute path on the right replaces the left side.",
      impl = joined,
    },
    tostring = {
      description = "The path as the text the engine would open.",
      impl = function(self)
        return rawget(self, "_raw")
      end,
    },
    eq = {
      description = "Two paths are equal when their filesystem text is equal.",
      impl = function(a, b)
        return raw_of(a) == raw_of(b)
      end,
    },
    concat = {
      description = "A path joins text as itself, whichever side of the `..` it is on.",
      impl = function(a, b)
        return tostring(a) .. tostring(b)
      end,
    },
  },
})

api.define("path.new", {
  description = "Creates a path from text, which the engine opens as it stands. Every `%token%` "
    .. 'in the text is expanded first, so `"%config_dir%/mymod"` says the same thing as '
    .. '`trx.path.config_dir / "mymod"`.',
  params = {
    { name = "text", type = "string", description = "The path as text." },
  },
  returns = { type = "path.Path", description = "The path." },
  examples = {
    [[local kept = trx.path.new("%config_dir%/mymod/state.json")]],
  },
  impl = function(text)
    return make(raw.expand(text))
  end,
})

api.define("path.kinds", {
  description = "Every kind of file `trx.path.resolve` may be asked for.",
  returns = {
    type = "table",
    description = "The file kinds, as a list of strings.",
  },
  examples = {
    [[for _, kind in ipairs(trx.path.kinds()) do
  trx.log.info(kind)
end]],
  },
  impl = raw.kinds,
})

api.define("path.resolve", {
  description = [[
    Works out where the engine would find one of its own files, searching in
    the order it searches: a mod's own copy first, then the game the mod sits
    on, then the configuration directory. If no file is found, this returns
    `nil`.

    This is how a script reads a file the game ships without knowing which of
    those directories supplies it. `trx.path.kinds` lists what may be asked for.
  ]],
  params = {
    {
      name = "kind",
      type = "string",
      description = "Which kind of file, such as `common_config` or `level_file`. "
        .. "<!--noref: common_config--><!--noref: level_file-->",
    },
    {
      name = "name",
      type = "string",
      description = "The file to look for, such as `weapons.json5`. "
        .. "<!--noref: weapons.json5-->",
    },
  },
  returns = {
    type = "path.Path",
    nullable = true,
    description = "The file path, or `nil`.",
  },
  examples = {
    [[local weapons = trx.path.resolve("common_config", "weapons.json5")
if weapons ~= nil then
  trx.log.info("weapons come from " .. tostring(weapons))
end]],
  },
  impl = function(kind, name)
    local found = raw.resolve(kind, name)
    return found ~= nil and make(found) or nil
  end,
})

for _, name in ipairs(raw.roots()) do
  api.property("path." .. name, {
    type = "path.Path",
    nullable = true,
    description = ("The `%%%s%%` directory, or `nil` where the game keeps none."):format(
      name
    ),
    get = function()
      local dir = raw.root(name)
      return dir ~= nil and make(dir) or nil
    end,
  })
end
