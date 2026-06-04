trx.events.after_level_state(function()
  for i = 1, #trx.rooms do
    local room = trx.rooms[i]
    room.damaging = true
    room.cold = true
  end
end)
