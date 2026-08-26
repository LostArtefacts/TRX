-- Reads or changes any setting, matched by name.
--
-- Usages:
--   /set fov          report the current value
--   /set fov 90       change it
--   /set fov -        put the default back
--   /set -f fov 90    write through a setting the game flow enforces

trx.locale.declare({
  ["console/cmd/set/ambiguous_2"] = "Ambiguous input: %s and %s",
  ["console/cmd/set/ambiguous_3"] = "Ambiguous input: %s, %s, ...",
  ["console/cmd/set/bad_invocation"] = "Invalid invocation: %s",
  ["console/cmd/set/help"] = "Displays or updates the given configuration setting. Use --force to change level-enforced settings for the current session.",
  ["console/cmd/set/option_enforced"] = "%s is enforced and cannot be changed",
  ["console/cmd/set/option_get"] = "%s is currently set to %s",
  ["console/cmd/set/option_set"] = "%s changed to %s",
  ["console/cmd/set/unknown_option"] = "Unknown option: %s",
  ["console/cmd/set/valid_values"] = "Valid values: %s",
})

-- Every setting, by its dashed name, for matching and completion.
local function option_sources()
  local sources = {}
  for key in pairs(trx.config.list()) do
    sources[#sources + 1] =
      { key = trx.strings.dash_case(key), value = key, weight = 1 }
  end
  return sources
end

local function match_options(text)
  return trx.strings.fuzzy_match(text, option_sources())
end

-- The values the option takes, for completion: on and off for a boolean, the
-- enum values, and the dash that puts the default back. A setting that holds a
-- number or a color has no list to offer, so nothing is suggested for it.
local function value_choices(parsed)
  if parsed.option == nil then
    return nil
  end
  local matches = match_options(parsed.option)
  if #matches ~= 1 then
    return nil
  end
  local desc = trx.config.describe(matches[1].value)
  local out = {}
  if desc.kind == "boolean" then
    out[#out + 1] = { key = "on", value = "on" }
    out[#out + 1] = { key = "off", value = "off" }
  elseif desc.kind == "enum" or desc.kind == "dynamic_enum" then
    for _, value in ipairs(desc.values) do
      out[#out + 1] = {
        key = trx.strings.dash_case(value),
        value = trx.strings.dash_case(value),
      }
    end
  else
    return nil
  end
  out[#out + 1] = { key = "-", value = "-" }
  return out
end

-- What the option accepts, for the message after a value that would not parse.
local function valid_values(key)
  local valid = trx.config.accepted_values(key)
  if trx.config.describe(key).kind == "dynamic_enum" and valid ~= nil then
    -- The dash that puts the default back is offered alongside the values.
    valid = "-, " .. valid
  end
  return valid
end

local function run(args)
  if args.option == nil then
    return trx.console.Result.BAD_INVOCATION
  end

  local matches = match_options(args.option)
  if #matches == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/set/unknown_option", args.option)
  end
  if #matches == 2 then
    return trx.console.Result.FAILURE,
      trx.locale.format(
        "console/cmd/set/ambiguous_2",
        matches[1].key,
        matches[2].key
      )
  end
  if #matches > 2 then
    return trx.console.Result.FAILURE,
      trx.locale.format(
        "console/cmd/set/ambiguous_3",
        matches[1].key,
        matches[2].key
      )
  end
  local key = matches[1].value

  if args.value == nil then
    trx.console.log(
      trx.locale.format(
        "console/cmd/set/option_get",
        trx.strings.dash_case(key),
        trx.config.format_value(key)
      )
    )
    return trx.console.Result.OK
  end

  if not args.force and trx.config.is_overridden(key) then
    return trx.console.Result.FAILURE,
      trx.locale.format(
        "console/cmd/set/option_enforced",
        trx.strings.dash_case(key)
      )
  end

  local value = args.value
  local ok
  if value == "-" then
    ok = trx.config.reset(key, args.force)
  else
    ok = pcall(trx.config.set, key, value, args.force)
  end

  if not ok then
    trx.console.log.error(
      trx.locale.format("console/cmd/set/bad_invocation", value)
    )
    local valid = valid_values(key)
    if valid ~= nil then
      trx.console.log(trx.locale.format("console/cmd/set/valid_values", valid))
    end
    return trx.console.Result.FAILURE
  end

  trx.console.log(
    trx.locale.format(
      "console/cmd/set/option_set",
      trx.strings.dash_case(key),
      trx.config.format_value(key)
    )
  )
  return trx.console.Result.OK
end

trx.console.register({
  name = "set",
  help = "console/cmd/set/help",
  args = function(parser)
    parser:flag("force", { short = "-f", long = "--force" })
    -- The option is matched by run, forgivingly and with its own messages, so
    -- the parser suggests the keys and takes the token as it stands.
    parser:positional("option", { optional = true, suggest = option_sources })
    parser:rest("value", { optional = true, suggest = value_choices })
  end,
  run = run,
})
