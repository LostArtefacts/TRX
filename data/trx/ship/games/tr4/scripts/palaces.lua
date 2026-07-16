trx.events.before_item_setup(function(level)
  trx.items[63].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[104].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
end)
