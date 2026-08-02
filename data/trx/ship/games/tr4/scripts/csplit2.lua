trx.events.on_game_start(function()
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
  trx.items[18].properties.pickup_mode = trx.items.PickupMode.SARCOPHAGUS
end)
