-- The quick settings commands. Each reads or writes one setting, with the
-- messages /set uses.
--
-- Usages:
--   /vsync      report the current value
--   /vsync on   change it
--   /vsync -    put the default back

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

-- The console shows an option key with dashes, not underscores.
local function display(key)
  return (key:gsub("_", "-"))
end

local function run(key, args)
  if args == "" then
    trx.console.log(
      trx.locale.format(
        "console/cmd/easy_config/option_get",
        display(key),
        trx.config.format_value(key)
      )
    )
    return trx.console.Result.OK
  end

  if trx.config.is_overridden(key) then
    return trx.console.Result.FAILURE,
      trx.locale.format(
        "console/cmd/easy_config/option_enforced",
        display(key)
      )
  end

  local ok
  if args == "-" then
    ok = trx.config.reset(key)
  else
    ok = pcall(trx.config.set, key, args)
  end

  if not ok then
    trx.console.log.error(
      trx.locale.format("console/cmd/easy_config/bad_invocation", args)
    )
    trx.console.log(
      trx.locale.format(
        "console/cmd/easy_config/valid_values",
        trx.config.accepted_values(key)
      )
    )
    return trx.console.Result.FAILURE
  end

  trx.console.log(
    trx.locale.format(
      "console/cmd/easy_config/option_set",
      display(key),
      trx.config.format_value(key)
    )
  )
  return trx.console.Result.OK
end

for _, cmd in ipairs(COMMANDS) do
  trx.console.register({
    name = cmd.name,
    help = cmd.help,
    run = function(args)
      return run(cmd.key, args)
    end,
  })
end
