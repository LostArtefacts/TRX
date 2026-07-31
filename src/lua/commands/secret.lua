-- Lists the level's secrets, or gives Lara one and takes it back.
--
-- Usages:
--   /secret          what the level holds, and what Lara has of it
--   /secret 3        give her secret 3
--   /secret give     give her every secret in the level
--   /secret give 3
--   /secret take     take them all back
--   /secret take 3

trx.locale.declare({
  ["console/cmd/secret/action_help"] = "whether to give the secret or take it back",
  ["console/cmd/secret/given"] = "Added secret %s",
  ["console/cmd/secret/help"] = "Lists Lara's secrets, or takes/gives a secret by number.",
  ["console/cmd/secret/list"] = "Secrets collected: %d of %d (%s)",
  ["console/cmd/secret/none"] = "Secrets collected: %d of %d",
  ["console/cmd/secret/secret_help"] = "which secret, or all of them",
  ["console/cmd/secret/taken"] = "Removed secret %s",
})

local function display(num)
  return "#" .. num
end

-- The secrets a verb can act on: the ones still to find for give, the ones Lara
-- holds for take, and everything the level has when no verb was typed.
local function targets(action)
  local out = {}
  for _, secret in ipairs(trx.stats.secret_list()) do
    if action == nil or secret.found == (action == "take") then
      out[#out + 1] = { key = tostring(secret.num), value = secret.num }
    end
  end
  return out
end

local function report()
  local held = {}
  for _, secret in ipairs(trx.stats.secret_list()) do
    if secret.found then
      held[#held + 1] = display(secret.num)
    end
  end

  local count = trx.stats.secrets.count
  local max_count = trx.stats.secrets.max
  if #held == 0 then
    return trx.locale.format("console/cmd/secret/none", count, max_count)
  end
  return trx.locale.format(
    "console/cmd/secret/list",
    count,
    max_count,
    table.concat(held, ", ")
  )
end

local function change(action, num)
  if action == "take" then
    return trx.stats.take_secret(num)
  end
  return trx.stats.give_secret(num)
end

local function change_one(action, num)
  if not change(action, num) then
    -- The parser offers only a secret the verb can act on, so reaching here
    -- means the level has moved on since. Say where it stands.
    return trx.console.Result.FAILURE, report()
  end
  local key = action == "take" and "console/cmd/secret/taken"
    or "console/cmd/secret/given"
  return trx.console.Result.OK, trx.locale.format(key, display(num))
end

local function change_all(action)
  for _, secret in ipairs(trx.stats.secret_list()) do
    change(action, secret.num)
  end
  return trx.console.Result.OK, report()
end

trx.console.register({
  name = "secret",
  help = "console/cmd/secret/help",
  args = function(parser)
    parser:positional("action", {
      choices = { "give", "take" },
      optional = true,
      help = "console/cmd/secret/action_help",
    })
    parser:any_of("secret", {
      {
        choices = function(parsed)
          return targets(parsed.action)
        end,
      },
      { choices = { "all" } },
    }, { optional = true, help = "console/cmd/secret/secret_help" })
  end,
  run = function(args)
    if not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    if args.action == nil and args.secret == nil then
      return trx.console.Result.OK, report()
    end

    -- A bare number is a give, which is what a player asking for one means.
    local action = args.action or "give"
    if args.secret == nil or args.secret == "all" then
      return change_all(action)
    end
    return change_one(action, args.secret)
  end,
})
