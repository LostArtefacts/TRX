-- Plays a sound effect, or lists the ones the level has.
--
-- Usages:
--   /sfx        list the available sample ids
--   /sfx 42     play a sample by id

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
  run = function(args)
    if args == "" then
      return trx.console.Result.OK,
        trx.locale.format(
          "console/cmd/sfx/available",
          trx.strings.collapse_ranges(available_ids())
        )
    end

    local id = tonumber(args)
    if id == nil or id % 1 ~= 0 then
      return trx.console.Result.BAD_INVOCATION
    end

    local sample = trx.sound.samples[id]
    if sample == nil then
      return trx.console.Result.FAILURE,
        trx.locale.format("console/cmd/sfx/invalid", id)
    end

    sample:play()
    return trx.console.Result.OK,
      trx.locale.format("console/cmd/sfx/playing", id)
  end,
})
