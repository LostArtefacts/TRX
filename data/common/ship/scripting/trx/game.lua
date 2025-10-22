local raw = trxc.game

local function make_level(table_type, i)
  return {
    num = raw.get_level_num(table_type, i),
    name = raw.get_level_name(table_type, i),
    path = raw.get_level_path(table_type, i),
    type = raw.get_level_type(table_type, i),
  }
end

local function make_levels(table_type)
  local count = raw.count_levels(table_type)
  local levels = {}
  for i = 1, count do
    levels[i] = make_level(table_type, i)
  end
  return levels
end

local table_map = {
  levels = raw.LevelTable.MAIN,
  demos = raw.LevelTable.DEMOS,
  cutscenes = raw.LevelTable.CUTSCENES,
}

-- settings system
local settings_getters = {
  lockout_option_ring = function()
    return trx.config.get("flow.lockout_option_ring")
  end,
  load_save_disabled = function()
    return trx.config.get("flow.load_save_disabled")
  end,
}

local settings_setters = {
  lockout_option_ring = function(value)
    trx.config.set("flow.lockout_option_ring", value)
  end,
  load_save_disabled = function(value)
    trx.config.set("flow.load_save_disabled", value)
  end,
}

local Settings = setmetatable({}, {
  __index = function(_, key)
    local getter = settings_getters[key]
    return getter and getter() or nil
  end,
  __newindex = function(_, key, value)
    local setter = settings_setters[key]
    if setter then
      setter(value)
      return
    end
    error("Cannot set field '" .. key .. "' on Settings")
  end,
})

local dynamic_getters = {
  current_level = function()
    return make_level(raw.get_current_level_table(), raw.get_current_level_idx())
  end,
  version = raw.get_version,
  trx_version = raw.get_trx_version,
}

trx.game = setmetatable({
  LevelTable = raw.LevelTable,
  LevelType = raw.LevelType,
  settings = Settings,
  play_level = raw.play_level,
  play_cutscene = raw.play_cutscene,
  play_demo = raw.play_demo,
}, {
  __index = function(self, key)
    local table_type = table_map[key]
    if table_type then
      local t = make_levels(table_type)
      rawset(self, key, t)
      return t
    end

    local getter = dynamic_getters[key]
    return getter and getter() or nil
  end,
  __newindex = function(self, key, value)
    error("Cannot set field '" .. key .. "' on trx.game")
  end,
})
