-- Toggles the visual debug overlays.
--
-- Usages:
--   /debug              show the state of every overlay
--   /debug on           turn them all on
--   /debug off          turn them all off
--   /debug triggers     toggle one, matched by name
--   /debug triggers on  and force it on or off

-- The overlays this command reaches, each a boolean config option. The console
-- shows a key with dashes, not underscores.
local KEYS = {
  "debug.enable_debug_portals",
  "debug.enable_debug_room_clip",
  "debug.enable_debug_triggers",
  "debug.enable_debug_spheres",
  "debug.enable_debug_bounding_boxes",
  "debug.enable_debug_pos",
  "debug.enable_debug_anim",
  "debug.enable_debug_camera",
  "debug.enable_debug_status",
}

local function display(key)
  return (key:gsub("_", "-"))
end

local function log_get(key)
  trx.console.log(
    trx.locale.format(
      "console/cmd/debug/option_get",
      display(key),
      tostring(trx.config.get(key))
    )
  )
end

local function log_set(key)
  trx.console.log(
    trx.locale.format(
      "console/cmd/debug/option_set",
      display(key),
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
    sources[#sources + 1] = { key = display(key), value = key, weight = 1 }
  end
  local matched = {}
  for _, m in ipairs(trx.strings.fuzzy_match(text, sources)) do
    matched[#matched + 1] = m.value
  end
  return matched
end

trx.console.register({
  name = "debug",
  help = "console/cmd/debug/help",
  run = function(args)
    if args == "" then
      for _, key in ipairs(KEYS) do
        log_get(key)
      end
      return trx.console.Result.OK
    end

    local key, val = args:match("^(%S+)%s+(.+)$")
    if key == nil then
      key = args
    end
    if val ~= nil and val:match("%s") then
      return trx.console.Result.BAD_INVOCATION
    end

    local explicit
    if val == nil then
      -- A lone on/off with no name sets every overlay at once.
      local all = trx.strings.parse_bool(key)
      if all ~= nil then
        for _, k in ipairs(KEYS) do
          set(k, all)
        end
        return trx.console.Result.OK
      end
    else
      explicit = trx.strings.parse_bool(val)
      if explicit == nil then
        return trx.console.Result.BAD_INVOCATION
      end
    end

    local matched = match_keys(key)
    if #matched == 0 then
      return trx.console.Result.FAILURE,
        trx.locale.format("console/cmd/debug/unknown_option", key)
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
