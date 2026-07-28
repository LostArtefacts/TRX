-- Plays a sound effect, or lists the ones the level has.
--
-- Usages:
--   /sfx        list the available sample ids
--   /sfx 42     play a sample by id

trx.locale.declare({
  ["console/cmd/sfx/available"] = "Available sounds: %s",
  ["console/cmd/sfx/help"] = "Plays a sound effect with the given id.",
  ["console/cmd/sfx/id_help"] = "play the sample with this id",
  ["console/cmd/sfx/invalid"] = "Invalid sound: %d",
  ["console/cmd/sfx/playing"] = "Playing sound %d",
})

local function available_ids()
  local ids = {}
  for id in pairs(trx.sound.samples) do
    ids[#ids + 1] = id
  end
  return ids
end

trx.console.register({
  name = "sfx",
  help = "console/cmd/sfx/help",
  args = function(parser)
    parser:positional("id", {
      type = "integer",
      optional = true,
      metavar = "id",
      help = "console/cmd/sfx/id_help",
    })
  end,
  run = function(args)
    if args.id == nil then
      return trx.console.Result.OK,
        trx.locale.format(
          "console/cmd/sfx/available",
          trx.strings.collapse_ranges(available_ids())
        )
    end

    local sample = trx.sound.samples[args.id]
    if sample == nil then
      return trx.console.Result.FAILURE,
        trx.locale.format("console/cmd/sfx/invalid", args.id)
    end

    sample:play()
    return trx.console.Result.OK,
      trx.locale.format("console/cmd/sfx/playing", args.id)
  end,
})
