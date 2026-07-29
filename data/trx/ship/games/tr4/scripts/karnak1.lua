trx.events.on_game_start(function(level)
  trx.items[66].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[68].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[82].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[85].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.HIDDEN_REACH
  trx.objects.switch_type_generic_2.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
end)
