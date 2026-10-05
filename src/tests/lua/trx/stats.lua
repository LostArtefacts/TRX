-- What a level keeps count of, as a script sees it.
--
-- Three numbers answer different questions - how much of a thing Lara has, how
-- much of it counts towards completion, and how much the level holds - so all
-- three are pinned here, as is reading them for a level other than the one
-- being played.

local h = require("harness")
local test = h.test

-- The numbers the C side addresses a category by, mirrored from stats.lua so a
-- test can name one when it sets the fake up.
local PICKUPS = 0
local KILLS = 1
local SECRETS = 2

-- The fake's game flow opens with the gym, which has no number of its own, so
-- the first level a script can address is the second in the table.
local CAVES = 2

local function level_with(nums)
  fake.set_current_level(1)
  fake.set_secrets(nums)
end

local function nums_of(secrets)
  local out = {}
  for i, secret in ipairs(secrets) do
    out[i] = secret.num
  end
  return table.concat(out, ",")
end

test("the secrets come back in level order", function()
  level_with({ 1, 3, 7 })
  local secrets = trx.stats.secret_list()
  assert(#secrets == 3, "the level holds three secrets")
  assert(nums_of(secrets) == "1,3,7", "the numbers are the player's, from one")
  for _, secret in ipairs(secrets) do
    assert(not secret.found, "nothing has been found yet")
  end
end)

test("a secret Lara holds reads as found", function()
  level_with({ 1, 2 })
  fake.set_found(2, true)

  local secrets = trx.stats.secret_list()
  assert(not secrets[1].found)
  assert(secrets[2].found)
  assert(trx.stats.secrets.count == 1)
end)

test("give_secret marks one found", function()
  level_with({ 1, 2 })
  assert(trx.stats.give_secret(2))
  assert(trx.stats.secret_list()[2].found, "the secret was not marked")
  assert(trx.stats.secrets.count == 1)
end)

-- The module stands for the level being played, so both spellings mean it.
test("a verb reads the same called either way", function()
  level_with({ 1, 2 })
  assert(trx.stats:give_secret(1))
  assert(trx.stats.give_secret(2))
  assert(trx.stats:secret_list()[1].found)
  assert(trx.stats.secrets.count == 2)
end)

test("giving a secret Lara already holds is refused", function()
  level_with({ 1 })
  assert(trx.stats.give_secret(1))
  assert(not trx.stats.give_secret(1), "the second give must not count")
  assert(trx.stats.secrets.count == 1)
end)

test("take_secret leaves it to be found again", function()
  level_with({ 1, 2 })
  fake.set_found(1, true)

  assert(trx.stats.take_secret(1))
  assert(not trx.stats.secret_list()[1].found)
  assert(trx.stats.secrets.count == 0)
  assert(not trx.stats.take_secret(1), "there is nothing left to take")
end)

test("a number the level has no secret for is refused", function()
  level_with({ 1, 3 })
  assert(not trx.stats.give_secret(2), "the level skips 2")
  assert(not trx.stats.take_secret(2))
end)

-- A number far outside the range would reach the engine as a truncated index,
-- and 33 would land on 1.
test("a number outside the range is refused rather than wrapped", function()
  level_with({ 1 })
  for _, num in ipairs({ 0, -1, 17, 33, 65537 }) do
    assert(not trx.stats.give_secret(num), "gave secret " .. num)
  end
  assert(trx.stats.secrets.count == 0)
end)

test("the completion count is not the length of the list", function()
  level_with({ 1, 2, 3 })
  -- The game flow declares one of them unobtainable, so the level asks for two.
  fake.set_max_secret_count(2)

  assert(#trx.stats.secret_list() == 3, "all three are still there to find")
  assert(trx.stats.secrets.max == 2)
end)

test(
  "what the level holds is what counts plus what is out of reach",
  function()
    fake.set_current_level(CAVES)
    fake.set_max(1, PICKUPS, 34)
    fake.set_unobtainable(1, PICKUPS, 2)

    local pickups = trx.stats.pickups
    assert(pickups.max == 34, "34 of them count")
    assert(pickups.unobtainable == 2)
    assert(pickups.raw == 36, "the level holds all 36")
  end
)

test("a category the game flow writes nothing off reads zero", function()
  fake.set_current_level(CAVES)
  fake.set_max(1, KILLS, 12)

  assert(trx.stats.kills.unobtainable == 0)
  assert(trx.stats.kills.raw == 12)
end)

-- The statistics screen counts the allies against the player only once she has
-- turned on one, so a screen written in Lua needs the same three numbers.
test("the kill maximum says how it divides", function()
  fake.set_current_level(CAVES)
  fake.set_kill_split(1, 2, 10)

  assert(trx.stats.kills.max == 12, "the maximum holds both")
  assert(trx.stats.max_ally_kills == 2)
  assert(trx.stats.max_enemy_kills == 10)
  assert(not trx.stats.allies_hurt, "she has left them alone")

  fake.set_allies_hurt(1, true)
  assert(trx.stats.allies_hurt)
end)

test("a level answers for its own allies", function()
  fake.set_current_level(CAVES)
  fake.set_allies_hurt(2, true)

  assert(not trx.stats.allies_hurt)
  assert(trx.game.levels[2].stats.allies_hurt)
end)

test("a count can be written", function()
  fake.set_current_level(CAVES)
  trx.stats.pickups.count = 7
  trx.stats.timer = 900

  assert(trx.stats.pickups.count == 7)
  assert(trx.stats.timer == 900)
end)

-- The mask behind them is the truth, so a count written straight over it could
-- disagree with which secrets Lara actually holds.
test("the secret count is not one of them", function()
  level_with({ 1, 2 })
  local ok = pcall(function()
    trx.stats.secrets.count = 2
  end)
  assert(not ok, "the count must go through the secret verbs")
  assert(trx.stats.secrets.count == 0)
end)

test("another level's counters are readable", function()
  fake.set_current_level(CAVES)
  fake.set_count(1, KILLS, 3)
  fake.set_count(2, KILLS, 8)
  fake.set_max(2, KILLS, 10)

  local other = trx.game.levels[2].stats
  assert(trx.stats.kills.count == 3, "the level being played counts its own")
  assert(other.kills.count == 8)
  assert(other.kills.max == 10)
end)

test("a hub is the levels a script adds up", function()
  fake.set_current_level(CAVES)
  fake.set_count(1, SECRETS, 1)
  fake.set_count(2, SECRETS, 2)

  local found = 0
  for _, level in ipairs(trx.game.levels) do
    if level.stats ~= nil then
      found = found + level.stats.secrets.count
    end
  end
  assert(found == 3, "the two levels hold three between them")
end)

test("nothing is counted outside a level", function()
  level_with({ 1, 2 })
  fake.set_current_level(nil)

  assert(trx.stats.secrets == nil, "the title screen counts nothing")
  assert(trx.stats.timer == nil)
  assert(trx.stats.secret_list == nil, "nor is there anything to ask")
end)

-- The title screen is a level the game flow is on and the game is not, so the
-- counts have to be guarded by the one they are read against.
test(
  "nor at the title screen, which the game flow counts as a level",
  function()
    level_with({ 1, 2 })
    fake.set_current_title()

    assert(trx.stats.secrets == nil)
    assert(trx.stats.timer == nil)
  end
)

return h.report()
