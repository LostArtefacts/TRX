local raw = trxc.game

local function make_levels(table_type)
  local levels = {}
  local count = trxc.game.count_levels(table_type)
  for i = 1, count do
    levels[i] = {
      num = trxc.game.get_level_num(table_type, i),
      name = trxc.game.get_level_name(table_type, i),
      path = trxc.game.get_level_path(table_type, i),
      type = trxc.game.get_level_type(table_type, i),
    }
  end
  return levels
end

trx.game = setmetatable({
  LevelTable = trxc.game.LevelTable,
  LevelType = trxc.game.LevelType,
}, {
  __index = function(self, key)
    local t
    if key == "levels" then
      t = make_levels(trxc.game.LevelTable.MAIN)
    elseif key == "demos" then
      t = make_levels(trxc.game.LevelTable.DEMOS)
    elseif key == "cutscenes" then
      t = make_levels(trxc.game.LevelTable.CUTSCENES)
    elseif key == "current_level" then
      local table_type = trxc.game.get_current_level_table()
      local i = trxc.game.get_current_level_idx()
      return {
        num = trxc.game.get_level_num(table_type, i),
        name = trxc.game.get_level_name(table_type, i),
        path = trxc.game.get_level_path(table_type, i),
        type = trxc.game.get_level_type(table_type, i),
      }
    elseif key == "version" then
      return trxc.game.get_version()
    elseif key == "trx_version" then
      return trxc.game.get_trx_version()
    else
      return nil
    end
    rawset(self, key, t)
    return t
  end,
})
