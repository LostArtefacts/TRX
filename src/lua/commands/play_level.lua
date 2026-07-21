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

-- The levels by title, with the gym first, reachable as "gym" or by its title. A
-- number typed straight is read as an ordinal, so both spellings reach the same
-- value.
local function level_choices()
  local out = {}
  local gym = trx.game.gym
  if gym ~= nil then
    out[#out + 1] = { key = "gym", value = "gym" }
    if gym.title ~= nil then
      out[#out + 1] = { key = gym.title, value = "gym" }
    end
  end
  for ordinal, level in ipairs(trx.game.levels) do
    if level.title ~= nil then
      out[#out + 1] = { key = level.title, value = ordinal }
    end
  end
  return out
end

-- A number is an ordinal; a name is fuzzy-matched to one. The levels are not
-- listed back in errors, only offered for completion, so this resolves them
-- itself rather than leaning on choices.
local function level_match(token)
  if token:match("^%-?%d+$") then
    return tonumber(token), true
  end
  local matches = trx.strings.fuzzy_match(token, level_choices())
  if #matches > 0 then
    return matches[1].value, true
  end
  return nil, false
end

local function run(args)
  local pick = args.level
  -- Ordinal zero and the word "gym" both mean Lara's home.
  if pick == 0 or pick == "gym" then
    return start_gym()
  end

  local level = trx.game.levels[pick]
  if level == nil then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/play_level/invalid")
  end
  trx.game.play_level(pick, { select = true })
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/play_level/loading", level.title)
end

for _, name in ipairs({ "play", "level" }) do
  trx.console.register({
    name = name,
    help = "console/cmd/play_level/help",
    args = function(parser)
      parser:positional("level", {
        match = level_match,
        greedy = true,
        suggest = level_choices,
        help = "console/cmd/play_level/level_help",
      })
    end,
    run = run,
  })
end
