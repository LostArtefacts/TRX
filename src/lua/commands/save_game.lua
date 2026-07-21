-- Saves the game, to a numbered slot or to the next quick save.
--
-- Usages:
--   /save 3        the numbered save slot
--   /save quick    the next quick save slot
--   /qs            the next quick save slot

local R = trx.console.Result
local Pool = trx.savegame.Pool

local function quick_save()
  if not trx.savegame.save(nil, Pool.QUICK) then
    return R.FAILURE, trx.locale.get("console/cmd/save/quick_fail_no_slots")
  end
  return R.OK, trx.locale.get("console/cmd/save/quick_success")
end

local function run(args)
  if not trx.game.is_playable then
    return R.UNAVAILABLE
  end

  if args:match("^%-?%d+$") then
    local n = tonumber(args)
    if n < 1 or n > trx.savegame.slot_count() then
      return R.BAD_INVOCATION,
        trx.locale.format("console/cmd/save/fail_invalid_slot", n)
    end
    trx.savegame.save(n)
    return R.OK, trx.locale.format("console/cmd/save/success", n)
  end

  if args == "q" or args == "quick" then
    return quick_save()
  end
  return R.BAD_INVOCATION
end

local function run_quick(args)
  if not trx.game.is_playable then
    return R.UNAVAILABLE
  end
  if args ~= "" then
    return R.BAD_INVOCATION
  end
  return quick_save()
end

trx.console.register({
  name = "save",
  help = "console/cmd/save/help",
  run = run,
})

trx.console.register({
  name = "quicksave",
  aliases = { "qs" },
  help = "console/cmd/save/help",
  run = run_quick,
})
