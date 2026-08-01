trx.events.on_game_start(function()
  trx.items[58].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[92].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[104].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[120].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW

  trx.objects.animating_16.properties.collidable = false
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
end)
