-- Starts a main level, by number or by name.
--
-- Usages:
--   /play 3        by its place in the level list
--   /play 0        Lara's home, which sits at ordinal zero
--   /play caves    by name, fuzzy-matched
--   /play gym      Lara's home

local function start_gym()
  local gym = trx.game.gym
  if gym == nil then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_level/invalid")
  end
  trx.game.play_gym()
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/play_level/loading", gym.title)
end

local function run(args)
  if args == "" then
    return trx.console.Result.BAD_INVOCATION
  end

  local levels = trx.game.levels

  local num = tonumber(args)
  if num ~= nil and num % 1 == 0 then
    if num == 0 then
      return start_gym()
    end
    local level = levels[num]
    if level == nil then
      return trx.console.Result.FAILURE,
        trx.locale.get("console/cmd/play_level/invalid")
    end
    trx.game.play_level(num, { select = true })
    return trx.console.Result.OK,
      trx.locale.format("console/cmd/play_level/loading", level.title)
  end

  -- Match the name against the level titles, with the gym reachable as "gym".
  local sources = {}
  for ordinal, level in ipairs(levels) do
    if level.title ~= nil then
      sources[#sources + 1] =
        { key = level.title, value = ordinal, weight = 1 }
    end
  end
  if trx.game.gym ~= nil then
    sources[#sources + 1] = { key = "gym", value = "gym", weight = 1 }
  end

  local matches = trx.strings.fuzzy_match(args, sources)
  if #matches == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_level/invalid")
  end

  local pick = matches[1].value
  if pick == "gym" then
    return start_gym()
  end
  trx.game.play_level(pick, { select = true })
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/play_level/loading", levels[pick].title)
end

for _, name in ipairs({ "play", "level" }) do
  trx.console.register({
    name = name,
    help = "console/cmd/play_level/help",
    run = run,
  })
end
