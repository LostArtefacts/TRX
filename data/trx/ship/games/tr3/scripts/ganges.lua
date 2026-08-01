trx.events.on_game_start(function()
  local props = trx.objects.quad_bike.properties
  props.track_1 = 9
  props.track_2 = 12
  props.track_3 = 4
  props.track_4 = 12

  -- Setup shoals
  trx.items[5].properties.range = { x = 6, y = 1, z = 14 }
  trx.items[7].properties.range = { x = 2, y = 2, z = 14 }
  trx.items[28].properties.range = { x = 10, y = 2, z = 6 }
  trx.items[70].properties.range = { x = 6, y = 1, z = 10 }
  trx.items[85].properties.range = { x = 6, y = 2, z = 18 }
  trx.items[86].properties.range = { x = 6, y = 1, z = 26 }
  trx.items[89].properties.range = { x = 14, y = 1, z = 6 }
  trx.items[93].properties.range = { x = 18, y = 1, z = 6 }
end)
