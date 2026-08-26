-- Reads or changes any gameplay rule, matched by name.
--
-- Usages:
--   /rule                     list every rule and its value
--   /rule exposure.damage     report one
--   /rule exposure.damage 25  change it
--   /rule exposure.damage -   put its default back
--
-- Nothing here names a rule. The keys come from trx.rules.list, so a rule
-- added to the engine shows up in the listing and the completion without this
-- file being touched.

trx.locale.declare({
  ["console/cmd/rule/ambiguous"] = "Ambiguous input: %s and %s",
  ["console/cmd/rule/bad_invocation"] = "Invalid invocation: %s",
  ["console/cmd/rule/help"] = "Displays or updates the given gameplay rule. Use - as the value to put its default back.",
  ["console/cmd/rule/rule_get"] = "%s is currently set to %s",
  ["console/cmd/rule/rule_set"] = "%s changed to %s",
  ["console/cmd/rule/unknown_rule"] = "Unknown rule: %s",
})

local function rule_sources()
  local sources = {}
  for _, key in ipairs(trx.rules.list()) do
    sources[#sources + 1] =
      { key = trx.strings.dash_case(key), value = key, weight = 1 }
  end
  return sources
end

local function report(key)
  return trx.locale.format(
    "console/cmd/rule/rule_get",
    trx.strings.dash_case(key),
    trx.rules.format_value(key)
  )
end

local function run(args)
  if args.rule == nil then
    for _, key in ipairs(trx.rules.list()) do
      trx.console.log(report(key))
    end
    return trx.console.Result.OK
  end

  local matches = trx.strings.fuzzy_match(args.rule, rule_sources())
  if #matches == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/rule/unknown_rule", args.rule)
  end
  if #matches > 1 then
    return trx.console.Result.FAILURE,
      trx.locale.format(
        "console/cmd/rule/ambiguous",
        matches[1].key,
        matches[2].key
      )
  end
  local key = matches[1].value

  if args.value == nil then
    return trx.console.Result.OK, report(key)
  end

  local ok
  if args.value == "-" then
    ok = pcall(trx.rules.reset, key)
  else
    ok = pcall(trx.rules.set, key, args.value)
  end
  if not ok then
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/rule/bad_invocation", args.value)
  end

  return trx.console.Result.OK,
    trx.locale.format(
      "console/cmd/rule/rule_set",
      trx.strings.dash_case(key),
      trx.rules.format_value(key)
    )
end

trx.console.register({
  name = "rule",
  help = "console/cmd/rule/help",
  args = function(parser)
    -- The rule is matched by run, forgivingly and with its own messages, so
    -- the parser only suggests the keys for completion.
    parser:positional("rule", { optional = true, suggest = rule_sources })
    parser:rest("value", { optional = true })
  end,
  run = run,
})
