-- The quick settings commands. Each reads or writes one setting, with the
-- messages /set uses.
--
-- Usages:
--   /vsync      report the current value
--   /vsync on   change it
--   /vsync -    put the default back

trx.locale.declare({
  ["console/cmd/braid/help"] = "Toggles Lara's braid.",
  ["console/cmd/cheats/help"] = "Toggles in-game cheats on or off.",
  ["console/cmd/fps/help"] = "Changes the FPS value.",
  ["console/cmd/lighting/help"] = "Toggles lighting system.",
  ["console/cmd/textures/help"] = "Toggles textures.",
  ["console/cmd/vsync/help"] = "Toggles vertical sync.",
  ["console/cmd/wireframe/help"] = "Toggles wireframe rendering.",
})

local COMMANDS = {
  {
    name = "braid",
    key = "visuals.enable_braid",
    help = "console/cmd/braid/help",
  },
  {
    name = "cheats",
    key = "gameplay.enable_cheats",
    help = "console/cmd/cheats/help",
  },
  {
    name = "vsync",
    key = "rendering.enable_vsync",
    help = "console/cmd/vsync/help",
  },
  {
    name = "wireframe",
    key = "rendering.enable_wireframe",
    help = "console/cmd/wireframe/help",
  },
  { name = "fps", key = "rendering.fps", help = "console/cmd/fps/help" },
  {
    name = "lighting",
    key = "rendering.enable_lighting",
    help = "console/cmd/lighting/help",
  },
  {
    name = "textures",
    key = "rendering.enable_textures",
    help = "console/cmd/textures/help",
  },
}

-- What the setting takes, for completion: on/off or the enum values, and the
-- dash that puts the default back. A number has no list to offer, so nothing is
-- advertised for it.
local function value_choices(key)
  local desc = trx.config.describe(key)
  local out = {}
  if desc.kind == "boolean" then
    out[#out + 1] = { key = "on", value = "on" }
    out[#out + 1] = { key = "off", value = "off" }
  elseif desc.kind == "enum" or desc.kind == "dynamic_enum" then
    for _, value in ipairs(desc.values) do
      out[#out + 1] = {
        key = trx.strings.dash_case(value),
        value = trx.strings.dash_case(value),
      }
    end
  else
    return nil
  end
  out[#out + 1] = { key = "-", value = "-" }
  return out
end

local function run(key, args)
  if args.value == nil then
    trx.console.log(
      trx.locale.format(
        "console/cmd/set/option_get",
        trx.strings.dash_case(key),
        trx.config.format_value(key)
      )
    )
    return trx.console.Result.OK
  end

  if trx.config.is_overridden(key) then
    return trx.console.Result.FAILURE,
      trx.locale.format(
        "console/cmd/set/option_enforced",
        trx.strings.dash_case(key)
      )
  end

  local ok
  if args.value == "-" then
    ok = trx.config.reset(key)
  else
    ok = pcall(trx.config.set, key, args.value)
  end

  if not ok then
    trx.console.log.error(
      trx.locale.format("console/cmd/set/bad_invocation", args.value)
    )
    trx.console.log(
      trx.locale.format(
        "console/cmd/set/valid_values",
        trx.config.accepted_values(key)
      )
    )
    return trx.console.Result.FAILURE
  end

  trx.console.log(
    trx.locale.format(
      "console/cmd/set/option_set",
      trx.strings.dash_case(key),
      trx.config.format_value(key)
    )
  )
  return trx.console.Result.OK
end

for _, cmd in ipairs(COMMANDS) do
  trx.console.register({
    name = cmd.name,
    help = cmd.help,
    args = function(parser)
      parser:rest("value", {
        optional = true,
        suggest = function()
          return value_choices(cmd.key)
        end,
      })
    end,
    run = function(args)
      return run(cmd.key, args)
    end,
  })
end
