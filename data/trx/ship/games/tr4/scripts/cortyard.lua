trx.events.on_game_start(function(level)
  trx.items[1].properties.lift = true
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.HIDDEN_PICKUP
  trx.items[3].properties.pickup_mode = trx.items.PickupMode.HIDDEN
end)
