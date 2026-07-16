trx.events.before_item_setup(function(level)
  trx.items[3].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[4].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[87].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
end)
