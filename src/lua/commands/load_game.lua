-- Loads a saved game, by save slot or from a quick save.
--
-- Usages:
--   /load 3        the numbered save slot
--   /load quick    the most recent quick save
--   /load q2       the second quick save
--   /ql            the most recent quick save
--   /ql 2          the second quick save

local R = trx.console.Result
local Pool = trx.savegame.Pool

local function parse_int(s)
  if s:match("^%-?%d+$") then
    return tonumber(s)
  end
  return nil
end

-- Matches "q2", "q 2", "quick 2" and the like, giving back the number.
local function parse_quick_num(s)
  local n = s:match("^q%s*(%-?%d+)$") or s:match("^quick%s*(%-?%d+)$")
  return n and tonumber(n) or nil
end

-- The `load` word: a bare number is a normal slot, and the quick pool needs the
-- quick keyword.
local function parse_load(args)
  local n = parse_int(args)
  if n ~= nil then
    return Pool.NORMAL, n, true
  end
  local q = parse_quick_num(args)
  if q ~= nil then
    return Pool.QUICK, q, true
  end
  if args == "q" or args == "quick" then
    return Pool.QUICK, nil, false
  end
  return nil
end

-- The `quickload` word: everything is a quick save, so a bare number is one too,
-- and no argument means the most recent.
local function parse_quick(args)
  if args == "" then
    return Pool.QUICK, nil, false
  end
  local n = parse_int(args)
  if n ~= nil then
    return Pool.QUICK, n, true
  end
  local q = parse_quick_num(args)
  if q ~= nil then
    return Pool.QUICK, q, true
  end
  return nil
end

local function execute(pool, slot_num, has_slot_num)
  local index, shown
  if pool == Pool.QUICK then
    -- The quick pool is addressed by its on-screen order, and defaults to the
    -- most recent save.
    shown = has_slot_num and slot_num or 1
    if shown < 1 or shown > trx.savegame.slot_count(Pool.QUICK) then
      return R.FAILURE,
        trx.locale.format("console/cmd/load/fail_invalid_slot", shown)
    end
    index = shown
  else
    if slot_num < 1 or slot_num > trx.savegame.slot_count() then
      return R.FAILURE,
        trx.locale.format("console/cmd/load/fail_invalid_slot", slot_num)
    end
    index = slot_num
    shown = slot_num
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

local function run(args)
  local pool, slot_num, has_slot_num = parse_load(args)
  if pool == nil then
    return R.BAD_INVOCATION
  end
  return execute(pool, slot_num, has_slot_num)
end

local function run_quick(args)
  local pool, slot_num, has_slot_num = parse_quick(args)
  if pool == nil then
    return R.BAD_INVOCATION
  end
  return execute(pool, slot_num, has_slot_num)
end

trx.console.register({
  name = "load",
  help = "console/cmd/load/help",
  run = run,
})

trx.console.register({
  name = "quickload",
  aliases = { "ql" },
  help = "console/cmd/load/help",
  run = run_quick,
})
