-- Reports or changes the outfit Lara wears.
--
-- Usages:
--   /outfit            what she has on
--   /outfit natla      put that outfit on
--   /outfit -          hand the choice back to the level

local OPTION = "visuals.lara_outfit"

trx.locale.declare({
  ["console/cmd/outfit/current"] = "Lara is wearing %s",
  ["console/cmd/outfit/default"] = "Lara wears what the level dresses her in",
  ["console/cmd/outfit/help"] = "Displays or changes Lara's outfit.",
  ["console/cmd/outfit/outfit_help"] = "which outfit, or - for the level's own",
  ["console/cmd/outfit/unknown"] = "Unknown outfit: %s",
  ["console/cmd/outfit/worn"] = "Lara changed into %s",
})

local function choices()
  local out = { { key = "-", value = "-" } }
  for _, name in ipairs(trx.config.describe(OPTION).values) do
    out[#out + 1] = { key = trx.strings.dash_case(name), value = name }
  end
  return out
end

local function report()
  local worn = trx.lara.outfit
  if worn == nil then
    return trx.console.Result.OK, trx.locale.get("console/cmd/outfit/default")
  end
  return trx.console.Result.OK,
    trx.locale.format(
      "console/cmd/outfit/current",
      trx.strings.dash_case(worn)
    )
end

trx.console.register({
  name = "outfit",
  help = "console/cmd/outfit/help",
  args = function(parser)
    parser:positional("outfit", {
      choices = choices,
      optional = true,
      help = "console/cmd/outfit/outfit_help",
    })
  end,
  run = function(args)
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    if args.outfit == nil then
      return report()
    end

    if args.outfit == "-" then
      trx.config.reset(OPTION)
      return trx.console.Result.OK,
        trx.locale.get("console/cmd/outfit/default")
    end

    if not pcall(trx.config.set, OPTION, args.outfit) then
      return trx.console.Result.FAILURE,
        trx.locale.format("console/cmd/outfit/unknown", args.outfit)
    end
    return trx.console.Result.OK,
      trx.locale.format(
        "console/cmd/outfit/worn",
        trx.strings.dash_case(args.outfit)
      )
  end,
})
