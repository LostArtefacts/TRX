trx.events.on_game_start(function()
  -- Setup shoals
  trx.items[15].properties.range = { x = 6, y = 5, z = 10 }
  trx.items[51].properties.range = { x = 18, y = 8, z = 18 }

  trx.objects.flame_emitter_side.properties.interval = 4
end)
