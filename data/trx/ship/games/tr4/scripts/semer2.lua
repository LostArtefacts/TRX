trx.events.before_item_setup(function(level)
  trx.items[53].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW
  trx.items[60].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW
  trx.items[61].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW
  trx.items[70].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW
  trx.items[96].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW
  trx.items[97].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW
  trx.items[98].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW
  trx.items[63].properties.lift = true
end)
