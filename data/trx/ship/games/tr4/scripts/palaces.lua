trx.events.before_item_setup(function(level)
  trx.items[64].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW
  trx.items[105].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW
end)
