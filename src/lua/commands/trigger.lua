-- The `trigger` and `untrigger` console commands.
--
--   /trigger 12       by item id, as the console counts them
--   /trigger gate     by the item's own unique name
--   /trigger door     by object name, hitting every item of that kind
--   /untrigger door   and the same, taking the trigger back

-- One rule about what may be triggered, asked the same way whichever path found
-- the item.
local function is_targetable(item)
  return item ~= nil
    and item.object_id ~= trx.catalog.objects.LARA
    and not item.is_killed
end

local function apply(item, enable)
  if enable then
    item:trigger()
    return
  end

  item:trigger({ type = trx.items.TriggerType.ANTITRIGGER })

  -- A door reads its trigger and stands itself down, so it has to keep running
  -- to animate shut. A creature never reads its trigger, so it has to be
  -- stopped outright.
  if trx.objects[item.object_id].is_intelligent then
    item:deactivate()
  end
end

-- The keys are spelled out at each call, not looked up from a variable:
-- tools/update_game_strings reads this file to learn which localized strings are
-- still wanted, and it can only see a literal. A key it cannot see is a key it
-- prunes, and the command loses its text.
local function targets_from_id(text)
  local id = tonumber(text)
  if id == nil or id % 1 ~= 0 then
    return nil
  end

  local item = trx.items[id]
  if not is_targetable(item) then
    return {}, trx.locale.format("console/cmd/trigger/invalid_item", text)
  end
  return { item.index }
end

local function targets_from_item_name(text)
  local item = trx.items[text]
  if item == nil then
    return nil
  end
  if not is_targetable(item) then
    return {}, trx.locale.format("console/cmd/trigger/invalid_item", text)
  end
  return { item.index }
end

local function targets_from_object_name(text)
  local ids = trx.objects.query:loaded():by_name(text):ids()
  if #ids == 0 then
    return {}, trx.locale.format("console/cmd/trigger/no_match", text)
  end

  local wanted = {}
  for _, id in ipairs(ids) do
    wanted[id] = true
  end

  local found = {}
  for i = 0, #trx.items - 1 do
    local item = trx.items[i]
    if item ~= nil and wanted[item.object_id] and is_targetable(item) then
      found[#found + 1] = item.index
    end
  end

  if #found == 0 then
    return {}, trx.locale.format("console/cmd/trigger/not_found", text)
  end
  return found
end

-- An id, then the item's own name, then an object name - narrowest first, so an
-- item named "door" wins over every door in the level.
local function targets(text)
  local found, err = targets_from_id(text)
  if found ~= nil then
    return found, err
  end

  found, err = targets_from_item_name(text)
  if found ~= nil then
    return found, err
  end

  return targets_from_object_name(text)
end

local function run(target, enable)
  if not trx.game.is_playable then
    return trx.console.Result.UNAVAILABLE
  end

  local ids, err = targets(target)
  if err ~= nil then
    return trx.console.Result.FAILURE, err
  end

  for _, id in ipairs(ids) do
    apply(trx.items[id], enable)
  end

  local listed = trx.strings.collapse_ranges(ids)
  if enable then
    return trx.console.Result.OK,
      trx.locale.format("console/cmd/trigger/triggered", listed)
  end
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/trigger/untriggered", listed)
end

-- An id, an item name or an object name, taken whole so a name with spaces
-- survives; targets() decides which of the three it is.
local function args(parser)
  parser:rest("target")
end

trx.console.register({
  name = "trigger",
  help = "console/cmd/trigger/help",
  args = args,
  run = function(parsed)
    return run(parsed.target, true)
  end,
})

trx.console.register({
  name = "untrigger",
  help = "console/cmd/trigger/help",
  args = args,
  run = function(parsed)
    return run(parsed.target, false)
  end,
})
