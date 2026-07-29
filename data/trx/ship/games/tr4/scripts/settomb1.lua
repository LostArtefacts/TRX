trx.events.on_game_start(function(level)
  trx.items[3].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[4].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[87].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[61].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[100].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
end)
