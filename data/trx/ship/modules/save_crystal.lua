local save_crystal = {}

function save_crystal.initialise()
  trx.events.on_game_start(function(is_save)
    -- TR3 PS1 gave Lara a save crystal on starting the game
    if
      not is_save
      and trx.game.current_level.type == trx.game.LevelType.NORMAL
      and trx.game.current_level.num == 1
      and trx.config.get("gameplay.save_crystal_mode") == "save_pickup"
    then
      trx.inventory.give(trx.catalog.objects.save_crystal_item)
    end
  end)

  -- The crystal modes lock saving to the crystals, so the game offers a save
  -- as each level ends, like the PS1 releases.
  trx.savegame.set_save_offer(function()
    local mode = trx.config.get("gameplay.save_crystal_mode")
    return mode == "save" or mode == "save_pickup"
  end)
end

return save_crystal
