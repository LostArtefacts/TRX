trx.events.on_game_start(function(level)
  trx.objects.animating_16.properties.collidable = false
  trx.items[26].properties.crowbar = true
  trx.items[123].properties.crowbar = true
  trx.items[11].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[124].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[126].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[125].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.items[127].properties.pickup_mode = trx.items.PickupMode.HIDDEN
end)
