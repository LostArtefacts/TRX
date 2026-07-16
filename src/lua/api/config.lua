local raw = trxc.config
local api = trx.api

api.module("config", {
  order = 8,
  description = "Module for reading and changing engine settings.\n\n"
    .. "These are the player's settings, not the level's. `set` writes to them and keeps the "
    .. "change: it is remembered across saves and relaunches, exactly as if the player had made "
    .. "it themselves. A level that wants to tint the water or pull the fog in wants `override` "
    .. "instead, which lasts as long as the script keeps it and leaves the player's own value "
    .. "untouched underneath.",
})

api.define("config.get", {
  description = "Reads a setting. The value comes back as the type the option is declared with, so "
    .. "a boolean option reads as a boolean. Colors and enums read as strings.",
  params = {
    {
      name = "key",
      type = "string",
      description = "Dotted path, e.g. `visuals.water_color`.",
    },
  },
  returns = { type = "any", description = "Raises if no option has that key." },
  examples = {
    [[if trx.config.get("audio.enable_music") then trx.music.play(1) end]],
  },
  impl = raw.get,
})

api.define("config.set", {
  description = "Changes the player's setting, and keeps the change. Raises if the key is unknown "
    .. "or the value will not parse.\n\n"
    .. "The old value is not kept anywhere: the new one becomes the active setting as if the "
    .. "player had chosen it, and is remembered across saves and relaunches. Prefer `override` "
    .. "for anything a level wants only while it is running.",
  params = {
    { name = "key", type = "string", description = "Dotted path." },
    {
      name = "value",
      type = "any",
      description = "A boolean, a number, or a string, matching the option's type. A color is a "
        .. "6-digit hex string.",
    },
  },
  impl = raw.set,
})

api.define("config.override", {
  description = "Changes a setting for as long as the script keeps the override, without touching "
    .. "the player's own value.\n\n"
    .. "The player's value sits underneath and comes back on `restore`. Nothing is written to "
    .. "disk. Overrides stack, so one can be pushed over another; each `restore` lifts one off. "
    .. "A setting the game flow enforces cannot be overridden.",
  params = {
    { name = "key", type = "string", description = "Dotted path." },
    { name = "value", type = "any", description = "As for `set`." },
  },
  examples = {
    [[trx.config.override("visuals.water_color", "0080ff")
-- ... and when the level is done with it:
trx.config.restore("visuals.water_color")]],
  },
  impl = raw.override,
})

api.define("config.restore", {
  description = "Lifts one override off a setting, putting back whatever was underneath it.",
  params = {
    { name = "key", type = "string", description = "Dotted path." },
  },
  returns = {
    type = "boolean",
    description = "`false` if the setting was not overridden.",
  },
  impl = raw.restore,
})

api.define("config.is_overridden", {
  description = "Whether a script or the game flow is currently holding this setting away from the "
    .. "player's own value.",
  params = {
    { name = "key", type = "string", description = "Dotted path." },
  },
  returns = { type = "boolean" },
  impl = raw.is_overridden,
})

api.define("config.list", {
  description = "Every setting and its current value.",
  returns = {
    type = "table",
    description = "Maps each option's key to its value.",
  },
  examples = {
    [[for key, value in pairs(trx.config.list()) do
  trx.log.info(key .. " = " .. tostring(value))
end]],
  },
  impl = raw.list,
})
