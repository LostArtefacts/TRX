trx.events.on_game_start(function()
  local level = trx.game.current_level
  local use_one_shot = level ~= nil and level.type ~= trx.game.LevelType.GYM
  trx.rules.set("music.is_one_shot_default", use_one_shot)
end)
