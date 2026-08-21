local raw = trxc.config
local raw_settings = trxc.settings
local api = trx.api

require("trx.locale")
require("trx.math")

api.module("config", {
  order = 26,
  description = [[Module for reading, changing and declaring engine settings.

These are the player's settings, not the level's. `trx.config.set` writes to them and keeps the
change: it is remembered across saves and relaunches, exactly as if the player had made it
themselves. A level that wants to tint the water or pull the fog in wants `trx.config.override`
instead, which lasts as long as the script keeps it and leaves the player's own value untouched
underneath.

A game can also add settings of its own with `trx.config.declare`: they are saved and loaded with
the player's own, and shown in the settings menu where the declaration asks for. A setting a
game's `scripts/_game.lua` declares belongs to that game and goes when it does.]],
})

trx.locale.declare({
  ["console/config/accepted_bool"] = "on, off",
  ["console/config/accepted_decimal"] = "[decimal]",
  ["console/config/accepted_integer"] = "[integer]",
  ["console/config/accepted_percent"] = "[integer]",
})

api.type("config.Shape", {
  record = true,
  description = [[Everything about a setting but the value it holds now.

`trx.config.describe` hands one back and `trx.config.declare` takes one, so what a script reads
of a setting is what a script writes to make one. The row a declaration asks for is the part
`trx.config.describe` does not report: see `trx.config.Row`.]],
  fields = {
    key = {
      type = "string",
      description = "Dotted path the setting answers to. The last segment is what the "
        .. "settings file keys it on, so it has to differ from every other setting's.",
    },
    kind = {
      type = "string",
      description = "One of `boolean`, `integer`, `number`, `color`, `enum`, "
        .. "`dynamic_enum` or `string`. A declaration writes `boolean`, `integer` or "
        .. "`dynamic_enum`; the rest name storage the engine owns. "
        .. "<!--noref: color, enum, dynamic_enum-->",
    },
    percent = {
      type = "boolean",
      optional = true,
      description = "Marks a number stored 0-1 but entered and shown as a 0-100 "
        .. "percentage.",
    },
    default = {
      type = "any",
      description = "What the setting holds until the player changes it, and what "
        .. "`trx.config.reset` puts back.",
    },
    values = {
      type = "string",
      list = true,
      optional = true,
      description = "What the setting accepts, for the enum kinds. A declared "
        .. "`dynamic_enum` needs them, and they have to list the default. "
        .. "<!--noref: dynamic_enum-->",
    },
    min = {
      type = "integer",
      optional = true,
      description = "Lowest value a number takes. Absent, it takes as low a number as "
        .. "it can hold.",
    },
    max = {
      type = "integer",
      optional = true,
      description = "Highest value a number takes. Absent, it takes as high a number "
        .. "as it can hold.",
    },
    ui = {
      type = "config.Row",
      optional = true,
      description = "The row a declared setting takes in the settings menu. Without "
        .. "one the setting has no row, and is the script's to read and write.",
    },
  },
})

api.type("config.Row", {
  record = true,
  description = [[The settings row a declared setting is shown on: where it sits, and what it
does that the setting itself cannot say.

Every callback below is optional, and one that raises is logged and answered as though it were
absent. They are read as the setting is declared and are not reported back by
`trx.config.describe`.]],
  fields = {
    tab = {
      type = "string",
      description = "Settings tab the row sits on: `gameplay_general`, `gameplay_controls`, "
        .. "`gameplay_mods`, `gameplay_fixes`, `graphic_visuals`, `graphic_ui`, "
        .. "`graphic_ui_stats`, `graphic_ui_bars`, `graphic_rendering`, `sound_volume` or "
        .. "`sound_misc`. <!--noref: gameplay_general, gameplay_controls, gameplay_mods, "
        .. "gameplay_fixes, graphic_visuals, graphic_ui, graphic_ui_stats, graphic_ui_bars, "
        .. "graphic_rendering, sound_volume, sound_misc-->",
    },
    before = {
      type = "string",
      optional = true,
      description = "Setting the row sits above. Steadier than a position: the row stays "
        .. "put when the tab is reordered.",
    },
    after = {
      type = "string",
      optional = true,
      description = "Setting the row sits below. The row lands at the end of the tab where "
        .. "neither anchor is given.",
    },
    delta_fast = {
      type = "integer",
      optional = true,
      description = "How far one press moves a number. One step where it is absent.",
    },
    delta_slow = {
      type = "integer",
      optional = true,
      description = "How far one press moves a number while fine adjustment is held.",
    },
    format_value = {
      type = "function",
      optional = true,
      description = "Called with the value, returning what the row prints in place of it.",
    },
    is_available = {
      type = "function",
      optional = true,
      description = "Called with the value. Return false to grey the row out: it stays "
        .. "visible, and the player cannot move it.",
    },
    is_visible = {
      type = "function",
      optional = true,
      description = "Called with the value. Return false to leave the row out of the tab.",
    },
    can_change_value = {
      type = "function",
      optional = true,
      description = "Called with the value and the direction, `-1` or `1`. Return false to "
        .. "refuse that press.",
    },
    request_change_value = {
      type = "function",
      optional = true,
      description = "Called with the value and the direction. Return true to take the press "
        .. "over; the row is left alone, and moving the setting is the script's to do.",
    },
  },
})

api.define("config.get", {
  description = "Reads a setting. The value comes back as the type the option is declared with, so "
    .. "a boolean option reads as a boolean and a color as a `trx.math.Color`. Enums read as strings.",
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
  description = "Everything about a setting but the value it holds now: what it is, what it "
    .. "accepts, and what it falls back to. This is the shape `trx.config.declare` takes, so a "
    .. "script can read one setting and declare another like it.",
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
        .. "`trx.math.Color` or the hex text one is written as. An enum value is taken in either "
        .. "spelling: underscores or the dashes the console shows.",
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
  description = "Lifts one override off a setting, putting back the value underneath it.",
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

api.define("config.declare", {
  description = [[Adds a setting of the game's own.

The declaration carries no text. The engine derives `settings/<key>/title`,
`settings/<key>/description` and, for an enum, `settings/<key>/values/<value>`, and looks each up
in the game strings, so a declared setting is translated as every other one is.

The setting comes up holding the player's saved value for it, whether the declaration runs before
the settings file is read or after.

Raises where the key is taken, or where the declaration describes a setting that could hold
nothing it allows: an enum defaulting to a value it does not list, or an integer defaulting
outside its own bounds.]],
  params = {
    {
      name = "spec",
      type = "config.Shape",
      description = "The setting's declaration.",
    },
  },
  examples = {
    [[trx.config.declare({
  key = "mod.water_color_mode",
  kind = "dynamic_enum",
  values = { "tombati", "dos", "custom" },
  default = "custom",
  ui = {
    tab = "graphic_visuals",
    before = "visuals.water_color",
  },
})]],
  },
  impl = function(spec)
    raw.declare(spec)
    if spec.ui ~= nil then
      raw_settings.add_row(spec.key, spec.ui)
    end
  end,
})

-- What on_change hands back. The engine keys a watcher by a number, and the
-- number is the module's business rather than a script's.
local Watcher = api.type("config.Watcher", {
  description = [[A setting being watched. `trx.config.on_change` hands one back, and holding it
is what lets the watcher be dropped later. A watcher is spent once detached, and the end of a level
spends every one a level script attached.]],
  methods = {
    detach = {
      description = "Stops the watcher, which hears of no further change.",
      returns = {
        type = "boolean",
        description = "Whether it was still watching.",
      },
      impl = function(self)
        return raw.off_change(rawget(self, "_id"))
      end,
    },
  },
})

api.define("config.on_change", {
  description = [[Calls `fn(value)` whenever the setting changes, and once as the watcher is
attached with the value it holds now - so a script applies the player's saved value rather than
waiting for them to touch it again.

A watcher that changes a setting itself is heard by that setting's watchers too. One that raises
is logged and the rest still run; it is called again on the next change.

A watcher a level script attaches goes when the level ends, as a `trx.events` listener does. One a
game script attaches stays for as long as the game.]],
  params = {
    { name = "key", type = "string", description = "Dotted path to watch." },
    {
      name = "fn",
      type = "function",
      description = "Called with the setting's value.",
    },
  },
  returns = {
    type = "config.Watcher",
    description = "The watcher, for dropping it later.",
  },
  examples = {
    [[local watcher = trx.config.on_change("mod.scanlines", function(value)
  trx.log.info("scanlines are now " .. tostring(value))
end)
-- ... and when the script is done with it:
watcher:detach()]],
  },
  impl = function(key, fn)
    return setmetatable({ _id = raw.on_change(key, fn) }, Watcher)
  end,
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
