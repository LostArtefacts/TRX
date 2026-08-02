local raw = trxc.config
local api = trx.api

require("trx.locale")

api.module("config", {
  order = 22,
  description = "Module for reading and changing engine settings.\n\n"
    .. "These are the player's settings, not the level's. `trx.config.set` writes to them and keeps the "
    .. "change: it is remembered across saves and relaunches, exactly as if the player had made "
    .. "it themselves. A level that wants to tint the water or pull the fog in wants `trx.config.override` "
    .. "instead, which lasts as long as the script keeps it and leaves the player's own value "
    .. "untouched underneath.",
})

trx.locale.declare({
  ["console/config/accepted_bool"] = "on, off",
  ["console/config/accepted_decimal"] = "[decimal]",
  ["console/config/accepted_integer"] = "[integer]",
  ["console/config/accepted_percent"] = "[integer]",
})

api.type("config.Shape", {
  description = "How a setting is entered and shown, beyond the type it reads back as.",
  fields = {
    kind = {
      type = "string",
      description = "One of `boolean`, `integer`, `number`, `color`, `enum`, "
        .. "`dynamic_enum` or `string`. <!--noref: color, enum, dynamic_enum-->",
    },
    percent = {
      type = "boolean",
      description = "Marks a number stored 0-1 but entered and shown as a 0-100 "
        .. "percentage.",
    },
    values = {
      type = "string",
      list = true,
      description = "What the setting accepts, for the enum kinds. `nil` for the rest.",
    },
  },
})

api.define("config.get", {
  description = "Reads a setting. The value comes back as the type the option is declared with, so "
    .. "a boolean option reads as a boolean. Colors and enums read as strings.",
  params = {
    {
      name = "key",
      type = "string",
      description = "Dotted path, e.g. `visuals.water_color`. <!--noref: visuals.water_color-->",
    },
  },
  returns = { type = "any", description = "Raises if no option has that key." },
  examples = {
    [[if trx.config.get("audio.enable_music") then
  trx.music.play(trx.catalog.music.SECRET)
end]],
  },
  impl = raw.get,
})

api.define("config.describe", {
  description = "What shape a setting has: how a value is entered and shown, beyond the type "
    .. "`trx.config.get` reads back.",
  params = {
    { name = "key", type = "string", description = "Dotted path." },
  },
  returns = {
    type = "config.Shape",
    description = "What the setting is and how it is entered.",
  },
  examples = {
    [[for _, value in ipairs(trx.config.describe("visuals.shadow_type").values) do
  trx.log.info(value)
end]],
  },
  impl = raw.describe,
})

-- The console spells enum values and keys with dashes for underscores.
local function display(text)
  return (text:gsub("_", "-"))
end

api.define("config.format_value", {
  description = "The current value as the console prints it: `1` or `0` for a boolean, two "
    .. "decimals for a plain number, a 0-100 percentage where the option is one, and enum "
    .. "values with dashes for underscores.",
  params = {
    { name = "key", type = "string", description = "Dotted path." },
  },
  returns = { type = "string", description = "The text, ready to print." },
  examples = { [[trx.console.log(trx.config.format_value("visuals.fov"))]] },
  impl = function(key)
    local desc = raw.describe(key)
    local value = raw.get(key)
    if value == nil then
      return "(null)"
    end
    if desc.kind == "boolean" then
      return value and "1" or "0"
    end
    if desc.percent then
      return ("%.0f%%"):format(value * 100)
    end
    if desc.kind == "number" then
      return ("%.2f"):format(value)
    end
    if desc.kind == "enum" or desc.kind == "dynamic_enum" then
      -- A dynamic enum with nothing chosen reads back as an empty string, which
      -- prints as nothing; say so plainly instead.
      if value == "" then
        return "(null)"
      end
      return display(value)
    end
    return tostring(value)
  end,
})

api.define("config.accepted_values", {
  description = "What a setting accepts, as text for an error message: `on, off` for a "
    .. "boolean, a marker like `[integer]` for the number kinds, or the value names for "
    .. "the enum kinds, with dashes for underscores.",
  params = {
    { name = "key", type = "string", description = "Dotted path." },
  },
  returns = {
    type = "string",
    nullable = true,
    description = "`nil` for the kinds with nothing to list, such as a color.",
  },
  impl = function(key)
    local desc = raw.describe(key)
    if desc.kind == "boolean" then
      return trx.locale.get("console/config/accepted_bool")
    end
    if desc.kind == "integer" then
      return trx.locale.get("console/config/accepted_integer")
    end
    if desc.kind == "number" then
      if desc.percent then
        return trx.locale.get("console/config/accepted_percent")
      end
      return trx.locale.get("console/config/accepted_decimal")
    end
    if desc.kind == "enum" or desc.kind == "dynamic_enum" then
      local values = {}
      for _, value in ipairs(desc.values) do
        values[#values + 1] = display(value)
      end
      return table.concat(values, ", ")
    end
    return nil
  end,
})

api.define("config.set", {
  description = "Changes the player's setting, and keeps the change. Raises if the key is unknown "
    .. "or the value will not parse.\n\n"
    .. "The old value is not kept anywhere: the new one becomes the active setting as if the "
    .. "player had chosen it, and is remembered across saves and relaunches. Prefer `trx.config.override` "
    .. "for anything a level wants only while it is running.",
  params = {
    { name = "key", type = "string", description = "Dotted path." },
    {
      name = "value",
      type = "any",
      description = "A boolean, a number, or a string, matching the option's type. A color is a "
        .. "6-digit hex string. An enum value is taken in either spelling: underscores or the "
        .. "dashes the console shows.",
    },
    {
      name = "force",
      type = "boolean",
      optional = true,
      description = "Write through a setting the game flow enforces.",
    },
  },
  impl = function(key, value, force)
    local ok, err = pcall(raw.set, key, value, force)
    if ok then
      return
    end
    -- Enum values are shown with dashes for underscores, so they are taken
    -- in either spelling as well.
    if type(value) == "string" then
      local kind = raw.describe(key).kind
      if kind == "enum" or kind == "dynamic_enum" then
        local variants = { (value:gsub("_", "-")), (value:gsub("-", "_")) }
        for _, variant in ipairs(variants) do
          if variant ~= value and pcall(raw.set, key, variant, force) then
            return
          end
        end
      end
    end
    error(err, 0)
  end,
})

api.define("config.reset", {
  description = "Puts a setting back to its default, and keeps the change, as `trx.config.set` does.",
  params = {
    { name = "key", type = "string", description = "Dotted path." },
    {
      name = "force",
      type = "boolean",
      optional = true,
      description = "As for `trx.config.set`.",
    },
  },
  returns = {
    type = "boolean",
    description = "`false` when a script or the game flow is holding the setting "
      .. "(see `trx.config.is_overridden`).",
  },
  impl = raw.reset,
})

api.define("config.override", {
  description = "Changes a setting for as long as the script keeps the override, without touching "
    .. "the player's own value.\n\n"
    .. "The player's value sits underneath and comes back on `trx.config.restore`. Nothing is written to "
    .. "disk. Overrides stack, so one can be pushed over another; each `trx.config.restore` lifts one off. "
    .. "A setting the game flow enforces cannot be overridden.",
  params = {
    { name = "key", type = "string", description = "Dotted path." },
    { name = "value", type = "any", description = "As for `trx.config.set`." },
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
  returns = {
    type = "boolean",
    description = "True while an override stands, and false once the last is lifted.",
  },
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
