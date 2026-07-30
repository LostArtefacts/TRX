trx.events.on_game_start(function(level)
  trx.items[63].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[104].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.objects.switch_type_generic_2.properties.switch_mode =
    trx.items.SwitchMode.SHOVE
  trx.items[45].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[50].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[71].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[84].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[86].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[94].properties.pickup_mode = trx.items.PickupMode.CROWBAR
end)
