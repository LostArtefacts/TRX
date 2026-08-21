local raw = trxc.mod
local api = trx.api

api.module("mod", {
  order = 29,
  description = "The mods the game was built with, and which one is loaded.",
})

api.enum("mod.Type", {
  backing = "SHELL_MOD_TYPE",
  description = "What kind of mod it is.",
  values = {
    BASE_GAME = "The base game.",
    EXPANSION_PACK = "An expansion pack.",
    MISC = "A miscellaneous mod.",
    DIRECT_LEVEL = "A single level loaded on its own.",
    CUSTOM = "A custom mod.",
  },
})

api.type("mod.Mod", {
  backing = "SHELL_MOD",
  description = "A mod the game can run. Everything on it is read-only.",

  fields = {
    name = {
      from = "name",
      type = "string",
      writable = false,
      description = "The mod's identifier, as `trx.mod.switch` takes it.",
    },
    title = {
      from = "title",
      type = "string",
      writable = false,
      description = "The mod's name, as shown to the player.",
    },
    type = {
      from = "mod_type",
      type = "mod.Type",
      writable = false,
      description = "What kind of mod it is.",
    },
    engine_version = {
      from = "engine_version",
      type = "integer",
      writable = false,
      description = "Which Tomb Raider the mod runs on.",
    },
    base_mod = {
      from = "base_mod",
      type = "string",
      writable = false,
      description = "The mod this one builds on, or `nil` if it stands alone.",
    },
    is_available = {
      from = "is_available",
      type = "boolean",
      writable = false,
      description = "Whether the mod's files are present.",
    },
    is_valid = {
      from = "is_valid",
      type = "boolean",
      writable = false,
      description = "Whether the mod can be loaded.",
    },
  },
})

api.property("mod.list", {
  type = "mod.Mod",
  list = true,
  description = "The mods the game was built with, counted from one.",
  get = function()
    local mods = {}
    for i = 1, raw.count() do
      mods[i] = raw.get(i - 1)
    end
    return mods
  end,
})

api.property("mod.current", {
  type = "mod.Mod",
  description = "The loaded mod.",
  get = raw.get_current,
})

api.define("mod.switch", {
  description = "Restarts the game into another mod. The switch happens once the game flow "
    .. "picks it up, not on the call.",
  params = {
    {
      name = "mod",
      type = "any",
      description = "A `trx.mod.Mod` or a mod name.",
    },
  },
  returns = {
    {
      type = "boolean",
      description = "Whether the mod can be switched to. `false` leaves the game where it is.",
    },
  },
  examples = { [[trx.mod.switch("arabian-nights")]] },
  impl = raw.switch,
})
