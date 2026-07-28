trx.events.on_game_start(function(level)
  trx.creatures.add_ally(trx.catalog.objects.rx_worker_3)
  for _, room in pairs(trx.rooms) do
    room.damaging = room.underwater
    room.cold = true
  end
end)
