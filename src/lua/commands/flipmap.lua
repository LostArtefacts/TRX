-- Toggles flip rooms, either a group of them or every group at once.
--
-- Usages:
--   /flip          toggle every group
--   /flip on       force every group on
--   /flip off      force every group off
--   /flip 3        toggle group 3
--   /flip 3 on     force group 3 on

trx.locale.declare({
  ["console/cmd/flipmap/all_already_off"] = "Every flipmap is already OFF",
  ["console/cmd/flipmap/all_already_on"] = "Every flipmap is already ON",
  ["console/cmd/flipmap/all_off"] = "Flipmaps set to OFF",
  ["console/cmd/flipmap/all_on"] = "Flipmaps set to ON",
  ["console/cmd/flipmap/all_toggled"] = "Flipmaps toggled",
  ["console/cmd/flipmap/already_off"] = "Flipmap %d is already OFF",
  ["console/cmd/flipmap/already_on"] = "Flipmap %d is already ON",
  ["console/cmd/flipmap/help"] = "Toggles flip rooms.",
  ["console/cmd/flipmap/no_group"] = "No such flipmap group: %d",
  ["console/cmd/flipmap/off"] = "Flipmap %d set to OFF",
  ["console/cmd/flipmap/on"] = "Flipmap %d set to ON",
})

local function run_one(group, target)
  local current = trx.rooms.is_flipped(group)
  if target == nil then
    target = not current
  end

  if current == target then
    local key = target and "console/cmd/flipmap/already_on"
      or "console/cmd/flipmap/already_off"
    trx.console.log.warning(trx.locale.format(key, group))
    return trx.console.Result.OK
  end

  trx.rooms.flip(group)
  local key = target and "console/cmd/flipmap/on" or "console/cmd/flipmap/off"
  return trx.console.Result.OK, trx.locale.format(key, group)
end

local function run_all(target)
  if target == nil then
    trx.rooms.flip()
    return trx.console.Result.OK,
      trx.locale.get("console/cmd/flipmap/all_toggled")
  end

  local moved = false
  for group = 0, trx.rooms.flip_group_count - 1 do
    if trx.rooms.is_flipped(group) ~= target then
      trx.rooms.flip(group)
      moved = true
    end
  end

  if not moved then
    local key = target and "console/cmd/flipmap/all_already_on"
      or "console/cmd/flipmap/all_already_off"
    trx.console.log.warning(trx.locale.get(key))
    return trx.console.Result.OK
  end

  local key = target and "console/cmd/flipmap/all_on"
    or "console/cmd/flipmap/all_off"
  return trx.console.Result.OK, trx.locale.get(key)
end

local function run(args)
  if not trx.game.is_playable then
    return trx.console.Result.UNAVAILABLE
  end

  if args.group == nil then
    return run_all(args.state)
  end

  if args.group < 0 or args.group >= trx.rooms.flip_group_count then
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/flipmap/no_group", args.group)
  end
  return run_one(args.group, args.state)
end

trx.console.register({
  name = "flip",
  aliases = { "flipmap" },
  help = "console/cmd/flipmap/help",
  args = function(parser)
    parser:positional("group", { type = "integer", optional = true })
    parser:positional("state", { type = "boolean", optional = true })
  end,
  run = run,
})
