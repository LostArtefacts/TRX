trx.events.on_game_start(function(level)
  trx.objects.switch_type_generic_2.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
  trx.items[159].properties.pickup_mode = trx.items.PickupMode.CROWBAR
end)
