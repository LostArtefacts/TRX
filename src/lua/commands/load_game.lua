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

local function execute(pool, index, has_index)
  local shown
  if pool == Pool.QUICK then
    -- The quick pool is addressed by its on-screen order, and defaults to the
    -- most recent save.
    shown = has_index and index or 1
    if shown < 1 or shown > trx.savegame.slot_count(Pool.QUICK) then
      return R.FAILURE,
        trx.locale.format("console/cmd/load/fail_invalid_slot", shown)
    end
    index = shown
  else
    if index < 1 or index > trx.savegame.slot_count() then
      return R.FAILURE,
        trx.locale.format("console/cmd/load/fail_invalid_slot", index)
    end
    shown = index
  end

  if trx.savegame.is_free(index, pool) then
    return R.FAILURE,
      trx.locale.format("console/cmd/load/fail_unavailable_slot", shown)
  end

  trx.savegame.load(index, pool)
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
    return execute(args.slot.pool, args.slot.index, args.slot.index ~= nil)
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
    -- is ignored and a bare number is read as a quick index (`q2` and `2` alike).
    local index = args.slot ~= nil and args.slot.index or nil
    return execute(Pool.QUICK, index, index ~= nil)
  end,
})
