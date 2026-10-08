-- Toggles the visual debug overlays.
--
-- Usages:
--   /debug              show the state of every overlay
--   /debug on           turn them all on
--   /debug off          turn them all off
--   /debug triggers     toggle one, matched by name
--   /debug triggers on  and force it on or off

trx.locale.declare({
  ["console/cmd/debug/help"] = "Toggles visual debug information.",
  ["console/cmd/debug/option_get"] = "%s is currently set to %s",
  ["console/cmd/debug/option_set"] = "%s changed to %s",
  ["console/cmd/debug/unknown_option"] = "Unknown option: %s",
})

-- The overlays this command reaches, each a boolean config option. The console
-- shows a key with dashes, not underscores.
local KEYS = {
  "debug.enable_debug_portals",
  "debug.enable_debug_room_clip",
  "debug.enable_debug_triggers",
  "debug.enable_debug_zones",
  "debug.enable_debug_spheres",
  "debug.enable_debug_bounding_boxes",
  "debug.enable_debug_pos",
  "debug.enable_debug_anim",
  "debug.enable_debug_camera",
  "debug.enable_debug_status",
}

-- The zone overlay the "zones" option draws. The zones are read through the
-- public module, so the overlay reaches no more of one than a script does.
local ZONE_COLOR = "00ff00"
local ZONE_ALPHA = 160
local ZONE_ALPHA_DISABLED = 48

-- How far above the floor a tile is drawn, so that it does not fight the floor
-- it lies on.
local ZONE_TILE_LIFT = 8

-- The corners a zone is outlined between. A tile reaches every height the level
-- has, which no outline can show, so it is drawn flat on the floor it covers,
-- the way a floor trigger is. The height is taken from the highest of the four
-- corners, so a sloped sector is covered rather than cut through.
local function zone_corners(zone)
  if zone.room_num == nil then
    return zone.min, zone.max
  end
  local room = trx.rooms[zone.room_num]
  if room == nil or not room:is_valid() then
    return nil
  end

  -- A height is read from inside the room, so that a sector with another room
  -- stacked over or under it answers for this one.
  local bounds = room.bounds
  local y = (bounds.min_y + bounds.max_y) // 2
  local top = nil
  for _, x in ipairs({ zone.min.x, zone.max.x }) do
    for _, z in ipairs({ zone.min.z, zone.max.z }) do
      local height = room:floor_height({ x = x, y = y, z = z })
      if height ~= nil and (top == nil or height < top) then
        top = height
      end
    end
  end
  if top == nil then
    return nil
  end

  local floor = top - ZONE_TILE_LIFT
  local min = { x = zone.min.x, y = floor, z = zone.min.z }
  local max = { x = zone.max.x, y = floor, z = zone.max.z }
  return min, max
end

local function log_get(key)
  trx.console.log(
    trx.locale.format(
      "console/cmd/debug/option_get",
      trx.strings.dash_case(key),
      tostring(trx.config.get(key))
    )
  )
end

local function log_set(key)
  trx.console.log(
    trx.locale.format(
      "console/cmd/debug/option_set",
      trx.strings.dash_case(key),
      tostring(trx.config.get(key))
    )
  )
end

local function set(key, enable)
  trx.config.set(key, enable)
  log_set(key)
end

-- Match the typed name against the overlays, by the dashed name the console
-- shows.
local function match_keys(text)
  local sources = {}
  for _, key in ipairs(KEYS) do
    sources[#sources + 1] =
      { key = trx.strings.dash_case(key), value = key, weight = 1 }
  end
  local matched = {}
  for _, m in ipairs(trx.strings.fuzzy_match(text, sources)) do
    matched[#matched + 1] = m.value
  end
  return matched
end

local function overlay_choices()
  local out = {}
  for _, key in ipairs(KEYS) do
    out[#out + 1] = { key = trx.strings.dash_case(key), value = key }
  end
  return out
end

trx.console.register({
  name = "debug",
  help = "console/cmd/debug/help",
  args = function(parser)
    parser:positional("option", { optional = true, suggest = overlay_choices })
    parser:positional("state", { type = "boolean", optional = true })
  end,
  run = function(args)
    if args.option == nil then
      for _, key in ipairs(KEYS) do
        log_get(key)
      end
      return trx.console.Result.OK
    end

    local explicit = args.state
    if explicit == nil then
      -- A lone on/off with no name sets every overlay at once.
      local all = trx.strings.parse_bool(args.option)
      if all ~= nil then
        for _, k in ipairs(KEYS) do
          set(k, all)
        end
        return trx.console.Result.OK
      end
    end

    local matched = match_keys(args.option)
    if #matched == 0 then
      return trx.console.Result.FAILURE,
        trx.locale.format("console/cmd/debug/unknown_option", args.option)
    end

    for _, k in ipairs(matched) do
      local enable = explicit
      if enable == nil then
        enable = not trx.config.get(k)
      end
      set(k, enable)
    end
    return trx.console.Result.OK
  end,
})

trx.scene.on_paint(function()
  if #trx.zones == 0 or not trx.config.get("debug.enable_debug_zones") then
    return
  end
  for _, zone in pairs(trx.zones) do
    local alpha = zone.enabled and ZONE_ALPHA or ZONE_ALPHA_DISABLED
    local centre, radius = zone.centre, zone.radius
    if centre ~= nil and radius ~= nil then
      trx.scene.sphere(centre, radius, ZONE_COLOR, alpha)
    else
      local min, max = zone_corners(zone)
      if min ~= nil then
        trx.scene.box(min, max, ZONE_COLOR, alpha)
      end
    end
  end
end)
