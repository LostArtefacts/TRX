-- Reads or changes any setting, matched by name.
--
-- Usages:
--   /set fov          report the current value
--   /set fov 90       change it
--   /set fov -        put the default back
--   /set -f fov 90    write through a setting the game flow enforces

-- The console shows keys and enum values with dashes, not underscores.
local function display(text)
  return (text:gsub("_", "-"))
end

-- Every setting, by its dashed name, for matching.
local function option_sources()
  local sources = {}
  for key in pairs(trx.config.list()) do
    sources[#sources + 1] = { key = display(key), value = key, weight = 1 }
  end
  return sources
end

local function match_options(text)
  return trx.strings.fuzzy_match(text, option_sources())
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
        display(key),
        trx.config.format_value(key)
      )
    )
    return trx.console.Result.OK
  end

  if not args.force and trx.config.is_overridden(key) then
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/set/option_enforced", display(key))
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
      display(key),
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
    -- The option is matched by run, forgivingly and with its own messages.
    parser:positional("option", { optional = true })
    parser:rest("value", { optional = true })
  end,
  run = run,
})
