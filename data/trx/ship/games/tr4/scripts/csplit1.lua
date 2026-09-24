trx.events.on_game_start(function()
  trx.items[58].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[92].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[104].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[120].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW

  trx.objects.animating_16.properties.collidable = false
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.SHOVE

  trx.items[65].properties.speed = 15
  trx.items[65].properties.travel_distance = 24
  trx.items[79].properties.is_pressure_plate = true
  trx.items[79].properties.travel_distance = 1
end)
