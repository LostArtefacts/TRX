trx.events.before_level_file(function(level)
  trx.creatures.add_ally(trx.catalog.objects.rx_worker_3)
end)

trx.events.after_level_state(function()
  for _, room in pairs(trx.rooms) do
    room.damaging = room.underwater
    room.cold = true
  end
end)
