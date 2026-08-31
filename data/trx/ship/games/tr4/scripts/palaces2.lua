trx.events.on_game_start(function()
  trx.objects.animating_16.properties.collidable = false
  trx.items[26].properties.crowbar = true
  trx.items[123].properties.crowbar = true
  trx.items[11].properties.switch_mode = trx.items.SwitchMode.HIDDEN_REACH
  trx.items[124].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[126].properties.switch_mode = trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[125].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.items[127].properties.pickup_mode = trx.items.PickupMode.HIDDEN
  trx.items[116].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[5].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[6].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[7].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[8].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[9].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[10].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[13].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[38].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[57].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[60].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[68].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[69].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[70].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[71].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[118].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.items[122].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.ONE_SHOT
end)
