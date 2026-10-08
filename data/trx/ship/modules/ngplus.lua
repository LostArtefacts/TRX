local ngplus = {}

function ngplus.initialise()
  trx.events.on_game_start(function()
    local level = trx.game.current_level
    if trx.game.is_ngplus then
      trx.rules.health.scale = 2
    elseif level ~= nil and level.type == trx.game.LevelType.DEMO then
      -- Rules outlive the game they were set in; a demo plays the original.
      trx.rules.reset("health.scale")
    end
  end)
end

return ngplus
