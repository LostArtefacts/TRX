trx.events.on_game_start(function()
  for _, room in pairs(trx.rooms) do
    room.damaging = room.underwater
    room.cold = true
  end
end)
