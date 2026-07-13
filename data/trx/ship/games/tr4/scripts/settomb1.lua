trx.events.before_item_setup(function(level)
  trx.items[4].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[5].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[88].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
end)
