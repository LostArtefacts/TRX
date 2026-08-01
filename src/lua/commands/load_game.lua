-- Loads a saved game, by save slot or from a quick save.
--
-- Usages:
--   /load 3       the numbered save slot
--   /load q       the most recent quick save
--   /load q2      the second quick save
--   /ql           the most recent quick save
--   /ql 2         the second quick save

trx.locale.declare({
  ["console/cmd/load/fail_invalid_slot"] = "Invalid save slot %d",
  ["console/cmd/load/fail_unavailable_slot"] = "Save slot %d is not available",
  ["console/cmd/load/help"] = "Loads game from the given save slot or from a quick save.",
  ["console/cmd/load/quick_success"] = "Quick-loaded slot %d",
  ["console/cmd/load/slot_help"] = "a slot number, q or quick for the most recent quick save, q2 for the second",
  ["console/cmd/load/success"] = "Loaded game from save slot %d",
})

local R = trx.console.Result
local Pool = trx.savegame.Pool

-- One token names a slot: a bare number is a normal save, `q`/`quick` is the
-- quick pool, and a number stuck to it (`q2`) is that quick save.
local function slot(token)
  if token:match("^%d+$") then
    return { pool = Pool.NORMAL, slot_num = tonumber(token) }, true
  end
  local slot_num = token:match("^q(%d*)$") or token:match("^quick(%d*)$")
  if slot_num ~= nil then
    return {
      pool = Pool.QUICK,
      slot_num = slot_num ~= "" and tonumber(slot_num) or nil,
    },
      true
  end
  return nil, false
end

local function execute(pool, slot_num, has_slot_num)
  local shown
  if pool == Pool.QUICK then
    -- The quick pool is addressed by its on-screen order, and defaults to the
    -- most recent save.
    shown = has_slot_num and slot_num or 1
    if shown < 1 or shown > trx.savegame.slot_count(Pool.QUICK) then
      return R.FAILURE,
        trx.locale.format("console/cmd/load/fail_invalid_slot", shown)
    end
    slot_num = shown
  else
    if slot_num < 1 or slot_num > trx.savegame.slot_count() then
      return R.FAILURE,
        trx.locale.format("console/cmd/load/fail_invalid_slot", slot_num)
    end
    shown = slot_num
  end

  if trx.savegame.is_free(slot_num, pool) then
    return R.FAILURE,
      trx.locale.format("console/cmd/load/fail_unavailable_slot", shown)
  end

  trx.savegame.load(slot_num, pool)
  if pool == Pool.QUICK then
    return R.OK, trx.locale.format("console/cmd/load/quick_success", shown)
  end
  return R.OK, trx.locale.format("console/cmd/load/success", shown)
end

trx.console.register({
  name = "load",
  help = "console/cmd/load/help",
  args = function(parser)
    parser:positional(
      "slot",
      { match = slot, help = "console/cmd/load/slot_help" }
    )
  end,
  run = function(args)
    return execute(
      args.slot.pool,
      args.slot.slot_num,
      args.slot.slot_num ~= nil
    )
  end,
})

trx.console.register({
  name = "quickload",
  aliases = { "ql" },
  help = "console/cmd/load/help",
  args = function(parser)
    parser:positional("slot", { match = slot, optional = true })
  end,
  run = function(args)
    -- Everything the quickload word takes is a quick save, so the slot's pool
    -- is ignored and a bare number is read as a quick slot number (`q2` and `2`
    -- alike).
    local slot_num = args.slot ~= nil and args.slot.slot_num or nil
    return execute(Pool.QUICK, slot_num, slot_num ~= nil)
  end,
})
