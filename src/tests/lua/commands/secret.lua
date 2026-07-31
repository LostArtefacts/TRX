local h = require("harness")
local test = h.test

local function secret(args)
  return fake.run("secret", args or "")
end

local function message()
  return fake.calls().log.message
end

local function found_nums()
  local out = {}
  for _, entry in ipairs(trx.stats.secret_list()) do
    if entry.found then
      out[#out + 1] = entry.num
    end
  end
  return table.concat(out, ",")
end

test("a bare command reports what has been found", function()
  fake.set_secrets({ 1, 2, 3 })
  fake.set_found(2, true)

  assert(secret() == trx.console.Result.OK)
  assert(message() == "Secrets collected: 1 of 3 (#2)")
end)

test("a level with nothing found reports the count alone", function()
  fake.set_secrets({ 1, 2 })

  assert(secret() == trx.console.Result.OK)
  assert(message() == "Secrets collected: 0 of 2")
end)

test("a bare number gives that secret", function()
  fake.set_secrets({ 1, 2, 3 })

  assert(secret("2") == trx.console.Result.OK)
  assert(found_nums() == "2")
  assert(message() == "Added secret #2")
end)

test("give and take name one secret", function()
  fake.set_secrets({ 1, 2 })

  assert(secret("give 1") == trx.console.Result.OK)
  assert(found_nums() == "1")

  assert(secret("take 1") == trx.console.Result.OK)
  assert(found_nums() == "")
  assert(message() == "Removed secret #1")
end)

test("a verb with no number acts on every secret", function()
  fake.set_secrets({ 1, 3 })

  assert(secret("give") == trx.console.Result.OK)
  assert(found_nums() == "1,3")
  assert(message() == "Secrets collected: 2 of 2 (#1, #3)")

  assert(secret("take") == trx.console.Result.OK)
  assert(found_nums() == "")
end)

test("all is the spelling of the same thing", function()
  fake.set_secrets({ 1, 2 })

  assert(secret("give all") == trx.console.Result.OK)
  assert(found_nums() == "1,2")
end)

-- The parser offers a verb only the secrets it can act on, so a number that
-- would do nothing never reaches the command.
test("a number the verb cannot act on is refused", function()
  fake.set_secrets({ 1, 3 })
  fake.set_found(1, true)

  assert(secret("give 1") == trx.console.Result.FAILURE, "1 is already found")
  assert(secret("take 3") == trx.console.Result.FAILURE, "3 is not found")
  assert(secret("give 2") == trx.console.Result.FAILURE, "the level skips 2")
  assert(found_nums() == "1", "nothing changed")
end)

test("completion offers the verbs and the secrets together", function()
  fake.set_secrets({ 1, 3 })

  local out = fake.complete_args("secret", "")
  assert(table.concat(out, ",") == "give,take,1,3,all")
end)

test("completion narrows the secrets to what the verb can act on", function()
  fake.set_secrets({ 1, 2, 3 })
  fake.set_found(2, true)

  assert(table.concat(fake.complete_args("secret", "give "), ",") == "1,3,all")
  assert(table.concat(fake.complete_args("secret", "take "), ",") == "2,all")
end)

test("the command needs a level", function()
  fake.set_secrets({ 1 })
  fake.set_current_level(nil)
  assert(secret() == trx.console.Result.UNAVAILABLE)
end)

test("the command does not run in a cutscene", function()
  fake.set_secrets({ 1 })
  fake.set_in_cutscene(true)
  assert(secret("give") == trx.console.Result.UNAVAILABLE)
end)

return h.report()
