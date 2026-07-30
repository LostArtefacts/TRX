-- The level's secrets as a script sees them.
--
-- Two counts answer different questions - how many secrets the level holds, and
-- how many of them count towards completion - so both are pinned here.

local h = require("harness")
local test = h.test

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
  local secrets = trx.stats.secrets
  assert(#secrets == 3, "the level holds three secrets")
  assert(nums_of(secrets) == "1,3,7", "the numbers are the player's, from one")
  for _, secret in ipairs(secrets) do
    assert(not secret.found, "nothing has been found yet")
  end
end)

test("a secret Lara holds reads as found", function()
  level_with({ 1, 2 })
  fake.set_found(2, true)

  local secrets = trx.stats.secrets
  assert(not secrets[1].found)
  assert(secrets[2].found)
  assert(trx.stats.secret_count == 1)
end)

test("give_secret marks one found", function()
  level_with({ 1, 2 })
  assert(trx.stats.give_secret(2))
  assert(trx.stats.secrets[2].found, "the secret was not marked")
  assert(trx.stats.secret_count == 1)
end)

test("giving a secret Lara already holds is refused", function()
  level_with({ 1 })
  assert(trx.stats.give_secret(1))
  assert(not trx.stats.give_secret(1), "the second give must not count")
  assert(trx.stats.secret_count == 1)
end)

test("take_secret leaves it to be found again", function()
  level_with({ 1, 2 })
  fake.set_found(1, true)

  assert(trx.stats.take_secret(1))
  assert(not trx.stats.secrets[1].found)
  assert(trx.stats.secret_count == 0)
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
  assert(trx.stats.secret_count == 0)
end)

test("the completion count is not the length of the list", function()
  level_with({ 1, 2, 3 })
  -- The game flow declares one of them unobtainable, so the level asks for two.
  fake.set_max_secret_count(2)

  assert(#trx.stats.secrets == 3, "all three are still there to find")
  assert(trx.stats.max_secret_count == 2)
end)

test("nothing is counted outside a level", function()
  level_with({ 1, 2 })
  fake.set_current_level(nil)

  assert(#trx.stats.secrets == 0, "the title screen holds no secrets")
  assert(trx.stats.secret_count == 0)
  assert(trx.stats.max_secret_count == 0)
  assert(not trx.stats.give_secret(1), "there is no level to give one in")
  assert(not trx.stats.take_secret(1))
end)

return h.report()
