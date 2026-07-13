trx.events.before_item_setup(function(level)
  trx.items[64].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[105].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
end)
