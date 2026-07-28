-- Saves the game, to a numbered slot or to a quick save.
--
-- Usages:
--   /save 3       the numbered save slot
--   /save q       the next quick save slot
--   /save q2      the second quick save slot
--   /qs           the next quick save slot

trx.locale.declare({
  ["console/cmd/save/fail_invalid_slot"] = "Invalid save slot %d",
  ["console/cmd/save/help"] = "Saves game to the given save slot or to the next quick save slot.",
  ["console/cmd/save/quick_fail_no_slots"] = "No quick save slots are configured",
  ["console/cmd/save/quick_success"] = "Quick-saved",
  ["console/cmd/save/slot_help"] = "a slot number, q or quick for the next quick save, q2 for the second",
  ["console/cmd/save/success"] = "Saved game to save slot %d",
})

local R = trx.console.Result
local Pool = trx.savegame.Pool

-- One token names a slot: a bare number is a normal save, `q`/`quick` is the
-- quick pool, and a number stuck to it (`q2`) is that quick save.
local function slot(token)
  if token:match("^%d+$") then
    return { pool = Pool.NORMAL, index = tonumber(token) }, true
  end
  local index = token:match("^q(%d*)$") or token:match("^quick(%d*)$")
  if index ~= nil then
    return {
      pool = Pool.QUICK,
      index = index ~= "" and tonumber(index) or nil,
    },
      true
  end
  return nil, false
end

local function quick_save(index)
  -- A named quick slot is range-checked the way a normal slot is; the next-slot
  -- form (no index) leaves it to the save system.
  if
    index ~= nil
    and (index < 1 or index > trx.savegame.slot_count(Pool.QUICK))
  then
    return R.BAD_INVOCATION,
      trx.locale.format("console/cmd/save/fail_invalid_slot", index)
  end
  if not trx.savegame.save(index, Pool.QUICK) then
    return R.FAILURE, trx.locale.get("console/cmd/save/quick_fail_no_slots")
  end
  return R.OK, trx.locale.get("console/cmd/save/quick_success")
end

trx.console.register({
  name = "save",
  help = "console/cmd/save/help",
  args = function(parser)
    parser:positional(
      "slot",
      { match = slot, help = "console/cmd/save/slot_help" }
    )
  end,
  run = function(args)
    if not trx.game.is_playable then
      return R.UNAVAILABLE
    end
    if args.slot.pool == Pool.QUICK then
      return quick_save(args.slot.index)
    end
    local index = args.slot.index
    if index < 1 or index > trx.savegame.slot_count() then
      return R.BAD_INVOCATION,
        trx.locale.format("console/cmd/save/fail_invalid_slot", index)
    end
    trx.savegame.save(index)
    return R.OK, trx.locale.format("console/cmd/save/success", index)
  end,
})

trx.console.register({
  name = "quicksave",
  aliases = { "qs" },
  help = "console/cmd/save/help",
  run = function()
    if not trx.game.is_playable then
      return R.UNAVAILABLE
    end
    return quick_save(nil)
  end,
})
