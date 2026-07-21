-- Reads or sets the game's turbo speed.
--
-- Usages:
--   /speed        report the current speed
--   /speed 2      set it

local KEY = "gameplay.turbo_speed"

trx.console.register({
  name = "speed",
  help = "console/cmd/speed/help",
  args = function(parser)
    parser:positional("speed", { type = "integer", optional = true })
  end,
  run = function(args)
    if args.speed == nil then
      return trx.console.Result.OK,
        trx.locale.format("console/cmd/speed/get", trx.config.get(KEY))
    end

    -- config.set writes and persists it; the config-change reaction clamps it
    -- and re-anchors the sim clock. Read it back for the message, in case it
    -- was clamped.
    trx.config.set(KEY, args.speed)
    return trx.console.Result.OK,
      trx.locale.format("console/cmd/speed/set", trx.config.get(KEY))
  end,
})
